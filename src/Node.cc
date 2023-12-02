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
    startWindow = 0;
    currentIndex = startWindow;
    endWindow = getParentModule()->par("WS").intValue() - 1;
    PT = getParentModule()->par("PT").doubleValue();
    TD = getParentModule()->par("TD").doubleValue();
    WS = getParentModule()->par("WS").intValue();
}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    // check first if the message is sent from the coordinator
    std::string receivedMessage = msg->getName();
    std::string keyWord = "coordinator";

    EV << receivedMessage << endl;

    if (receivedMessage.find(keyWord) != std::string::npos)
    {
        // message is from the coordinator then this node should start
        // assign the starting node from the coordinator to the sender
        // this will determine which node is the sender and which is the receriver for this session
        sender = int(receivedMessage[receivedMessage.length() - 1]) - 48;
        EV << "Sender: " << Node::sender << endl;

        // 1. read the input file
        this->readFile("input" + std::to_string(sender) + ".txt");
        this->start = messagesToSend.begin();
        // EV << *(this->start) << endl;
        this->end = this->start + WS - 1;
        // EV << *(this->end) << endl;
        processDataToSend();
    }

    if (nodeID == sender)
    {
        // then this is the node that is supposed to send
        // we will call the sending protocol function -> handle the message as the sender
        if (msg->isSelfMessage())
        {
            // if the self message sent is because of loss of frame
            if (strcmp(msg->getName(), "LOSS") == 0)
            {
                currentIndex++;
                processDataToSend();
            }
            // send processed message OR send duplicate
            // The the sender should send the message at the scheduled time
            else
            {
                if (currentIndex >= startWindow && currentIndex <= endWindow)
                {
                    send(msg, "port$o");
                    currentIndex++;
                }
            }
        }
        else if (strcmp(msg->getName(), "ACK") == 0)
        {
            Message *receivedAck = dynamic_cast<Message *>(msg);
            receiveAck(receivedAck);
            // EV << "Sender: " << msg->getName() << endl;
            processDataToSend();
        }
        else if (strcmp(msg->getName(), "NACK") == 0)
        {
            // receive the NACK
            EV << msg->getName() << endl;
            Message *receivedNack = dynamic_cast<Message *>(msg);
            receiveNack(receivedNack);
            // process and send the lost data
            processDataToSend();
        }
        EV << "START: " << startWindow << " END: " << endWindow << " CURRENT: " << currentIndex << endl;
    }
    else
    {
        // then this code is executed for the receiving node
        // call the receiving protocol function -> handle the message as the receiver
        if (msg->isSelfMessage())
        {
            // send the processed ACK/NACK
            // EV << "Receiver: " << msg->getName() << endl;
            send(msg, "port$o");
        }
        else
        {
            // either receive the sent frame and process it -> deframe
            Message *receivedMessage = dynamic_cast<Message *>(msg);
            processReceivedData(receivedMessage);
        }
    }

    // output log to file and cancel and delete any messages in the system
    // this->writeFile();
    // cancelAndDelete(msg);
}

// ---------------------------------- MAIN FUNCTIONS ---------------------------------- //

// this function handles the algorithm on the sending side
void Node::processDataToSend()
{

    // TODO: implement the sending protocol
    // 1. read the correct file based on the nodeID
    // 2. loop on all messages read from the file and apply the following steps:
    // 3. apply byte stuffing to the payload (characters)
    // 4. calculate the sequence number and add it as the header
    // 5. calculate the checksum and add it as the parity byte to the trailer
    // 6. check the error code and handle error accordingly -> need to know how exactly
    // don't forget to update the current sequence number and check the window size

    // Error messaeg handling
    bool modification, loss, duplication, delay;

    handleError(errorCodes[currentIndex], modification, loss, duplication, delay);

    // if the loss bit is 1 then the message will not be sent
    if (loss)
    {
        scheduleAt(simTime(), new cMessage("LOSS"));
        return;
    }

    // declarations needed throughout the function
    int sequenceNumber = 0; // the sequence number of the frame max = WS - 1 then reset
    std::bitset<8> parityByte;
    int parity;
    // double PT = getParentModule()->par("PT").doubleValue();
    // double TD = getParentModule()->par("TD").doubleValue();

    // 2. loop on all the messages and apply the algorithm steps
    //    for(int i = 0; i < messagesToSend.size(); i++){

    // declare a new message
    Message *currentMessage = new Message("CurrentMessage");

    // 2. apply framing using byte stuffing
    std::string framedMessage;
    // framedMessage = this->frameMessage(messagesToSend[currentIndex]);
    framedMessage = this->frameMessage(messagesToSend[currentIndex]);
    // EV <<"Framed Message: "<< framedMessage << endl;

    // set the message payload to the framed message
    currentMessage->setPayload(framedMessage);

    // 3. calculate the sequence number and add it to the header
    // sequenceNumber = 0 % WS;
    sequenceNumber = currentIndex % WS;
    currentMessage->setHeader(sequenceNumber);
    //        EV << "Sequence Number: " << sequenceNumber << endl;

    // 4. calculate the checksum and add it to message trailer
    parityByte = this->calculateChecksum(framedMessage);
    parity = (int)(parityByte.to_ulong());
    //        EV << "Parity number "<< parity << endl;
    currentMessage->setTrailer(parity);
    currentMessage->setFrameType(2);

    EV << "Message Sent: " << messagesToSend[currentIndex] << endl;

    // send the processed message after the specified processing time
    scheduleAt(simTime() + PT + TD, currentMessage);
}

