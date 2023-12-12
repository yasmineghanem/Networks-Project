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

    // get and initialize system parameters
    nodeID = getIndex();
    PT = getParentModule()->par("PT").doubleValue();
    TD = getParentModule()->par("TD").doubleValue();
    DD = getParentModule()->par("DD").doubleValue();
    ED = getParentModule()->par("ED").doubleValue();
    TO = getParentModule()->par("TO").doubleValue();
    LP = getParentModule()->par("LP").doubleValue();
    WS = getParentModule()->par("WS").intValue();

    // messages vector indices
    startWindow = 0;
    currentIndex = startWindow;
    endWindow = WS - 1;

    // Logger::open()
    Logger::open("../outputs/output.txt");
}

void Node::handleMessage(cMessage *msg)
{
    // Logger::open("../outputs/output.txt");

    // cast the received message to our custom message
    CustomMessage_Base *receivedMessage = dynamic_cast<CustomMessage_Base *>(msg);
    // TODO - Generated method body

    // check first if the message is sent from the coordinator
    // initial message from the coordinator
    if (receivedMessage->getFrameType() == -1) // coordinator
    {

        EV << "COORDINATOR" << endl;
        // message is from the coordinator then this node should start
        // assign the starting node from the coordinator to the sender
        // this will determine which node is the sender and which is the receriver for this session
        sender = receivedMessage->getHeader();
        EV << "Sender: " << Node::sender << endl;

        // 1. read the input file
        this->readFile("input" + std::to_string(sender) + ".txt");

        // initialize window and
        start = messagesToSend.begin();
        current = start;
        end = start + WS - 1;
        cancelAndDelete(receivedMessage);

        EV << "-------------------------------SENDER-------------------------------" << endl;
        processDataToSend();
    }
    else
    {
        if (nodeID == sender) // SENDER SIDE
        {
            EV << "-------------------------------SENDER-------------------------------" << endl;
            // then this is the node that is supposed to send
            // we will call the sending protocol function -> handle the message as the sender
            if (receivedMessage->getFrameType() == 2) // the sender should send the processed data
            {
                // send processed message OR send duplicate
                // The the sender should send the message at the scheduled time
                if (strcmp(receivedMessage->getName(), "End Processing") == 0)
                {
                    // schedule data to be sent with the transmission delay
                    receivedMessage->setName("Transmit");
                    EV << receivedMessage->getPayload() << " DONE PROCESSING AT " << simTime() << endl;

                    // we should handle errors here before updating the indices
                    handleErrors(errorCodes[currentIndex], receivedMessage);
                    // scheduleAt(simTime() + TD, receivedMessage);

                    if (current <= end)
                    {
                        currentIndex++;
                        current++;
                    }

                    if (end < messagesToSend.end() && current >= start && current <= end)
                    {
                        processDataToSend();
                    }
                }
                else
                {
                    // actual transmission of data after the transmission delay
                    EV << receivedMessage->getPayload() << " SENT AT " << simTime() << endl;
                    send(receivedMessage, "port$o");
                }
            }
            else if (receivedMessage->getFrameType() == 1) // the sender is receiving an ack from the receiver
            {
                EV << "RECEIVED ACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;
                receiveAck(receivedMessage);
                if (current >= start && current <= end)
                {
                    processDataToSend();
                }
            }
            else // the sender is receiving a nack from the receiver
            {
                EV << "RECEIVED NACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;
                // receive the NACK
                receiveNack(receivedMessage);
                processDataToSend();
            }
            EV << "START: " << startWindow << " END: " << endWindow << " CURRENT: " << currentIndex << endl;
        }
        else // RECEIVER SIDE
        {
            EV << "-------------------------------RECEIVER-------------------------------" << endl;
            // then this code is executed for the receiving node
            // call the receiving protocol function -> handle the message as the receiver
            if (receivedMessage->getFrameType() == 1 || receivedMessage->getFrameType() == 0) // frametype for ack either end processing or transmit
            {
                std::string log;
                // EV << "SENDING ACK" << endl;
                if (strcmp(receivedMessage->getName(), "End Processing") == 0)
                {
                    receivedMessage->setName("Transmit");

                    // ha3mel haraka ghabeya delwa2ty hagarab haga
                    if (receivedMessage->getFrameType() == 1)
                    {

                        EV << "ACK " << receivedMessage->getAckNack() << " DONE PROCESSING AT " << simTime() << endl;
                        log = "At time " + simTime().str() + " node with ID = " + std::to_string(nodeID) + " started sending time after processing sending ACK with number " + std::to_string(receivedMessage->getAckNack()) + " and LOSS = YES/NO \n";
                    }
                    else
                    {
                        log = "At time " + simTime().str() + " node with ID = " + std::to_string(nodeID) + " started sending time after processing sending NACK with number " + std::to_string(receivedMessage->getAckNack()) + " and LOSS = YES/NO \n";
                        EV << "NACK " << receivedMessage->getAckNack() << " DONE PROCESSING AT " << simTime() << endl;
                        nackSent = true;
                    }
                    // EV << "N/ACK DONE PROCESSING AT " << simTime() << endl;

                    // output
                    Logger::write(log);

                    scheduleAt(simTime() + TD, msg);
                }
                else
                {
                    // ha3mel haraka ghabeya delwa2ty hagarab haga
                    if (receivedMessage->getFrameType() == 1)
                        EV << "SENT ACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;
                    else
                        EV << "SENT NACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;

                    send(receivedMessage, "port$o");
                }
            }
            else // if frametype == 2 then receive the data
            {
                EV << "RECEIVING DATA AT " << simTime() << endl;
                // either receive the sent frame and process it -> deframe
                processReceivedData(receivedMessage);
            }
        }
    }
    // Logger::close();
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

    // print message to send
    EV << "MESSAGE TO SEND: " << messagesToSend[currentIndex];

    // Error message handling
    // bool modification = false;
    // bool loss = false;
    // bool duplication = false;
    // bool delay = false;

    // getErrors(errorCodes[currentIndex], modification, loss, duplication, delay);

    // EV << "ERROR CODE: " << errorCodes[currentIndex] << " LOSS: " << loss << " MODIFICATION: " << modification << " DUPLICATION: " << duplication << " DELAY: " << delay << endl;

    // declarations needed throughout the function
    int sequenceNumber = 0; // the sequence number of the frame max = WS - 1 then reset
    std::bitset<8> parityByte;
    int parity;

    // declare a new message
    CustomMessage_Base *currentMessage = new CustomMessage_Base("CurrentMessage");

    // 2. apply framing using byte stuffing
    std::string framedMessage;
    // framedMessage = this->frameMessage(messagesToSend[currentIndex]);
    framedMessage = this->frameMessage(messagesToSend[currentIndex]);
    // EV <<"Framed CustomMessage_Base: "<< framedMessage << endl;
    char *framedCharacters = new char[framedMessage.length() + 1];
    std::strcpy(framedCharacters, framedMessage.c_str());
    // set the message payload to the framed message
    currentMessage->setPayload(framedCharacters);

    // 3. calculate the sequence number and add it to the header
    // sequenceNumber = 0 % WS;
    sequenceNumber = currentIndex % WS;
    currentMessage->setHeader(sequenceNumber);

    // 4. calculate the checksum and add it to message trailer
    parityByte = this->calculateChecksum(framedMessage);
    parity = (int)(parityByte.to_ulong());
    currentMessage->setTrailer(parity);
    currentMessage->setFrameType(2);

    // send the processed message after the specified processing time
    // handleErrors(errorCodes[currentIndex], modification, loss, duplication, delay, currentMessage);

    // simulate the message being processed
    currentMessage->setName("End Processing");
    scheduleAt(simTime() + PT, currentMessage);
    EV << " MESSAGE START PROCESSING AT " << simTime() << endl;

    // output to log file
    std::string log = "At time " + simTime().str() + " node with ID = " + std::to_string(nodeID) + " started processing, introducing channel error with code = " + errorCodes[currentIndex] + "\n";
    Logger::write(log);
}

