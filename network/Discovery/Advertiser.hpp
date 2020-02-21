//
//  Advertiser.hpp
//  NetworkEngine
//
//  Created by Valentin Dufois on 2019-09-09.
//  Copyright © 2019 Prisme. All rights reserved.
//

#ifndef Advertiser_hpp
#define Advertiser_hpp

#include <thread>

#include <boost/asio.hpp>

#include "../network.hpp"
#include "../Endpoint.hpp"

namespace asio = boost::asio;

namespace network {

/// An advertiser is used to advertise the presence of a machine on the network.
/// It broadcasts the machine name and type regularly on the network for other machines to get.
class Advertiser {
public:

	Advertiser(const NetworkPort &port, const std::string &interface = "");

	/// Start advertising on the network
	virtual void startAdvertising() final;

	/// Stop advertising on the network
	virtual void stopAdvertising() final;

	~Advertiser();

protected:

	/// Tell if the advertiser is running
	bool _isRunning = false;

	/// Set up/Reset the timer used to fire advertisement broadcasting
	void setTimer();

	std::string _interface;

	/// Provides the nework interface to use
	asio::ip::address getOutboundInterfaceIP();

	/// Effectively emits the advertisement on the network
	/// Method called regularly by the timer
	void advertise(const boost::system::error_code &e);

	/// The buffer holding data about this machine and being emitted on the network regularly
	asio::streambuf _outputBuffer;

	// MARK: - Network

	/// The broadcast endpoint
	asio::ip::udp::endpoint _broadcastEndpoint;

	/// The socket on which we are emitting
	asio::ip::udp::socket * _socket = nullptr;

	/// The timer
	asio::deadline_timer * _timer = nullptr;
};

} /* ::network */

#endif /* Advertiser_hpp */
