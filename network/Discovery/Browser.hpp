//
//  Browser.hpp
//  network
//
//  Created by Valentin Dufois on 2019-09-11.
//  Copyright © 2019 Prisme. All rights reserved.
//

#ifndef Browser_hpp
#define Browser_hpp

#include <iostream>
#include <thread>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "../network.hpp"
#include "../Endpoint.hpp"

namespace asio = boost::asio;

namespace network {

/// A Browser is able to detect any Advertising currenlty advertising on the network.
/// Everytime an advertisment is received, the onReceive method will be called. The method may be called for the same endpoint multiple times
class Browser {
public:

	Browser() = default;

	/// Called everytime a message is received from the network
	/// This is where you set up your callback method
	std::function<void(const Endpoint &)> onReceive;

	/// Make the browser attentive to the network, and ready to receive messages from
	/// other advertisers on the network
	virtual void startBrowsing(const NetworkPort &port) final;

	/// End listening to the network.
	virtual void stopBrowsing() final;

	~Browser();

private:

	/// Tell if the browser is running or not
	bool _isRunning = false;

	/// Called eeverytime a message is received from the network
	void handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred);

	/// The socket listning on the network
	asio::ip::udp::socket * _socket = nullptr;

	/// The reception buffer holding incoming messages
	boost::array<char, 1000> _receptionBuffer;

	/// Incoming messages are stored here
	messages::Endpoint _receptionMessage;

	/// Informations about the machine emmiting the message
	asio::ip::udp::endpoint _anyEndpoint;
};

} /* network */

#endif /* Browser_hpp */
