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

    // initialize timeout varibales
    for (int i = 0; i < WS + 1; i++)
    {
        std::string timerIndex = "Timer" + std::to_string(i);
        CustomMessage_Base *timerMessage = new CustomMessage_Base(timerIndex.c_str());
        timerMessage->setFrameType(3);
        timers.push_back(std::make_pair(nullptr, timerMessage));
    }

    startTimer = 0;
    endTimer = WS - 1;

    Logger::open("../outputs/output.txt");
}

void Node::handleMessage(cMessage *msg)
{
    // cast the received message to our custom message
    CustomMessage_Base *receivedMessage = dynamic_cast<CustomMessage_Base *>(msg);
    // TODO - Generated method body

    // check first if the message is sent from the coordinator
    // initial message from the coordinator
    if (receivedMessage->getFrameType() == -1) // COORDINATOR
    {
        EV << "COORDINATOR" << endl;
        // message is from the coordinator then this node should start
        // assign the starting node from the coordinator to the sender
        // this will determine which node is the sender and which is the receriver for this session
        sender = receivedMessage->getHeader();

        // 1. read the input file
        this->readFile("input" + std::to_string(sender) + ".txt");

        // initialize window size, start and end
        start = messagesToSend.begin();
        current = start;

        if (WS < messagesToSend.size())
            end = start + WS - 1;
        else
            end = --messagesToSend.end();

        cancelAndDelete(receivedMessage);

        EV << "-------------------------------SENDER-------------------------------" << endl;
        processDataToSend();
    }
    else
    {
        if (nodeID == sender) // SENDER'S SIDE
        {
            EV << "-------------------------------SENDER-------------------------------" << endl;
            // then this is the node that is supposed to send
            // we will call the sending protocol function -> handle the message as the sender

            // handle timeout
            if (receivedMessage->getFrameType() == 3) // the frametype for the timeout event
            {
                EV << "PROCESSING TIMEOUT MESSAGE " << endl;
                // output timeout event to log file
                std::string log = "TIMEOUT event at " + simTime().str() + " at NODE[" + std::to_string(nodeID) + "] for frame with SEQ_NUM[" + std::to_string(receivedMessage->getHeader()) + "]\n";
                Logger::write(log);
                // send the message again
                resendTimeoutMessages(receivedMessage->getHeader());
                processDataToSend();
            }
            else if (receivedMessage->getFrameType() == 2) // the sender should send the processed data
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
                    handleTimeout(receivedMessage);
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
                    // handleTimeout(receivedMessage);
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
        else // RECEIVER'S SIDE
        {
            EV << "-------------------------------RECEIVER-------------------------------" << endl;
            // then this code is executed for the receiving node
            // call the receiving protocol function -> handle the message as the receiver
            if (receivedMessage->getFrameType() == 1 || receivedMessage->getFrameType() == 0) // frametype for ack either end processing or transmit
            {

                if (strcmp(receivedMessage->getName(), "End Processing") == 0) // the ACK/NACK has finished processing and should be sent after the transmission delay
                {
                    // simulate the ack being lost
                    bool lost = loseAck(LP);
                    std::string messageType;
                    std::string loss;
                    std::string log;

                    if (lost)
                        loss = "YES";
                    else
                        loss = "NO";

                    receivedMessage->setName("Transmit");

                    if (receivedMessage->getFrameType() == 1)
                    {

                        EV << "ACK " << receivedMessage->getAckNack() << " DONE PROCESSING AT " << simTime() << endl;
                        messageType = "ACK";
                    }
                    else
                    {
                        EV << "NACK " << receivedMessage->getAckNack() << " DONE PROCESSING AT " << simTime() << endl;
                        messageType = "NACK";
                    }

                    // output to the log file
                    log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] started sending time after processing sending " + messageType + " with number " + std::to_string(receivedMessage->getAckNack()) + " and LOSS = " + loss + " \n";
                    Logger::write(log);

                    EV << " N/ACK LOST: " << lost << endl;

                    // schedule to send the message
                    if (!lost)
                    {
                        scheduleAt(simTime() + TD, msg);
                    }
                }
                else
                {

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
}

void Node::finish()
{
    // Custom finish code here
    EV << "Simulation finished\n";

    Logger::close();
    // Call the finish function of the base class
    cSimpleModule::finish();
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
    // 7. output to the log file
    // don't forget to update the current sequence number and check the window size

    // print message to send
    EV << "MESSAGE TO SEND: " << messagesToSend[currentIndex] << endl;

    // declarations needed throughout the function
    int sequenceNumber = 0; // the sequence number of the frame max = WS - 1 then reset
    std::bitset<8> parityByte;
    std::string framedMessage;
    int parity;

    // declare a new message
    CustomMessage_Base *currentMessage = new CustomMessage_Base("CurrentMessage");

    // 2. apply framing using byte stuffing
    framedMessage = this->frameMessage(messagesToSend[currentIndex]);
    char *framedCharacters = new char[framedMessage.length() + 1];
    std::strcpy(framedCharacters, framedMessage.c_str());

    // set the message payload to the framed message
    currentMessage->setPayload(framedCharacters);

    // 3. calculate the sequence number and add it to the header
    sequenceNumber = currentIndex % (WS + 1);
    currentMessage->setHeader(sequenceNumber);

    // 4. calculate the checksum and add it to message trailer
    parityByte = this->calculateChecksum(framedMessage);
    parity = (int)(parityByte.to_ulong());
    currentMessage->setTrailer(parity);
    currentMessage->setFrameType(2);

    // simulate the message being processed
    currentMessage->setName("End Processing");
    scheduleAt(simTime() + PT, currentMessage);
    EV << " MESSAGE WITH SEQUENCE NUMBER " << sequenceNumber << " START PROCESSING AT " << simTime() << endl;

    // output to log file
    std::string log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] started processing, introducing channel error with code = " + errorCodes[currentIndex] + "\n";
    Logger::write(log);
}

void Node::processReceivedData(CustomMessage_Base *msg)
{
    // TODO: implement the receiving protocol
    // 1. get the parity and calculate the checksum to detect if there is an error
    // 2. deframe the received message
    // 3. send the ack

    EV << "NACK SENT = " << nackSent << endl;

    CustomMessage_Base *receivedMessage = msg->dup();

    EV << receivedMessage->getPayload() << endl;

    // check that the received message is the one the receiver is witing for
    if (receivedMessage->getHeader() != n_ackNumber) // the received message is the intended message or is a timeout message
    {
        // messages where lost in the system
        // send ACK with the message we're waiting for
        // return -> don't process the message

        // check if it's duplicated then we don't send a nack

        bool duplicate = checkDuplicate(msg);
        EV << duplicate << endl;

        if (duplicate)
        {
            return;
        }
        EV << "MESSAGE NOT RECEIVED DIFFERENT SEQUENCE NUMBER" << endl;

        sendAck();
        return;
    }

    std::string receivedPayload = receivedMessage->getPayload();

    // 1. get the parity and calculate the checksum to detect if there is an error
    bool error = detectError(receivedPayload, receivedMessage->getTrailer());
    EV << "ERROR DETECTED: " << error << endl;

    if (error)
    {
        // send nack for the errored frame
        EV << "MESSAGE NOT RECEIVED CONTAINS ERROR" << endl;
        if (!nackSent)
        {
            sendNack();
        }
        return;
    }

    nackSent = false;

    // 2. deframe the received message
    std::string originalMessage = deframeMessage(receivedPayload);

    // set the original message after deframing to the message's payload
    char *payload = new char[originalMessage.length() + 1];
    std::strcpy(payload, originalMessage.c_str());
    receivedMessage->setPayload(payload);

    EV << "MESSAGE RECEIVED: " << receivedMessage->getPayload() << endl;

    receivedMessages.push_back(receivedMessage);

    std::string log = "UPLOADING PAYLOAD = " + originalMessage + " with SEQ_NUM[" + std::to_string(receivedMessage->getHeader()) + "] to the network layer " + "at time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "]\n";
    Logger::write(log);

    // cancelAndDelete(msg);

    // increment the ack/nack number to correspond to the next frame expected
    n_ackNumber = (n_ackNumber + 1) % (WS + 1);
    sendAck();
    // cancelAndDelete(msg);
}

void Node::sendAck()
{
    // TODO: send the ack of the received protocol
    CustomMessage_Base *ack = new CustomMessage_Base("End Processing"); // create a new message for the acknowledment and set its name to simulate the message's processing time
    ack->setFrameType(1);                                               // sets the framtype to the ack's frametype
    ack->setAckNack(n_ackNumber);                                       // sets the messages's ack number to the next expected frame
    EV << "ACK " << n_ackNumber << " START PROCESSING" << endl;

    // output to log file
    std::string log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] started processing ACK with NUMBER = " + std::to_string(n_ackNumber) + "\n";
    Logger::write(log);

    scheduleAt(simTime() + PT, ack);
}

void Node::sendNack()
{
    // TODO: send the ack of the received protocol
    CustomMessage_Base *nack = new CustomMessage_Base("End Processing"); // create a new message for the nack and set its name to simulate the message's processing time
    nack->setFrameType(0);                                               // sets the framtype to the nack's frametype
    nack->setAckNack(n_ackNumber);                                       // sets the messages's ack number to the frame that should be resent
    EV << "NACK " << n_ackNumber << " START PROCESSING" << endl;

    // output to log file
    std::string log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] started processing NACK with NUMBER = " + std::to_string(n_ackNumber) + "\n";
    Logger::write(log);

    scheduleAt(simTime() + PT, nack);
    nackSent = true;
}

