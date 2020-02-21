//
//  Engine.cpp
//  Engine
//
//  Created by Valentin Dufois on 2019-09-09.
//  Copyright © 2019 Prisme. All rights reserved.
//

#ifdef BOOST_ASIO_SEPARATE_COMPILATION
#include <boost/asio/impl/src.hpp>
#endif

#include "Engine.hpp"
#include <common/thread.hpp>

#ifndef _WIN32
#include <pthread.h>
#endif

// MARK: - Static informations

namespace network {

Endpoint::Type Engine::thisMachineType = "undefined";

Engine * Engine::_instance = nullptr;

void Engine::runContext() {
	if(_executionThread != nullptr && _executionThread->joinable()) {
		// We have a thread, meaning the context is running, do nothing
		return;
	}

	// No thread, create it and run it
	_executionThread = new std::thread([&] () {
		thread::setName("network-engine");

		_ioContext.restart();
		_ioContext.run();

		LOG_DEBUG("Detaching network engine thread");
		_executionThread->detach();
	});
}

std::vector<asio::ip::address> Engine::getOutboundInterfaces() {
	// Find the outbound interfaces
	asio::ip::udp::resolver resolver(Engine::instance()->getContext());
	asio::ip::udp::resolver::query query(asio::ip::host_name(), "");
	asio::ip::udp::resolver::iterator it = resolver.resolve(query);

	asio::ip::address interfaceIPAdress;

	std::vector<asio::ip::address> interfaces;

	// Select the first IPv4 interface and use it
	while(it != boost::asio::ip::udp::resolver::iterator()) {
		asio::ip::address interface = (it++)->endpoint().address();
		if(interface.is_v4()) {
			// Ignore loopback
			if(interface.to_string() == "127.0.0.1")
				continue;

			interfaces.push_back(interface);
		}
	}

	return interfaces;
}


} /* ::network */
