//
//  Endpoint.hpp
//  Protocol Bernardo
//
//  Created by Valentin Dufois on 2019-09-09.
//  Copyright © 2019 Prisme. All rights reserved.
//

#ifndef Endpoint_hpp
#define Endpoint_hpp

#include <string>
#include <boost/asio.hpp>

#include "network.hpp"
#include "Messages/network.pb.h"

namespace network {

/**
 An Endpoint represent a machine on the network
 */
struct Endpoint {
	/** The IP of the endpoint. For the current machine, this will always be `127.0.0.1` */
	std::string ip;

	/** The name of the machine, as defined by the network interface */
	std::string name;

	using Type = std::string;

	/** The role of the machine (for display only)*/
	Type type = "UNDEFINED";

	/// Port on which this endpoint is accessible
	NetworkPort port = 0;

	NetworkPort discoveryPort = 0;


	// MARK: - Constructors

	/// Default constructor
	Endpoint() = default;

	/// Construct an Endpoint using the given information
	///
	/// @param aType Type of the endpoint
	Endpoint(const Type &aType);

	/// Construct an Endpoint using the given information
	///
	/// @param aIP IP of the endpoint
	/// @param aPort Port of the endpoint
	Endpoint(const std::string &aIP, const unsigned int &aPort);

	/// Construct an Endpoint using the given information
	///
	/// @param aIP IP of the endpoint
	/// @param aName Name of the endpoint
	/// @param aType type of the endpoint
	Endpoint(const std::string &aIP, const std::string &aName, const Type &aType);

	/// Construct an Endpoint from the given message.
	/// There will not be any IP on this endpoint
	///
	/// @param message an Endpoint message to decode
	Endpoint(const messages::Endpoint message);

	/// Construct an Endpoint from the given asio endpoint
	///
	/// @param endpoint An asio::endpoint
	Endpoint(const boost::asio::ip::tcp::endpoint &endpoint);

	/// Construct an endpoint from the given message and using the
	/// given asio::endpoint for the IP.
	Endpoint(const messages::Endpoint message, const boost::asio::ip::udp::endpoint &endpoint);


	// MARK: - Methods

private:

	/// Gives the default port to connect to this endpoint
	NetworkPort getPort();

	/// Gives the port on which to listen to find this endpoint
	NetworkPort getDiscoveryPort();

public:

	/// Gives the uri (ip + port) for the current endpoint.
	inline std::string uri() const {
		return ip + ":" + std::to_string(port);
	}


	// MARK: - Operators

	/// Casts the endpoint to a message.
	/// - warning: the given message needs to be freed before being released
	operator messages::Endpoint * () const {
		messages::Endpoint * message = new messages::Endpoint;

		// Fill in the message
		message->set_name(name);
		message->set_type(type);

		return message;
	}

	/// Casts the endpoint as a TCP endpoint
	operator boost::asio::ip::tcp::endpoint () const {
		return boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port);
	}


	/// Tell if two endpoints are the same machine
	bool operator == (const Endpoint &b) const {
		return b.name == name && b.ip == ip && b.type == type;
	}

	/// Tell if two endoints are not the same machine
	bool operator != (const Endpoint &b) const {
		return !(*this == b);
	}
};

} /* ::network */

#endif /* Endpoint_hpp */
