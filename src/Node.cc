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

int Node::sender = 0;

void Node::initialize()
{
    // TODO - Generated method body
    nodeID = getIndex();
}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    // check first if the message is sent from the coordinator
    std::string receivedMessage = msg->getName();
    std::string keyWord = "coordinator";

    if(receivedMessage.find(keyWord) != std::string::npos) {
        // message is from the coordinator then this node should start
        // assign the starting node from the coordinator to the sender
        // this will determine which node is the sender and which is the receriver for this session
        sender = int(receivedMessage[receivedMessage.length()-1]) - 48;
        EV << "Sender: " << Node::sender << endl;
    }

    if(nodeID == sender){
        // then this is the node that is supposed to send
        // we will call the sending protocol function -> handle the message as the sender
        sendingProtocol();
    } else {
        // then this code is executed for the receiving node
        // call the receiving protocol function -> handle the message as the receiver
        Message *receivedMessage = dynamic_cast<Message*>(msg);
        receivingProtocol(receivedMessage);
    }

    // output log to file and cancel and delete any messages in the system
    this->writeFile();
    cancelAndDelete(msg);

}

// ---------------------------------- MAIN FUNCTIONS ---------------------------------- //

// this function handles the algorithm on the sending side
void Node::sendingProtocol(){

    // TODO: implement the sending protocol
    // 1. read the correct file based on the nodeID
    // 2. loop on all messages read from the file and apply the following steps:
    // 3. apply byte stuffing to the payload (characters)
    // 4. calculate the sequence number and add it as the header
    // 5. calculate the checksum and add it as the parity byte to the trailer
    // 6. check the error code and handle error accordingly -> need to know how exactly
    // don't forget to update the current sequence number and check the window size

    // declarations needed throughout the function
    int sequenceNumber = 0; // the sequence number of the frame max = WS - 1 then reset
    std::bitset<8> parityByte;
    int parity;

    // 1. read the input file
    this->readFile("input" + std::to_string(nodeID) + ".txt");

    // 2. loop on all the messages and apply the algorithm steps
    for(int i = 0; i < messagesToSend.size(); i++){

        // declare a new message
        Message *currentMessage = new Message("CurrentMessage");

        // 2. apply framing using byte stuffing
        std::string framedMessage;
        framedMessage = this->frameMessage(messagesToSend[i]);
//        EV <<"Framed Message: "<< framedMessage << endl;

        // set the message payload to the framed message
        currentMessage->setPayload(framedMessage);

        // 3. calculate the sequence number and add it to the header
        sequenceNumber = i % getParentModule()->par("WS").intValue();
        currentMessage->setHeader(sequenceNumber);
//        EV << "Sequence Number: " << sequenceNumber << endl;

        // 4. calculate the checksum and add it to message trailer
        parityByte = this->calculateChecksum(framedMessage);
        parity = (int)(parityByte.to_ulong());
//        EV << "Parity number "<< parity << endl;
        currentMessage->setTrailer(parity);

        send(currentMessage, "port$o");

        // for testing purposes
//        break;
    }

}

void Node::receivingProtocol(Message *msg){
    // TODO: implement the receiving protocol
    // 1. for every received frame
    EV << msg->getPayload() << endl;
    int ack = msg->getHeader();

}

// ---------------------------------- FILE FUNCTIONS ---------------------------------- //
// read the messages from the given file
void Node::readFile(std::string fileName){
    // TODO: separate the error code from the message

    std::ifstream inputFile;
    inputFile.open("../inputs/" + fileName);

    std::string message;
    std::string errorCode;

    if(inputFile.is_open()){
        while (!inputFile.eof()){
            errorCode = "";
            for(int i = 0; i < 4; i++){
                errorCode += inputFile.get();
            }
            inputFile.get();
            std::getline(inputFile, message);
            messagesToSend.push_back(message);
            errorCodes.push_back(errorCode);

            EV << errorCode << endl;
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

// ---------------------------------- PROTOCOL FUNCTIONS ---------------------------------- //
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

// TODO: given a string (message) calculate its checksum
// this will be used as the parity byte to be added to the message trailer
std::bitset<8> Node::calculateChecksum(std::string message){

    // declarations
    std::vector<std::bitset<8> > sums;
    std::bitset<8> zero(0);
    sums.push_back(zero);

    // first convert the message into binary vector
    std::vector<std::bitset<8> > binaryMessage = this->convertToBinary(message);
//    sums.push_back(binaryMessage[0]);
    int counter = 0;
    for (std::bitset<8> byte : binaryMessage){
        sums.push_back(this->byteAdder(sums[counter], byte));
        counter++;
    }


    // get the one's complement
    std::bitset<8> checksum(sums[sums.size()-1]);
    checksum = checksum.flip();
//    EV << "Checksum: " << checksum << endl;
    return checksum;
}


// ---------------------------------- UTILITY FUNCTIONS ---------------------------------- //
// TODO: given a string (message) return a vector of its binary representation
std::vector<std::bitset<8> > Node::convertToBinary(std::string String){
    std::vector<std::bitset<8> > binaryMessage;

    for(int i = 0; i < String.length(); i++){
        char character = String[i];
        std::bitset<8> asciiBitset(character);
        binaryMessage.push_back(asciiBitset);
//        EV << asciiBitset << endl;
    }

    return binaryMessage;
}

bool Node::fullAdder(bool firstBit, bool secondBit, bool& carry){
//    EV << "previous carry: " << carry;
    bool sum = firstBit ^ secondBit ^ carry;
    carry = (firstBit & secondBit) | ((firstBit ^ secondBit) & carry);
//    EV << " FB:" <<firstBit << " SB:" << secondBit << " bitSum: " << sum << " bitCarry: " << carry << endl;
    return sum;
}

std::bitset<8> Node::byteAdder(std::bitset<8> firstByte, std::bitset<8> secondByte){

//    EV << "1: " << firstByte << " 2: " << secondByte << endl;
//    EV << firstByte[1] << " " << secondByte[1];
    std::bitset<8> byteSum(0);
    bool carry = false;
    for(int i = 0; i < 8; i++){
//        EV << i << " ";
        byteSum[i] = this->fullAdder(firstByte[i], secondByte[i], carry);
//        EV << "byteSum carry: " << carry << endl;
    }
//    EV << "Sum: " << byteSum << " Carry: " << carry << endl;
//    overflowSum[0] = carry;
//    std::bitset<8> byteSum;
    // handle overflow
    // if final carry is 1 then increment the current byte
    if (carry == true){
        for(int i = 0; i < 8; i++){
            if(byteSum[i] == false){
                byteSum[i] = ~byteSum[i];
                break;
            } else {
                byteSum[i] = ~byteSum[i];
            }
        }
    }

//    EV << "Byte Sum: " << byteSum << endl;

    return byteSum;
}

// prints a given vector
void Node::printVector(std::vector<std::bitset<8> > Vector){
    for(int i = 0; i < Vector.size(); i++){
        EV << Vector[i] << endl;
    }
}