void Node::receiveAck(CustomMessage_Base *msg)
{
    int messagesCancelled = 0;
    // TODO: receive the ack from the receiver
    // update message pointers
    EV << "RECEIVED ACK: " << msg->getAckNack() << endl;

    EV << "HANDLING ACCUMULATIVE ACKS AND TIMEOUT EVENTS" << endl;
    // fisrt cancel the timeout event for the current message and clear its message

    int sequenceNumber;

    if (msg->getAckNack() == 0)
        sequenceNumber = WS;
    else
        sequenceNumber = msg->getAckNack() - 1;

    // then check all the previuos messages to handle accumulative acknowledment
    // in case any of the acks got lost in the process but the timer hasn't set off yet
    EV << "CANCELING AND DELETING TIMEOUT EVENT FOR CURRENT AND PREVIOUS ACKS" << endl;

    for (int i = sequenceNumber; i >= startTimer; i--)
    {
        if (timers[i].first != nullptr)
        {
            EV << "CANCELING AND DELETING TIMEOUT EVENT FOR " << i << " " << timers[i].first->getPayload() << endl;

            cancelEvent(timers[i].second);
            timers[i].first = nullptr;
            // slide the window to the right one step
            startWindow++;
            start++;

            // checks that the end of the window still didn't reach the end of the messages
            // if it is at the end then we shouldn't increment it
            if (end < --messagesToSend.end())
            {
                endWindow++;
                end++;
            }
            else
            {
                // if the window is at the end check that the start window is not sliding as well
                if (start > messagesToSend.end() - WS - 1)
                {
                    startWindow--;
                    start--;
                }
            }
            messagesCancelled++;
        }
        else
        {
            break;
        }

        EV << timers[i].first << endl;
    }

    EV << "NUMBER OF MESSAGES CANCELLED: " << messagesCancelled << endl;
    startTimer = (startTimer + messagesCancelled) % (WS + 1);
    endTimer = (endTimer + messagesCancelled) % (WS + 1);

    EV << "START TIMER INDEX " << startTimer << endl;
    EV << "END TIMER INDEX " << endTimer << endl;

    EV << "DONE CANCELLING" << endl;
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

    // cancel timers for all messages in the window
    for (int i = 0; i < WS; i++)
    {
        EV << "CANCELING TIMEOUT MESSAGES" << endl;
        if (timers[i].first != nullptr)
        {
            EV << "CANCELLING MESSAGE WITH SEQ_NUM " << i << " AND PAYLOAD " << timers[i].first->getPayload() << endl;
            cancelEvent(timers[i].second);
            timers[i].first = nullptr;
        }
    }

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
            // read the first four characters as the error bits
            errorCode = "";
            for (int i = 0; i < 4; i++)
            {
                errorCode += inputFile.get();
            }
            inputFile.get();

            // read the remaining of the line as the message
            std::getline(inputFile, message);
            messagesToSend.push_back(message);
            errorCodes.push_back(errorCode);

            EV << errorCode << endl;
            EV << message << endl;
        }
    }

    inputFile.close();
}

