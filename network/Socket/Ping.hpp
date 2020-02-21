//
//  Ping.hpp
//  network
//
//  Created by Valentin Dufois on 2019-09-30.
//

#ifndef Ping_hpp
#define Ping_hpp

#include <chrono>

#include "../Messages/network.pb.h"

using timeScale = std::chrono::milliseconds;

namespace network {

class BaseSocket;

class Ping {
protected:
	void ping(BaseSocket * socket) {
		long long now = std::chrono::duration_cast<timeScale>(std::chrono::system_clock::now().time_since_epoch()).count();

		messages::Ping ping;
		ping.set_time(now);

		protobuf::Any * data = new protobuf::Any();
		data->PackFrom(ping);

		messages::Datagram * datagram = new messages::Datagram();
		datagram->set_type(datagramType::ping);
		datagram->set_allocated_data(data);

		LOG_DEBUG("Sending a ping to " + socket->getRemote().ip);
		socket->send(datagram);

		datagram->clear_data();
	}

	void onPing(protobuf::Any * data, BaseSocket * socket) {
		// Say we received a ping
		LOG_DEBUG("Relaying a ping");

		// Relay the ping to its sender directly
		messages::Datagram * datagram = new messages::Datagram();
		datagram->set_type(datagramType::pong);
		datagram->set_allocated_data(new protobuf::Any(*data));

		socket->send(datagram);

		datagram->clear_data();
	}

	void onPong(protobuf::Any * data, BaseSocket * socket) {
		long long now = std::chrono::duration_cast<timeScale>(std::chrono::system_clock::now().time_since_epoch()).count();

		messages::Ping pong;
		data->UnpackTo(&pong);

		std::string duration = std::to_string((now - pong.time()));

		LOG_DEBUG("Ping-pong with " + socket->getRemote().ip + " in " + duration + "ms");
	}
};

} /* ::network */

#endif /* Ping_hpp */
