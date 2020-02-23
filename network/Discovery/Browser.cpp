//
//  Browser.cpp
//  NetworkEngine
//
//  Created by Valentin Dufois on 2019-09-11.
//  Copyright © 2019 Prisme. All rights reserved.
//

#include <boost/bind.hpp>

#include "Browser.hpp"

#include "../Engine.hpp"
#include "../Messages/network.pb.h"
#include <common/log.hpp>

namespace network {

void Browser::startBrowsing(const NetworkPort &port) {
	if(_isRunning)
		return;

	_anyEndpoint = asio::ip::udp::endpoint(asio::ip::address_v4::any(), port);

	// Create the socket
	_socket = new asio::ip::udp::socket(Engine::instance()->getContext());

	_socket->open(asio::ip::udp::v4());
	_socket->set_option(asio::socket_base::broadcast(true));
	_socket->set_option(asio::socket_base::reuse_address(true));

	// Open the socket
	boost::system::error_code error;
	_socket->bind(_anyEndpoint, error);

	if(error) {
		LOG_ERROR("Could not start browser on port " + std::to_string(port) + ": " + error.message());
		return;
	}

	// Bind the handler
	_socket->async_receive_from(asio::buffer(_receptionBuffer), _anyEndpoint, [&] (const boost::system::error_code &error, std::size_t bytes_transferred) {
		handleReceive(error, bytes_transferred);
	});

	// Run the thread on the background
	Engine::instance()->runContext();

	_isRunning = true;

	LOG_INFO("Browsing on port " + std::to_string(_anyEndpoint.port()));
}

void Browser::handleReceive(const boost::system::error_code &error, std::size_t bytes_transferred) {

	if(error) {
		if(!_isRunning)
			return;
		
		// Do nothing on error and prepare for receiving again
		// Prepare for another receive
		_socket->async_receive_from(asio::buffer(_receptionBuffer), _anyEndpoint, [&] (const boost::system::error_code &error, std::size_t bytes_transferred) {
			handleReceive(error, bytes_transferred);
		});

		// Run the thread on the background
		Engine::instance()->runContext();
	}

	// Get message as Endpoint
	_receptionMessage.ParseFromArray(_receptionBuffer.data(), (int)bytes_transferred);
	Endpoint endpoint = Endpoint(_receptionMessage, _anyEndpoint);

	// Clear the buffers
	_receptionMessage.Clear();
	_receptionBuffer.assign(0);

	// Propagate to handler
	if(onReceive)
		onReceive(endpoint);

	// Are we still running ?
	if(!_isRunning)
		return;

	// Prepare for another receive
	_socket->async_receive_from(asio::buffer(_receptionBuffer), _anyEndpoint, [&] (const boost::system::error_code &error, std::size_t bytes_transferred) {
		handleReceive(error, bytes_transferred);
	});

	// Run the thread on the background
	Engine::instance()->runContext();
}

void Browser::stopBrowsing() {
	if(!_isRunning)
		return;

	_isRunning = false;

	// Close the socket
	_socket->close();
	delete _socket;
	_socket = nullptr;

	LOG_INFO("Stopped browsing on port " + std::to_string(_anyEndpoint.port()));
}

Browser::~Browser() {
	// End operations
	stopBrowsing();

	// Free used memory
	delete _socket;
}

} /* network */
