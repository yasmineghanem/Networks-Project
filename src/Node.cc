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
    EV << getIndex() << endl;
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

    // Message Framing
    std::string framedMessage;
    framedMessage = this->frameMessage("hello/$");
    EV <<"Framed Message: "<< framedMessage << endl;

    // output log to file and cancel and delete any messages in the system
    this->writeFile();
    cancelAndDelete(msg);

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

// TODO: write this function to write the system outputs as specified in the table in the document
void Node::writeFile(){
    // Create and open a text file
    std::ofstream outputFile("../outputs/output.txt");

    // Write to the file
    outputFile << "output file";

    // Close the file
    outputFile.close();

}

// TODO: given a string (message) return the framed message to be sent
// using the byte stuffing framing method

std::string Node::frameMessage(std::string message){
    std::string framedMessage = "$";

    for(int i = 0; i < message.length(); i++){
        if(message[i] == '/' || message[i] == '$')
            framedMessage += '/';
        framedMessage += message[i];
    }

    framedMessage += '$';
    return framedMessage;
}


// TODO: given a string (message) return a vector of its binary representation
std::vector<std::bitset<8> > Node::convertToBinary(std::string String){
    std::vector<std::bitset<8> > binaryMessage;

    for(int i = 0; i < String.length(); i++){
        char character = String[i];
        std::bitset<8> asciiBitset(character);
        binaryMessage.push_back(asciiBitset);
    }

    return binaryMessage;
}

// prints a given vector
void Node::printVector(std::vector<std::bitset<8> > Vector){
    for(int i = 0; i < Vector.size(); i++){
        EV << Vector[i] << endl;
    }
}
