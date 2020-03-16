//
//  ServerDelegate.hpp
//  network
//
//  Created by Valentin Dufois on 2020-03-16.
//  Copyright © 2020 Perihelion. All rights reserved.
//

#ifndef ServerDelegate_h
#define ServerDelegate_h

// MARK: Forward declarations

namespace google {
namespace protobuf {
class Message;
}
}

namespace network {
class BaseServer;
}

// MARK: - ServerDelegate

namespace network {

class ServerDelegate {
public:
	virtual void serverDidSendToAll(BaseServer *, const google::protobuf::Message *) = 0;
};

} /* ::network */

#endif /* ServerDelegate_h */
