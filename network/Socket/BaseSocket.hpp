//
//  BaseSocket.hpp
//  network
//
//  Created by Valentin Dufois on 2020-02-05.
//

#ifndef BaseSocket_hpp
#define BaseSocket_hpp

#include <atomic>
#include <iostream>
#include <type_traits>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include <common/log.hpp>

#include "SocketStatus.hpp"
#include "../Endpoint.hpp"
#include "../Engine.hpp"

#define RECEPTION_BUFFER_SIZE 128000

namespace asio = boost::asio;
namespace protobuf = google::protobuf;

namespace network {

class SocketDelegate;

/// A Socket represents a connection over the network between two machines.
///
/// A Socket handles all logics needed to open a connection, send and receive datagrams, and close the connection.
/// Received datagrams, as well as multiple events, can be catched using a SocketDelegate subclass.
class BaseSocket {
public:

	BaseSocket(): _outputStream(&_outputBuffer) {}

	SocketDelegate * delegate = nullptr;

	// MARK: - Lifecycle

	/// Connects the socket to the given ip and port
	void connectTo(const std::string &ip, const NetworkPort &port);

	/// Connects the socket to the given endpoint
	void connectTo(const Endpoint &remote);

	/// Terminates the connection, closing the socket.
	///
	/// This method may be called by the socket on itself if certain conditions are
	/// met, such as when the remote closes the socket on its side.
	/// Calls to this method triggers the callback `onClose`.
	void close();

	virtual ~BaseSocket();


	// MARK: - Exchanges

	/// Sends the given message to the connected remote
	///
	///	If the socket is sent to send data synchronously this method
	///	will block until the end of the emission
	///	of the emission
	///
	/// @param message The message to send
	void send(const protobuf::Message * message);

	// MARK: - Getters & Setters

	/// Gives the underlying asio socket
	inline asio::ip::tcp::socket & getSocket() { return _socket; }

	/// Gives the status of the socket
	inline SocketStatus getStatus() const { return _status; }

	/// Gives the `EmissionType` of the socket
	inline EmissionType getEmissionType() const { return _emissionType; }

	/// Sets the `EmissionType` of the socket
	/// @param et An emission type
	inline void setEmissionType(const EmissionType &et) { _emissionType = et; }

	/// Gives the remote endpoint this socket is connected to
	inline Endpoint getRemote() const { return _remote; }

	/// Gives the exchange format used by the socket
	inline SocketFormat getFormat() const { return _format; }

	/// Sets the exchange format used by the socket
	/// @param aFormat An exchange format
	inline void setFormat(const SocketFormat &aFormat) {
		_format = aFormat;
	}

	// MARK: - Internal

protected:

	/// The socket exchange format
	SocketFormat _format = SocketFormat::protobuf;

	// We want the `BaseServer`s, and only the BaseServers, to be able to call `onOpenedFromRemote()`
	friend class BaseServer;

	/// Executed when the socket was created by another endpoint
	/// @param remoteType The type of the other endpoint
	void onOpenedFromRemote(const Endpoint::Type &remoteType);

	/// Executed when a fatal error occur during emission or reception
	void onError();

	virtual bool canPing() = 0;

	virtual void ping(BaseSocket *) = 0;

private:

	/// The underlying Asio socket
	asio::ip::tcp::socket _socket = asio::ip::tcp::socket(Engine::instance()->getContext());

	/// The status of the socket
	SocketStatus _status = SocketStatus::idle;

	/// The socket emission type
	EmissionType _emissionType = EmissionType::sync;

	/// The remote endpoint this socket is connected to. Irrelevant if the
	/// socket status isn't `SocketStatus::ready`
	Endpoint _remote;


	// MARK: - Emission

	asio::deadline_timer _timer = asio::deadline_timer(Engine::instance()->getContext());

	/// Mutex protecting from send errors
	std::mutex _sendMutex;

	/// Mutex protecting from receive errors
	std::mutex _receiveMutex;

	/// Synchronous send output buffer
	asio::streambuf _outputBuffer;

	/// Synchronous send output stream
	std::ostream _outputStream;

	/// Send a message to the server synchronously.
	/// @param message The message to send
	void sendSync(const protobuf::Message * message);

	/// Send a message to the server asynchronously
	void sendAsync(const protobuf::Message * message);

protected:

	/// Format the given message in the format defined by `getFormat()` and put it in the given `std::ostream`;
	/// @param message The message to format
	/// @param _outputStream The receiving stream
	void formatMessageToStream(const protobuf::Message * message, std::ostream & _outputStream);

	virtual protobuf::Message * decodeMessageFromBuffer(boost::array<char, RECEPTION_BUFFER_SIZE> & buffer, const std::size_t &bytes_transferred) = 0;

	virtual protobuf::Message * decodeMessageFromBuffer(boost::asio::streambuf *, const std::size_t &bytes_transferred) = 0;

	/// Called everytime a valid datagram is received
	virtual void onReceive(protobuf::Message * message) = 0;

private:

	// MARK: Timing

	inline void startTimer() {
		_timer.expires_from_now(boost::posix_time::seconds(2));
		_timer.async_wait([&] (const boost::system::error_code &error) {
			// Operation aborted is send if the timer is cancelled, meaning no timeout1
			if(error != boost::asio::error::operation_aborted) {
				LOG_ERROR("Socket send timeout");
				close();
			}
		});
	}

	inline void endTimer() {
		_timer.cancel();
	}


	// MARK: - Reception

	/// The reception buffer holding incoming informations
	boost::array<char, RECEPTION_BUFFER_SIZE> _receptionBuffer;

	boost::asio::streambuf _receptionStreamBuffer;

	/// Prepare the connection to receive new datagram
	void prepareReceive();

	/// Handles received data from the network
	void handleReceive(const boost::system::error_code &error, std::size_t bytes_transferred);
};

} /* ::network */

#endif /* BaseSocket_hpp */
