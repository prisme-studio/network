//
//  Socket.cpp
//  network
//
//  Created by Valentin Dufois on 2019-09-29.
//

#include "Socket.hpp"

#include <common/log.hpp>

namespace network {

// MARK: - BaseSocket

void BaseSocket::connectTo(const std::string &ip, const NetworkPort &port) {
	connectTo(Endpoint(ip, port));
}

void BaseSocket::connectTo(const Endpoint &remote) {
	if(_status != idle && _status != closed) {
		LOG_ERROR("This socket could not be opened");
	}

	_status = SocketStatus::connecting;

	_remote = remote;

	// Open the socket
	boost::system::error_code ec;
	_socket.open(asio::ip::tcp::v4(), ec);

	Engine::instance()->runContext();

	// Check errors
	if(ec) {
		LOG_ERROR(ec.message());
		return;
	}


	LOG_DEBUG("Opening connection to " + _remote.uri());

	// Connect synchronously
	_socket.connect(_remote, ec);

	// Check errors
	if(ec) {
		_status = SocketStatus::idle;
		LOG_ERROR(ec.message());
		_socket.shutdown(asio::socket_base::shutdown_both, ec);
		_socket.close(ec);
		return;
	}

	LOG_INFO("Connected to " + remote.uri());

	prepareReceive();

	_status = SocketStatus::ready;

	if(delegate)
		delegate->socketDidOpen(this);
}

void BaseSocket::close() {
	if(_status != ready)
		return;

	_status = closed;

	LOG_INFO("Closing connection with " + _remote.ip + ":" + std::to_string(_remote.port));

	boost::system::error_code ec;
	_socket.shutdown(asio::socket_base::shutdown_both, ec);
	_socket.close(ec);

	if(delegate)
		delegate->socketDidClose(this);
}

BaseSocket::~BaseSocket() {
	if(_status == closed)
		return;

	// Make sure the socket is properly closed
	_receiveMutex.lock();
	_sendSyncMutex.lock();

	close();
}


// MARK: - Exchanges

void BaseSocket::send(const google::protobuf::Message * message) {
	// Make sure the socket is ready to send data
	if(getStatus() != SocketStatus::ready) {
		LOG_WARN("Could not send data on a not-ready socket. The socket may not be opened yet or is already closed.");
		return;
	}

	switch(getEmissionType()) {
		case EmissionType::sync:
			sendSync(message);
			break;
		case EmissionType::async:
			sendAsync(message);
			break;
	}
}


// MARK: - Internal

void BaseSocket::onOpenedFromRemote(const Endpoint::Type &remoteType) {
	_remote = Endpoint(_socket.remote_endpoint());
	_remote.type = remoteType;

	_status = SocketStatus::ready;

	LOG_INFO("Connected the " + _remote.type + " server to client on " + _remote.uri());

	prepareReceive();

	if(canPing())
		ping(this);

	if(delegate)
		delegate->socketDidOpen(this);
}

void BaseSocket::onError() {
	close();
}

// MARK: - Emission

void BaseSocket::sendSync(const google::protobuf::Message * message) {
	_sendSyncMutex.lock();

	// Send the message to the output buffer (through the outputStream)
	formatMessageToStream(message, _outputStream);

	boost::system::error_code error;
	startTimer();

	// Send the datagram
	_socket.send(_outputBuffer.data(), boost::asio::socket_base::message_flags(), error);

	endTimer();

	if (error) {
		LOG_ERROR("An error occured while sending data synchronously");
		LOG_ERROR(error.message());

		_sendSyncMutex.unlock();
		close();
		return;
	}

	// Clear the buffer
	_outputBuffer.consume(_outputBuffer.size());

	_sendSyncMutex.unlock();
}

void BaseSocket::sendAsync(const google::protobuf::Message * message) {
	// Queue a copy of the message
	_asyncQueue.enqueue(message);

	// Execute send
	sendAsyncInternal();
}

void BaseSocket::sendAsyncInternal() {
	// Are we already sending something ?
	if(_isAsyncSending) {
		// Yes, do nothing
		return;
	}

	_isAsyncSending = true;

	// Get next message to send
	const protobuf::Message * message;

	if(!_asyncQueue.try_dequeue(message)) {
		_isAsyncSending = false;
		return;
	}

	// Default output buffer is not used for async call and must be preserved valid for sync calls
	asio::streambuf outputBuffer;
	std::ostream outputStream(&outputBuffer);

	// Put the message in the buffer
	formatMessageToStream(message, outputStream);

	// Send the datagram
	_socket.async_write_some(outputBuffer.data(), [&] (const boost::system::error_code &error, std::size_t bytes_transferred) {

		// Tell the delegate the message is sent
		if(delegate)
			delegate->socketDidSendAsynchronously(this, message);

		if(error) {
			LOG_ERROR("An error occured while sending data asynchronously");
			LOG_ERROR(error.message());
			_isAsyncSending = false;
			close();
			return;
		}

		_isAsyncSending = false;
		sendAsyncInternal();
	});
}

void BaseSocket::formatMessageToStream(const protobuf::Message * message, std::ostream & _outputStream) {
	switch(_format) {
		case SocketFormat::protobuf:
			message->SerializeToOstream(&_outputStream);
			break;

		case SocketFormat::json:
			std::string messageString;
			protobuf::util::MessageToJsonString(*message, &messageString);
			_outputStream << messageString;
			_outputStream << "\n";
			break;
	}
}
// MARK: - Reception

void BaseSocket::prepareReceive() {
	switch(_format) {
		case SocketFormat::protobuf:
			_socket.async_receive(asio::buffer(_receptionBuffer), boost::bind(&BaseSocket::handleReceive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			break;

		case SocketFormat::json:
			boost::asio::async_read_until(_socket, _receptionStreamBuffer, "\r\n\r\n", boost::bind(&BaseSocket::handleReceive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			break;
	}

	Engine::instance()->runContext();
}

void BaseSocket::handleReceive(const boost::system::error_code &error, std::size_t bytes_transferred) {

	// Check for any error during reception
	if(error) {
		if(error == asio::error::operation_aborted || error == asio::error::eof)
			return;

		LOG_ERROR("Error while receiving data. Closing socket");
		LOG_ERROR(error.message());

		close();
		return;
	}

	_receiveMutex.lock();

	// Check we haven't reached the buffer size
	if(bytes_transferred >= RECEPTION_BUFFER_SIZE) {
		LOG_WARN("TCP Connection reception buffer sized reach. If the message was larger than the buffer size, ignoring packet");

		_receiveMutex.unlock();
		return prepareReceive();
	}

	// Decode the message using the proper format
	protobuf::Message * message = nullptr;

	switch(_format) {
		case protobuf:
			message = decodeMessageFromBuffer(_receptionBuffer, bytes_transferred);
			break;
		case json:
			message = decodeMessageFromBuffer(&_receptionStreamBuffer, bytes_transferred);
	}

	// Pass along the received datagram
	onReceive(message);

	_receiveMutex.unlock();

	return prepareReceive();
}

} /* ::network */
