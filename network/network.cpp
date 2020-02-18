//
//  network.cpp
//  network
//
//  Created by Valentin Dufois on 2020-02-18.
//  Copyright © 2020 Perihelion. All rights reserved.
//

#include <iostream>
#include "network.hpp"
#include "networkPriv.hpp"

void network::HelloWorld(const char * s)
{
    networkPriv *theObj = new networkPriv;
    theObj->HelloWorldPriv(s);
    delete theObj;
};

void networkPriv::HelloWorldPriv(const char * s) 
{
    std::cout << s << std::endl;
};