// ---------------------------------- PROTOCOL FUNCTIONS ---------------------------------- //
// TODO: given a string (message) return the framed message to be sent
// using the byte stuffing framing method
std::string Node::frameMessage(std::string message)
{
    // add the flag byte at the beginning
    std::string framedMessage = "$";

    // apply byte dtuffing to the whole message
    for (int i = 0; i < message.length(); i++)
    {
        if (message[i] == '/' || message[i] == '$')
            framedMessage += '/';
        framedMessage += message[i];
    }

    // add the flag byte at the end
    framedMessage += '$';
    return framedMessage;
}

// TODO: given a framed message return the original message
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
    int counter = 0;
    for (std::bitset<8> byte : binaryMessage)
    {
        sums.push_back(this->byteAdder(sums[counter], byte));
        counter++;
    }

    // get the one's complement
    std::bitset<8> checksum(sums[sums.size() - 1]);
    checksum = checksum.flip();
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

    if (sums[sums.size() - 1].flip() == zero)
    {
        return false;
    }
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
    int modifiedBit;
    std::bitset<8> trialer(msg->getTrailer());

    getErrors(errorCode, modification, loss, duplication, delay);

    EV << "ERROR CODE: " << errorCodes[currentIndex] << " MODIFICATION: " << modification << " LOSS: " << loss << " DUPLICATION: " << duplication << " DELAY: " << delay << endl;

    // output to log file
    std::string log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + std::to_string(msg->getTrailer()) + "] in BITS[" + trialer.to_string() + "]\n";
    // log = log + " MODIFIED " + std::to_string(modification) + " LOST " + std::to_string(loss) + " DUPLICATED " + std::to_string(duplication) + " DELAY " + std::to_string(delay) + "\n";

    if (loss)
    {
        // don't send anything

        log += "MODIFIED [-1], LOST [Yes], DUPLICATE [0], DELAY [0] \n";
        Logger::write(log);

        return;
    }

    if (modification && duplication && delay)
    {
        // handle this error
        // 1. modify message
        modifiedBit = modifyMessage(msg);
        // 2. send modified delayed message
        scheduleAt(simTime() + TD + ED, msg);
        Logger::write(log + "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [1], DELAY [" + std::to_string(ED) + "] \n");

        // 3. send duplicated modified delayed message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + ED + DD, dupliactedMessage);

        // log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [1/2], DELAY [" + std::to_string(ED) + "] \n";
        Logger::write(log + "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [2], DELAY [" + std::to_string(ED) + "] \n");

        return;
    }

    if (modification && duplication)
    {
        // handle this error
        // 1. modify message
        modifiedBit = modifyMessage(msg);
        // 2. send modified message
        scheduleAt(simTime() + TD, msg);
        Logger::write(log + "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [1], DELAY [0] \n");

        // 3. send duplicated modified message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + DD, dupliactedMessage);

        // log += "Modified [" + std::to_string(modifiedBit) + "], Lost [No], Duplicate [1/2], Delay [0] \n";
        Logger::write(log + "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [2], DELAY [0] \n");
        return;
    }

    if (modification && delay)
    {
        // handle this error
        // 1. modify message
        modifiedBit = modifyMessage(msg);
        // 2. send delayed modified message
        scheduleAt(simTime() + TD + ED, msg);

        log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [0], DELAY[" + std::to_string(ED) + "] \n";
        Logger::write(log);

        return;
    }

    if (duplication && delay)
    {
        // handle this error
        // 1. delay the message
        scheduleAt(simTime() + TD + ED, msg);
        Logger::write(log + "MODIFIED [-1], LOST [No], DUPLICATE [2], DELAY [" + std::to_string(ED) + "]\n");

        // 2. duplicate the message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + ED + DD, dupliactedMessage);

        // log += "MODIFIED [-1], LOST [No], DUPLICATE [1/2], DELAY " + std::to_string(ED) + "\n";
        Logger::write(log + "MODIFIED [-1], LOST [No], DUPLICATE [2], DELAY [" + std::to_string(ED) + "]\n");

        return;
    }

    if (modification)
    {

        // handle modification error
        // 1. modify message
        modifiedBit = modifyMessage(msg);
        // 2. send modified message
        scheduleAt(simTime() + TD, msg);

        log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [0], DELAY [0] \n";
        Logger::write(log);
        return;
    }

    if (duplication)
    {

        // handle duplication error
        // 1. send original message
        scheduleAt(simTime() + TD, msg);
        Logger::write(log + "MODIFIED [-1], LOST [No], DUPLICATE [1], DELAY [0] \n");

        // 2. send duplicated message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + DD, dupliactedMessage);
        Logger::write(log + "MODIFIED [-1], LOST [No], DUPLICATE [2], DELAY [0] \n");
        return;
    }

    if (delay)
    {
        // EV << "DELAYED MESSAGE ONLY SCHEDULED TO SEND AFTER " << TD + ED << endl;
        // handle delay error
        // send delayed message
        scheduleAt(simTime() + TD + ED, msg);
        log += "MODIFIED [-1], LOST [No], DUPLICATE [0], DELAY [" + std::to_string(ED) + "] \n";
        Logger::write(log);
        return;
    }

    scheduleAt(simTime() + TD, msg);
    log += "Modified [-1], Lost [No], Duplicate [0], Delay [0] \n";
    Logger::write(log);
}