void Node::processReceivedData(CustomMessage_Base *msg)
{
    // TODO: implement the receiving protocol
    // 1. get the parity and calculate the checksum to detect if there is an error
    // 2. deframe the received message
    // 3. send the ack

    // check that the received message is the one the receiver is witing for
    if (msg->getHeader() != n_ackNumber) // the received message is the intended message
    {
        // messages where lost in the system
        // send NACK with the message we're waiting for
        // return -> don't process the message
        // if (!nackSent)
        // {
        EV << "MESSAGE NOT RECEIVED DIFFERENT SEQUENCE NUMBER" << endl;
        sendNack();
        // }
        return;
    }
    // nackSent = false;

    std::string receivedMessage = msg->getPayload();

    // 1. get the parity and calculate the checksum to detect if there is an error
    bool error = detectError(receivedMessage, msg->getTrailer());
    EV << "ERROR DETECTED: " << error << endl;

    if (error)
    {
        // do something
        // send Nack for the errored frame
        EV << "MESSAGE NOT RECEIVED CONTAINS ERROR" << endl;
        sendNack();
        return;
    }

    // 2. deframe the received message
    std::string originalMessage = deframeMessage(receivedMessage);

    EV << "MESSAGE RECEIVED: " << originalMessage << endl;

    std::string log = "At time " + simTime().str() + " node with ID = " + std::to_string(nodeID) + " received correct message and uploading PAYLOAD = " + originalMessage + " with SEQ_NUM = " + std::to_string(msg->getHeader()) + " to the network layer\n";
    Logger::write(log);

    receivedMessages.push_back(msg);

    cancelAndDelete(msg);

    n_ackNumber = (n_ackNumber + 1) % WS;

    sendAck();
}

