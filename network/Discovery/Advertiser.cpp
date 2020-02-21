//
//  Advertiser.cpp
//  NetworkEngine
//
//  Created by Valentin Dufois on 2019-09-09.
//  Copyright © 2019 Prisme. All rights reserved.
//

#include <boost/bind.hpp>
#include <iostream>
#include <string>

#include <common/log.hpp>

#include "Advertiser.hpp"

#include "../Engine.hpp"
#include "../Messages/network.pb.h"

namespace network {

Advertiser::Advertiser(const NetworkPort &port, const std::string &interface):
_interface(interface),
_broadcastEndpoint(asio::ip::udp::endpoint(asio::ip::address_v4::broadcast(), port)) {
	Endpoint thisMachine = Engine::thisMachine();
	thisMachine.port = port;

	// Prepare the message
	messages::Endpoint * message = thisMachine;

	// Build and store the message buffer
	std::ostream os(&_outputBuffer);
	message->SerializeToOstream(&os);

	delete message;
}

void Advertiser::startAdvertising() {
	if(_isRunning)
		return;

	// Open the network connection (Broadcast)
	_socket = new asio::ip::udp::socket(Engine::instance()->getContext());
	_socket->open(asio::ip::udp::v4());

	asio::ip::address outboundAddress = getOutboundInterfaceIP();

	LOG_INFO("Advertising on interface " + outboundAddress.to_string() + ":" + std::to_string(_broadcastEndpoint.port()));

	_socket->set_option(asio::ip::multicast::outbound_interface(outboundAddress.to_v4()));
	_socket->set_option(asio::ip::udp::socket::reuse_address(true));
	_socket->set_option(asio::socket_base::broadcast(true));
	_socket->bind(asio::ip::udp::endpoint(outboundAddress, 0));

	// Set up the timer
	setTimer();
    
	// Run the context
	Engine::instance()->runContext();

	_isRunning = true;
}

void Advertiser::advertise(const boost::system::error_code &e) {
	if(!_isRunning)
		return;

	boost::system::error_code error;
	_socket->send_to(_outputBuffer.data(), _broadcastEndpoint, boost::asio::socket_base::message_flags(), error);

	if(error) {
		LOG_ERROR("Error while advertising, stopping.");
		LOG_ERROR(error.message());
		stopAdvertising();
		return;
	}

	// Reset the timer
	setTimer();
}

void Advertiser::stopAdvertising() {
	if(!_isRunning)
		return;

	// Stop and erase timer
	_timer->cancel();
	delete _timer;
	_timer = nullptr;

	// Close the socket
	_socket->close();

	_isRunning = false;

	LOG_INFO("Advertising on port " +  std::to_string(_broadcastEndpoint.port()));
}

void Advertiser::setTimer() {
	if(!_isRunning) {
		// Create the timer
		_timer = new asio::deadline_timer(Engine::instance()->getContext());
	}

	// Set up the timer
	_timer->expires_from_now(boost::posix_time::seconds(advertiserRate));
	_timer->async_wait(boost::bind(&Advertiser::advertise, this, boost::asio::placeholders::error));
	Engine::instance()->runContext();
}

asio::ip::address Advertiser::getOutboundInterfaceIP() {
	// The user has specified a specific outbound interface
	if(_interface.size() > 0) {
		return asio::ip::make_address_v4(_interface);
	}

	std::vector<asio::ip::address> interfaces = Engine::instance()->getOutboundInterfaces();

	if(interfaces.size() == 0) {
		return asio::ip::make_address_v4("127.0.0.1");
	}

	if(interfaces.size() == 1) {
		return interfaces[0];
	}

	// Muliple network interfaces are available
	// Request user input for selecting the network interface to use
	LOG_WARN("Found " + std::to_string(interfaces.size()) + " ipv4 network interface.s");
	LOG_INFO("Please select interface to use for advertising on port:" + std::to_string(_broadcastEndpoint.port()));

	std::string queryInteraces = "";
	for(unsigned int i = 0; i < interfaces.size(); ++i) {
		queryInteraces += "(" + std::to_string(i) + ") " + interfaces[i].to_string() + "  ";
	}

	LOG_INFO(queryInteraces);
	std::cout << ">> ";
	std::string selectedInterface;
	std::cin >> selectedInterface;

	return interfaces[std::atoi(selectedInterface.c_str())];
}

Advertiser::~Advertiser() {
	// Properly close everything if needed
	stopAdvertising();

	// Free used memory
	delete _socket;
}

} /* ::network */
