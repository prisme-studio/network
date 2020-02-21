//
//  network.hpp
//  network
//
//  Created by Valentin Dufois on 2020-02-18.
//  Copyright © 2020 Perihelion. All rights reserved.
//

#ifndef network_h
#define network_h

namespace network {

using NetworkPort = unsigned short int;

// MARK: Advertiser
constexpr unsigned short int advertiserRate = 1; // Advertise every X seconds

enum datagramType: unsigned int {
	undefined	= 0,		//
	ping		= 5,		// Ping command
	pong		= 6,		// Ping response
	close		= 9,		// Tell other side the connection is closing
};

} /* ::network */

#endif /* network_h */