void Node::sendAck()
{
    // TODO: send the ack of the received protocol
    CustomMessage_Base *ack = new CustomMessage_Base("ACK");
    ack->setFrameType(1);
    ack->setAckNack(n_ackNumber);
    ack->setName("End Processing");
    EV << "ACK " << n_ackNumber << " START PROCESSING" << endl;

    // output to log file
    std::string log = "At time " + simTime().str() + " node with ID = " + std::to_string(nodeID) + " started processing ACK with NUMBER = " + std::to_string(n_ackNumber) + "\n";
    Logger::write(log);

    scheduleAt(simTime() + PT, ack);
}

void Node::sendNack()
{
    // TODO: send the ack of the received protocol
    CustomMessage_Base *nack = new CustomMessage_Base("NACK");
    nack->setFrameType(0);
    // expectedFrameNack = n_ackNumber % WS;
    nack->setAckNack(n_ackNumber % WS);
    nack->setName("End Processing");
    EV << "NACK " << n_ackNumber << " START PROCESSING" << endl;

    // output to log file
    std::string log = "At time " + simTime().str() + " node with ID = " + std::to_string(nodeID) + " started processing NACK with NUMBER = " + std::to_string(n_ackNumber) + "\n";
    Logger::write(log);

    scheduleAt(simTime() + PT, nack);
}

void Node::receiveAck(CustomMessage_Base *msg)
{
    // TODO: receive the ack from the receiver
    // update message pointers
    EV << "RECEIVED ACK: " << msg->getAckNack() << endl;

    startWindow++;
    start++;

    if (end < --messagesToSend.end())
    {
        endWindow++;
        end++;
    }
    else
    {
        if (start > messagesToSend.end() - WS - 1)
        {
            startWindow--;
            start--;
        }
    }

    cancelAndDelete(msg);
}

void Node::receiveNack(CustomMessage_Base *msg)
{
    // TODO: receive the nack from the receiver
    int NACK = msg->getAckNack();
    EV << "RECEIVED NACK: " << NACK << endl;
    // adjust the current index and the window start and end inidices
    currentIndex = startWindow;
    current = start;
    while (currentIndex % WS != NACK)
    {
        current++;
        currentIndex++;
    }
    // adjust the error code of the nack frame -> avoid infinate loop
    errorCodes[currentIndex] = "0000";
    cancelAndDelete(msg);
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

    inputFile.close();
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
    // EV << "CustomMessage_Base Bytes: " << endl;
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

void Node::getErrors(std::string errorCode, bool &modification, bool &loss, bool &duplication, bool &delay)
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
}

