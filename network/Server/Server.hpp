//
//  Server.hpp
//  network
//
//  Created by Valentin Dufois on 2019-09-25.
//

#ifndef Server_hpp
#define Server_hpp

#include "BaseServer.hpp"
#include "../Socket/Socket.hpp"

namespace asio = boost::asio;

namespace network {

/// A Server allows for building services and making them accessible to others on the
/// network.
/// Multiple Server should be able to run concurrently on different ports (not tested).
///
/// The server class handles all the connection logic for you, exposing only the
/// necessary handles to build your services. The server class is abstract and made
/// to be subclassed.
///
/// Data exchange logic with clients happens at the connection level and not at the
/// server level.
///
/// The Server class can advertise itself on the network using a built in Advertiser.
template<class MessageFormat = messages::Datagram>
class Server: public BaseServer {
public:

	// MARK: - Lifecycle

	/// Creates the server for the specified type.
	/// @param aType The type of service this server represent
	Server(const NetworkPort &port,
		   const NetworkPort &discoveryPort = 0,
		   const Endpoint::Type &aType = "undefined",
		   const std::string &interface = ""): BaseServer(port, discoveryPort, aType, interface) {};

	inline SocketFormat getEmissionFormat() { return _emissionFormat; }

	inline void setEmissionFormat(const SocketFormat &aFormat) {
		_emissionFormat = aFormat;
	}


protected:

	inline BaseSocket * makeSocket() override {
		Socket<MessageFormat> * newSocket = new Socket<MessageFormat>();
		newSocket->setFormat(_emissionFormat);

		return newSocket;
	}

private:

	SocketFormat _emissionFormat = SocketFormat::protobuf;
};

} /* ::network */

#endif /* Server_hpp */
