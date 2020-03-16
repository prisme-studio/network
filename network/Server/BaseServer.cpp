//
//  BaseServer.cpp
//  Protocol Bernardo
//
//  Created by Valentin Dufois on 2020-02-05.
//

#include <boost/bind.hpp>
#include <common/log.hpp>

#include "BaseServer.hpp"
#include "ServerDelegate.hpp"

#include "../Socket/BaseSocket.hpp"
#include "../Engine.hpp"
#include "../Endpoint.hpp"

namespace network {

BaseServer::BaseServer(const NetworkPort &port, const NetworkPort &discoveryPort, const Endpoint::Type &aType, const std::string &interface):
_type(aType),
_port(port),
_advertiser(discoveryPort, interface) {
	// Build the acceptor
	_acceptor = new asio::ip::tcp::acceptor(Engine::instance()->getContext(), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), _port));
}

void BaseServer::open() {
	if(_isRunning)
		return;

	// Run the context
	Engine::instance()->runContext();

	_isRunning = true;

	// Be ready to accept new connections
	prepareAccept();

	LOG_INFO(Endpoint(_type).type + " Server opened on port " + std::to_string(_port));
}

void BaseServer::sendToAll(protobuf::Message * aMessage) {
	_sendCount = _connections.size();
	for(BaseSocket * s: _connections) {
		s->send(aMessage);
	}
}

void BaseServer::socketDidOpen(BaseSocket * socket) { }

void BaseServer::socketDidClose(BaseSocket * socket) {
	if(!_isRunning)
		return;

	// The socket is closed, remove it from the array of connections
	_connections.erase(std::find(_connections.begin(), _connections.end(), socket));

	delete socket;
}
void BaseServer::socketDidSendAsynchronously(BaseSocket *, const protobuf::Message * message) {
	if(--_sendCount > 0)
		return;

	// send count is empty, tell delegate
	if(delegate)
		delegate->serverDidSendToAll(this, message);
}

void BaseServer::prepareAccept() {
	BaseSocket * newConnection = makeSocket();
	newConnection->delegate = this;

	_acceptor->async_accept(newConnection->getSocket(), boost::bind(&BaseServer::handleAccept, this, newConnection, boost::asio::placeholders::error));
}

void BaseServer::handleAccept(BaseSocket * newConnection, const boost::system::error_code &error) {
	// Was there an error during connection ?
	if(error) {
		if(error != asio::error::operation_aborted) {
			LOG_WARN("An error occured while accepting a connection");
			LOG_WARN(error.message());
		}
		return;
	}

	// Store the new connection
	_connections.push_back(newConnection);

	newConnection->onOpenedFromRemote(_type);

	prepareAccept();
}

BaseServer::~BaseServer() {
	// Perform stopping actions...
	_isRunning = false;

	_acceptor->cancel();
	_acceptor->close();
	delete _acceptor;

	for(BaseSocket * socket: _connections) {
		if(socket != nullptr)
			delete socket;
	}

	_connections.clear();

	LOG_INFO(Endpoint(_type).type + " Server using port " + std::to_string(_port) + " closed");
}

} /* ::network */