void Node::handleErrors(std::string errorCode, CustomMessage_Base *msg)
{
    bool modification = false;
    bool loss = false;
    bool duplication = false;
    bool delay = false;

    getErrors(errorCode, modification, loss, duplication, delay);

    EV << "ERROR CODE: " << errorCodes[currentIndex] << " LOSS: " << loss << " MODIFICATION: " << modification << " DUPLICATION: " << duplication << " DELAY: " << delay << endl;

    // output to log file
    std::string log = "At time " + simTime().str() + " node with ID = " + std::to_string(nodeID) + " started sending time after processing, sent frame with SEQ_NUM = " + std::to_string(msg->getHeader()) + " and PAYLOAD = " + msg->getPayload() + " and TRAILER = " + std::to_string(msg->getTrailer()) + "\n";
    log = log + " MODIFIED " + std::to_string(modification) + " LOST " + std::to_string(loss) + " DUPLICATED " + std::to_string(duplication) + " DELAY " + std::to_string(delay) + "\n";
    Logger::write(log);

    if (loss)
    {
        // don't send anything
        return;
    }

    if (modification && duplication && delay)
    {
        // handle this error
        // 1. modify message
        modifyMessage(msg);
        // 2. send modified delayed message
        scheduleAt(simTime() + TD + ED, msg);
        // 3. send duplicated modified delayed message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + ED + DD, dupliactedMessage);
        return;
    }

    else if (modification && duplication)
    {
        // handle this error
        // 1. modify message
        modifyMessage(msg);
        // 2. send modified message
        scheduleAt(simTime() + TD, msg);
        // 3. send duplicated modified message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + DD, dupliactedMessage);
        return;
    }

    else if (modification && delay)
    {
        // handle this error
        // 1. modify message
        modifyMessage(msg);
        // 2. send delayed modified message
        scheduleAt(simTime() + TD + ED, msg);
        return;
    }

    else if (duplication && delay)
    {
        // handle this error
        // 1. delay the message
        scheduleAt(simTime() + TD + ED, msg);
        // 2. duplicate the message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + ED + DD, dupliactedMessage);
        return;
    }

    else if (modification)
    {
        // handle modification error
        // 1. modify message
        modifyMessage(msg);
        // 2. send modified message
        scheduleAt(simTime() + TD, msg);
        return;
    }

    else if (duplication)
    {
        // handle duplication error
        // 1. send original message
        scheduleAt(simTime() + TD, msg);
        // 2. send duplicated message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + DD, dupliactedMessage);
        return;
    }

    else if (delay)
    {
        // handle delay error
        // send delayed message
        scheduleAt(simTime() + TD + ED, msg);
        return;
    }

    scheduleAt(simTime() + TD, msg);
}

void Node::modifyMessage(CustomMessage_Base *msg)
{

    std::string payload = msg->getPayload();
    EV << "TEST MODIFY MESSAGE" << endl;
    EV << "Payload: " << payload << endl;

    // first convert the message into binary vector
    std::vector<std::bitset<8>> binaryBitset = this->convertToBinary(payload);
    std::string binaryMessage = "";

    // convert the vector to a continuous string
    for (int i = 0; i < binaryBitset.size(); i++)
    {
        binaryMessage += binaryBitset[i].to_string();
    }

    // get a random bit in the message to modify
    int errorBit = int(uniform(0, binaryMessage.length()));
    if (binaryMessage[errorBit] == '0')
    {
        binaryMessage[errorBit] = '1';
    }
    else
    {
        binaryMessage[errorBit] = '0';
    }

    // convert the message to its original form again

    std::string modifiedMessage = "";
    std::istringstream in(binaryMessage);
    std::bitset<8> bs;
    while (in >> bs)
    {
        modifiedMessage += char(bs.to_ulong());
    }

    char *modifiedCharacters = new char[modifiedMessage.length() + 1];
    std::strcpy(modifiedCharacters, modifiedMessage.c_str());
    msg->setPayload(modifiedCharacters);

    EV << "MODIFIED MESSAGE: " << modifiedMessage << endl;
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
