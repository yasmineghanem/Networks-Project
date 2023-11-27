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

#include "Node.h"

Define_Module(Node);

void Node::initialize()
{
    // TODO - Generated method body
}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body

    // check first if the message is sent from the coordinator
    std::string receivedMessage = msg->getName();
    std::string keyWord = "coordinator";

    if(receivedMessage.find(keyWord) != std::string::npos) {
        // message is from the coordinator then this node should start
        // assign the nodeID from the coordinator to the nodeID member
        this->nodeID = int(receivedMessage[receivedMessage.length()-1]) - 48;
        EV << nodeID << endl;
    }

    // read the correct file based on the nodeID sent by the coordinator
    if(nodeID == 0){
        this->readFile("input0.txt");
    } else if(nodeID == 1){
        this->readFile("input1.txt");
    }

}

// read the messages from the given file
void Node::readFile(std::string fileName){
    std::ifstream inputFile;
    inputFile.open("../inputs/" + fileName);

    std::string message;

    if(inputFile.is_open()){
        while (!inputFile.eof()){
            std::getline(inputFile, message);
            messagesToSend.push_back(message);
            EV << message << endl;
        }
    }

}