void Node::processReceivedData(Message *msg)
{
    // TODO: implement the receiving protocol
    // 1. get the parity and calculate the checksum to detect if there is an error
    // 2. deframe the received message
    // 3. send the ack

    // check that the received message is the one the receiver is witing for
    if (msg->getHeader() != ackNumber) // the received message is the intended message
    {
        // messages where lost in the system
        // send NACK with the message we're waiting for
        // return -> don't process the message
        sendNack();
        return;
    }

    std::string receivedMessage = msg->getPayload();

    // 1. get the parity and calculate the checksum to detect if there is an error
    bool error = detectError(receivedMessage, msg->getTrailer());
    EV << "ERROR " << error << endl;

    if (error)
    {
        // do something
        // send Nack for the errored frame
    }

    // 2. deframe the received message
    std::string originalMessage = deframeMessage(receivedMessage);

    EV << "Message Received: " << originalMessage << endl;

    receivedMessages.push_back(msg);

    ackNumber = (ackNumber + 1) % WS;

    sendAck();
}

void Node::sendAck()
{
    // TODO: send the ack of the received protocol
    Message *ack = new Message("ACK");
    ack->setFrameType(1);
    ack->setAckNack(ackNumber);

    scheduleAt(simTime() + PT + TD, ack);
}

void Node::sendNack()
{
    // TODO: send the ack of the received protocol
    Message *nack = new Message("NACK");
    nack->setFrameType(0);
    nack->setAckNack(ackNumber % WS);
    EV << "NACK NUMBER: " << nack->getAckNack() << endl;
    scheduleAt(simTime() + PT + TD, nack);
}

void Node::receiveAck(Message *msg)
{
    // TODO: receive the ack from the receiver
    EV << "ACK Number: " << msg->getAckNack() << endl;
    startWindow++;
    endWindow++;
    start++;
    end++;
}

void Node::receiveNack(Message *msg)
{
    // TODO: receive the ack from the receiver
    int NACK = msg->getAckNack();
    EV << "NACK Number: " << NACK << endl;
    // adjust the current index and the window start and end inidices
    currentIndex = startWindow;
    while (currentIndex % WS != NACK)
        currentIndex++;
    // adjust the error code of the nack frame -> avoid infinate loop
    errorCodes[currentIndex] = "0000";
}

