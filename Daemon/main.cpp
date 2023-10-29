//
//  main.cpp
//  Daemon
//
//  Created by Alexandra on 26.10.2023.
//

#include <iostream>
#include "Daemon.hpp"

int main() {
    Daemon& daemon = Daemon::getInstance();
    daemon._start();
    return 0;
}