int Node::modifyMessage(CustomMessage_Base *msg)
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

    EV << "MODIFIED MESSAGE: " << modifiedMessage << " ERROR BIT: " << errorBit << endl;
    return errorBit;
}

bool Node::checkDuplicate(CustomMessage_Base *msg)
{
    // get the original payload of the message first
    std::string deframedMessage = deframeMessage(msg->getPayload());
    char *tempPayload = new char[deframedMessage.length() + 1];
    std::strcpy(tempPayload, deframedMessage.c_str());
    // msg->setPayload(tempPayload);

    EV << "CHECKING DUPLICATES" << endl;

    for (int i = 0; i < receivedMessages.size(); i++)
    {
        EV << receivedMessages[i]->getHeader() << " " << receivedMessages[i]->getPayload() << " " << receivedMessages[i]->getTrailer() << endl;
        if (receivedMessages[i]->getHeader() != msg->getHeader())
            continue;
        if (std::strcmp(tempPayload, receivedMessages[i]->getPayload()) == 0 && msg->getTrailer() == receivedMessages[i]->getTrailer())
        {
            EV << "DUPLICATED MESSAGE" << endl;
            return true;
        }
    }
    return false;
}

bool Node::loseAck(double LP)
{
    srand(time(0));

    double randomProbability;
    for (int i = 0; i < 2; i++)
    {
        randomProbability = ((double)rand()) / RAND_MAX;
    }
    EV << "PROBABILITY: " << randomProbability << endl;
    if (randomProbability <= LP)
    {
        return true;
    }
    return false;
}