// ---------------------------------- FILE FUNCTIONS ---------------------------------- //
// read the messages from the given file
void Node::readFile(std::string fileName)
{
    // TODO: separate the error code from the message

    std::ifstream inputFile;
    inputFile.open("../inputs/" + fileName);

    std::string message;
    std::string errorCode;

    if (inputFile.is_open())
    {
        while (!inputFile.eof())
        {
            errorCode = "";
            for (int i = 0; i < 4; i++)
            {
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
void Node::writeFile()
{
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

std::string Node::frameMessage(std::string message)
{
    std::string framedMessage = "$";

    for (int i = 0; i < message.length(); i++)
    {
        if (message[i] == '/' || message[i] == '$')
            framedMessage += '/';
        framedMessage += message[i];
    }

    framedMessage += '$';
    return framedMessage;
}

// TODO: given a string (message) return the framed message to be sent
// using the byte stuffing framing method

std::string Node::deframeMessage(std::string message)
{
    std::string originalMessage = "";

    for (int i = 1; i < message.length() - 1; i++)
    {
        if (message[i] == '/')
        {
            originalMessage += message[i + 1];
            i++;
        }
        else
            originalMessage += message[i];
    }

    return originalMessage;
}

// TODO: given a string (message) calculate its checksum
// this will be used as the parity byte to be added to the message trailer
std::bitset<8> Node::calculateChecksum(std::string message)
{

    // declarations
    std::vector<std::bitset<8>> sums;
    std::bitset<8> zero(0);
    sums.push_back(zero);

    // first convert the message into binary vector
    std::vector<std::bitset<8>> binaryMessage = this->convertToBinary(message);
    //    sums.push_back(binaryMessage[0]);
    int counter = 0;
    // EV << "Message Bytes: " << endl;
    for (std::bitset<8> byte : binaryMessage)
    {

        // EV << byte << endl;
        sums.push_back(this->byteAdder(sums[counter], byte));
        counter++;
    }

    // get the one's complement
    std::bitset<8> checksum(sums[sums.size() - 1]);
    checksum = checksum.flip();
    // EV << "Checksum: " << checksum << endl;
    return checksum;
}

// TODO: calcuate the checksum at the receiver to detect if there's an error
// returns True if an error is detected and False if not
// this will be used as the parity byte to be added to the message trailer
bool Node::detectError(std::string message, int checksum)
{

    // declarations
    std::vector<std::bitset<8>> sums;
    std::bitset<8> zero(0);
    sums.push_back(zero);

    // convert sent received checksum to binary
    std::bitset<8> receivedChecksum(checksum);

    // first convert the message into binary vector
    std::vector<std::bitset<8>> binaryMessage = this->convertToBinary(message);
    binaryMessage.push_back(receivedChecksum);

    int counter = 0;
    for (std::bitset<8> byte : binaryMessage)
    {
        sums.push_back(this->byteAdder(sums[counter], byte));
        counter++;
    }

    if ((int)(sums[sums.size() - 1].flip().to_ulong()) == 0)
        return false;
    return true;
}

void Node::handleError(std::string errorCode, bool &modification, bool &loss, bool &duplication, bool &delay)
{
    // 4 bits
    // 0 -> Modification
    // 1 -> Loss
    // 2 -> Duplication
    // 3 -> Delay

    // Strategy:
    // Loss overrules all
    // Duplication -> send again
    // Modification -> modify before sending
    // Delay -> send after longer time (check for timeout)
    if (errorCode[0] == '1')
    {
        modification = true;
    }

    if (errorCode[1] == '1')
    {
        loss = true;
    }

    if (errorCode[2] == '1')
    {
        duplication = true;
    }

    if (errorCode[3] == '1')
    {
        delay = true;
    }
    EV << "ERROR CODE: " << errorCode << " LOSS: " << loss << endl;
}

// ---------------------------------- UTILITY FUNCTIONS ---------------------------------- //
// TODO: given a string (message) return a vector of its binary representation
std::vector<std::bitset<8>> Node::convertToBinary(std::string String)
{
    std::vector<std::bitset<8>> binaryMessage;

    for (int i = 0; i < String.length(); i++)
    {
        char character = String[i];
        std::bitset<8> asciiBitset(character);
        binaryMessage.push_back(asciiBitset);
        //        EV << asciiBitset << endl;
    }

    return binaryMessage;
}

bool Node::fullAdder(bool firstBit, bool secondBit, bool &carry)
{
    //    EV << "previous carry: " << carry;
    bool sum = firstBit ^ secondBit ^ carry;
    carry = (firstBit & secondBit) | ((firstBit ^ secondBit) & carry);
    //    EV << " FB:" <<firstBit << " SB:" << secondBit << " bitSum: " << sum << " bitCarry: " << carry << endl;
    return sum;
}

std::bitset<8> Node::byteAdder(std::bitset<8> firstByte, std::bitset<8> secondByte)
{
    std::bitset<8> byteSum(0);
    bool carry = false;
    for (int i = 0; i < 8; i++)
    {
        //        EV << i << " ";
        byteSum[i] = this->fullAdder(firstByte[i], secondByte[i], carry);
        //        EV << "byteSum carry: " << carry << endl;
    }
    // handle overflow
    // if final carry is 1 then increment the current byte
    if (carry == true)
    {
        for (int i = 0; i < 8; i++)
        {
            if (byteSum[i] == false)
            {
                byteSum[i] = ~byteSum[i];
                break;
            }
            else
            {
                byteSum[i] = ~byteSum[i];
            }
        }
    }

    //    EV << "Byte Sum: " << byteSum << endl;

    return byteSum;
}

// prints a given vector
void Node::printVector(std::vector<std::bitset<8>> Vector)
{
    for (int i = 0; i < Vector.size(); i++)
    {
        EV << Vector[i] << endl;
    }
}
