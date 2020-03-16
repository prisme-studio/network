//
//  BaseServer.hpp
//  Protocol Bernardo
//
//  Created by Valentin Dufois on 2020-02-05.
//

#ifndef BaseServer_hpp
#define BaseServer_hpp

#include "../Socket/SocketStatus.hpp"
#include "../Socket/SocketDelegate.hpp"

#include "../Discovery/Advertiser.hpp"

namespace asio = boost::asio;
namespace protobuf = google::protobuf;

namespace network {

// Forward Declaration
class BaseSocket;
class ServerDelegate;

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
class BaseServer: public SocketDelegate {
public:

	// MARK: - Lifecycle

	/// Creates the server for the specified type.
	/// @param aType The type of service this server represent
	BaseServer(const NetworkPort &port,
			   const NetworkPort &discoveryPort = 0,
			   const Endpoint::Type &aType = "undefined",
			   const std::string &interface = "");

	void open();

	/// Sends the given message to all the connected sockets
	/// @param aMessage A message to send
	void sendToAll(protobuf::Message * aMessage);

	/// Start the advertiser, exposing explicitely the server on the network
	inline void advertise() { _advertiser.startAdvertising(); }

	/// Stops the advertiser
	inline void endAdvertising() { _advertiser.stopAdvertising(); }

protected:

	virtual void socketDidOpen(BaseSocket * socket) override;

	virtual void socketDidClose(BaseSocket * socket) override;

	virtual void socketDidSendAsynchronously(BaseSocket *, const protobuf::Message *) override;

public:

	~BaseServer();

	// MARK: - Properties

	ServerDelegate * delegate;

	/// Tell if the server is running
	/// @return True if running, false otherwise
	inline bool isRunning() { return _isRunning; }

	inline SocketFormat getEmissionFormat() { return _emissionFormat; }

	inline void setEmissionFormat(const SocketFormat &aFormat) {
		_emissionFormat = aFormat;
	}

protected:

	// The type of the server
	const Endpoint::Type _type;

	// The port of the server
	int _port;

	inline unsigned long socketsCount() { return _connections.size(); }

	virtual BaseSocket * makeSocket() = 0;

private:

	/// True if the server is opened and running, false otherwise
	bool _isRunning = false;

	SocketFormat _emissionFormat = SocketFormat::protobuf;

	unsigned short _sendCount = 0;

	/// The acceptor used to accept incoming connections
	asio::ip::tcp::acceptor * _acceptor = nullptr;

	/// Ready the server to accept a new connection
	void prepareAccept();

	/// Handls connection acception logic, and dispatch the new connection
	void handleAccept(BaseSocket * newConnection, const boost::system::error_code &error);

	/// Holds a reference to all the connection to this server
	std::vector<BaseSocket *> _connections;

	/// The underlying advertiser used to adveretise this server on the network
	Advertiser _advertiser;
};

} /* ::network */

#endif /* BaseServer_hpp */
