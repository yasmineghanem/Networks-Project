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
    if (coordinatorFile.is_open())
    {

        // read the value of the starting node
        nodeID = int(coordinatorFile.get()) - 48;
        EV << nodeID << endl;

        // read the value of the starting time
        std::string line;
        std::getline(coordinatorFile, line);

        // convert starting time to float
        float startingTime = std::stof(line);
        EV << startingTime << endl;

        CustomMessage_Base *coordinator = new CustomMessage_Base("Coordinator");
        coordinator->setFrameType(-1);
        coordinator->setHeader(nodeID);

        // Schedule a self message to send a message to the starting node to start at the specified starting time
        scheduleAt(simTime() + startingTime, coordinator);
    }
}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body

    // Send a message to the starting node to start sending
    send(msg, "outs", nodeID);
}
