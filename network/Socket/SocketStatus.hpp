//
//  SocketStatus.hpp
//  network
//
//  Created by Valentin Dufois on 2019-09-29.
//

#ifndef SocketStatus_h
#define SocketStatus_h

namespace network {

/// Defines status a `Socket` can have in its lifetime
enum SocketStatus {
	idle,
	connecting,
	ready,
	closed
};

/// Defines available mechanisms for sending on a `Socket`
enum EmissionType {
	sync,
	async
};

/// Definese available formats when sending and receiving on a `Socket`
enum SocketFormat {
	protobuf,
	json
};

} /* ::network */

#endif /* SocketStatus_h */
