//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "Coordinator.h"

Define_Module(Coordinator);

void Coordinator::initialize()
{
    // TODO - Generated method body

    // Open the coordinator file and get the starting info
    std::ifstream coordinatorFile;
    coordinatorFile.open("../inputs/coordinator.txt");
    if (coordinatorFile.is_open()){

        // Read the starting node
        nodeNumber = int(coordinatorFile.get()) - 48;
        EV << nodeNumber << endl;

        // Read the starting time
        std::string line;
        std::getline(coordinatorFile, line);

        // Convert starting time to float
        float startingTime = std::stof(line);
        EV << startingTime << endl;

        // Schedule a self message to send a message to the starting node
        scheduleAt(simTime() + startingTime, new cMessage(""));
    }
}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    // Send a message to the starting node to start sending
    send(new cMessage("Hey from coordinator"), "outs", nodeNumber);
}
