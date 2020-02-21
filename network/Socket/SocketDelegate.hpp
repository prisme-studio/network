//
//  SocketDelegate.hpp
//  network
//
//  Created by Valentin Dufois on 2019-11-15.
//

#ifndef SocketDelegate_h
#define SocketDelegate_h

namespace google {
namespace protobuf {
class Message;
}
}

namespace network {

class BaseSocket;

class SocketDelegate {
public:
	/// Called when the socket did connect to its remote
	virtual void socketDidOpen(BaseSocket *) {}

	/// Called everytime the socket received a datagram from the
	/// network. Some datagram with Socket-specific types, such as
	/// 'close' might not be propagated to this method.
	virtual void socketDidReceive(BaseSocket *, google::protobuf::Message *) {}

	/// Called when the socket disconnects/closes
	///
	/// Once the socket is closed, a new one should be used to
	/// re-open the connection if needed
	///
	/// Calling the destructor on the socket will not trigger this
	/// callback
	virtual void socketDidClose(BaseSocket *) {}
};

} /* ::network */

#endif /* SocketDelegate_h */