void Node::handleTimeout(CustomMessage_Base *msg)
{
    int sequenceNumber = msg->getHeader();

    if (timers[sequenceNumber].first != nullptr)
        return;

    // should start timer of the message sent
    EV << "STARTED TIMER FOR MESSAGE, TO = " << TO << " " << sequenceNumber << " " << msg->getPayload() << endl;
    timers[sequenceNumber].first = msg;
    timers[sequenceNumber].second->setHeader(sequenceNumber);
    scheduleAt(simTime() + TO, timers[sequenceNumber].second);
}

void Node::resendTimeoutMessages(int sequenceNumber)
{
    EV << "TIMEOUT MESSAGES" << endl;
    currentIndex = startWindow;
    current = start;
    while (currentIndex % (WS + 1) != sequenceNumber)
    {
        current++;
        currentIndex++;
    }

    // adjust the error code of the timeout frame -> avoid infinate loop
    errorCodes[currentIndex] = "0000";

    int index = startTimer;

    EV << "CANCELING TIMEOUT MESSAGES" << endl;

    while ((index % (WS + 1)) != endTimer)
    {

        if (timers[index].first != nullptr)
        {

            EV << "CANCELLING MESSAGE WITH SEQ_NUM " << index << " AND PAYLOAD " << timers[index].first->getPayload() << endl;

            cancelEvent(timers[index].second);

            timers[index].first = nullptr;
        }
        index = index + 1;
    }
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

// DESTRUCTOR
Node::~Node()
{
    // print messages before closing msh mafrood hena bas wtv

    // Logger::close();

    for (auto message : receivedMessages)
    {
        delete message;
    }
    receivedMessages.clear();

    for (const auto &timer : timers)
    {
        delete timer.first;
        delete timer.second;
    }
    timers.clear();
}
