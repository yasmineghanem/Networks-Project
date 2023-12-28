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
    resending = false;
    expectedACK = true;

    // initialize timeout varibales
    for (int i = 0; i < WS + 1; i++)
    {
        std::string timerIndex = "Timer" + std::to_string(i);
        CustomMessage_Base *timerMessage = new CustomMessage_Base(timerIndex.c_str());
        timerMessage->setFrameType(3);
        timers.push_back(std::make_pair(false, timerMessage));
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

        // message is from the coordinator then the number specified in the header should start
        // assign the starting node from the coordinator to the sender
        // this will determine which node is the sender and which is the receriver for this session
        sender = receivedMessage->getHeader();

        // read messages from the input file
        this->readFile("input" + std::to_string(sender) + ".txt");

        // initialize window size, start and end iterators
        start = messagesToSend.begin();
        current = start;

        if (WS < messagesToSend.size()) // if the number of messages is greater than the window size
            end = start + WS - 1;
        else
            end = --messagesToSend.end(); // if the size of messages is smaller than the window size

        // dispose of unwanted messages
        cancelAndDelete(receivedMessage);

        EV << "-------------------------------SENDER-------------------------------" << endl;
        // start processing and sending the first message
        processDataToSend();
    }
    else
    {
        if (nodeID == sender) // SENDER'S SIDE
        {
            EV << "-------------------------------SENDER-------------------------------" << endl;
            // then this is the node that is supposed to process and send
            // handle the sender side
            // the sender node should also handle receiving ACKS/NACKS from the receiver's node
            // and also handle messages if they timeout

            if (receivedMessage->getFrameType() == 3) // the frametype specified for the timeout event
            {
                EV << "PROCESSING TIMEOUT MESSAGE " << endl;

                // output timeout event to log file
                std::string log = "TIMEOUT event at [" + simTime().str() + "] at NODE[" + std::to_string(nodeID) + "] for frame with SEQ_NUM[" + std::to_string(receivedMessage->getHeader()) + "]\n";
                Logger::write(log);

                // updates the current index of the messages queue to resend all the messages in the window after and including the timed out message
                resendMessages(receivedMessage->getHeader());

                processDataToSend();
            }
            else if (receivedMessage->getFrameType() == 2) // the frametype specified for the processed data to send

            {
                if (strcmp(receivedMessage->getName(), "End Processing") == 0) // schedule the message that ended processing to be sent after the specified time
                {
                    receivedMessage->setName("Transmit");
                    EV << receivedMessage->getPayload() << " DONE PROCESSING AT " << simTime() << endl;

                    // schedule the data to be sent according to the errors read with the messages
                    sendMessage(errorCodes[currentIndex], receivedMessage);

                    // start the timer for the sent message
                    startMessageTimer(receivedMessage);

                    // increment the current index to process the next message
                    if (current <= end)
                    {
                        currentIndex++;
                        current++;
                    }

                    // check if there are still more messages to process
                    if (end < messagesToSend.end() && current >= start && current <= end)
                    {
                        processDataToSend();
                    }
                }
                else // the actual sending of the data after the scheduling time is up
                {
                    EV << receivedMessage->getPayload() << " SENT AT " << simTime() << endl;
                    send(receivedMessage, "port$o");
                }
            }
            else if (receivedMessage->getFrameType() == 1) // frametype specified to for the ACK from the receiver's node
            {
                EV << "RECEIVED ACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;

                // receive the ACK and update parameters accordingly
                bool expectedACK = receiveAck(receivedMessage);

                // if there are still more messages to process and send
                // the expectedACK variable specifies if the received ack is the one the sender is waiting for
                if (current >= start && current <= end && expectedACK)
                {
                    processDataToSend();
                }
            }
            else // frametype specified to for receiving the NACK from the receiver's node
            {
                EV << "RECEIVED NACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;

                // updates the current index of the message queue to the resend the errored message + all subsequent messages in the window
                resendMessages(receivedMessage->getAckNack());

                // bool to indicate whether this is the first time sending the message of the sender is reseinding the message
                resending = true;

                // proceed with sending the messages in the queue
                processDataToSend();
            }
            EV << "START: " << startWindow << " END: " << endWindow << " CURRENT: " << currentIndex << endl;
        }
        else // RECEIVER'S SIDE
        {
            EV << "-------------------------------RECEIVER-------------------------------" << endl;
            // then this is the node that is supposed to receive the data and process them
            // identifies errored messages and out of order messages
            // the reciever side should send ACKS/NACKS to the sender's node on messages received

            if (receivedMessage->getFrameType() == 1 || receivedMessage->getFrameType() == 0) // the frametypes of the ACKS/NACKS scheduled by the receiver
            {
                if (strcmp(receivedMessage->getName(), "End Processing") == 0) // the ACK/NACK has finished processing and should be sent after the specified  delay
                {
                    // handle if the ACK/NACK is supposed to be lost
                    bool lost = isLost(LP);

                    // variables for writing in the log file according to the type of the message (ACK or NACK)
                    std::string messageType;
                    std::string loss;
                    std::string log;

                    // the lost ack for the output log file
                    if (lost)
                        loss = "YES";
                    else
                        loss = "NO";

                    // rename the name of the message to indicate the it's being sent
                    receivedMessage->setName("Transmit");

                    // will be deleted
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

                    // output to the log file that the ACK/NACK is being sent
                    log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] sending [" + messageType + "] with NUMBER[" + std::to_string(receivedMessage->getAckNack()) + "] and LOSS[" + loss + "] \n";
                    Logger::write(log);

                    EV << " N/ACK LOST: " << lost << endl;

                    // schedule to send the message if the ACK/NACK is not supposed to be lost
                    if (!lost)
                    {
                        scheduleAt(simTime() + TD, msg);
                    }
                }
                else // actual sending of the ACK/NACK after the transmission delay specified
                {

                    if (receivedMessage->getFrameType() == 1)
                        EV << "SENT ACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;
                    else
                        EV << "SENT NACK NUMBER " << receivedMessage->getAckNack() << " AT " << simTime() << endl;

                    send(receivedMessage, "port$o");
                }
            }
            else // the received message is the data sent from the sender and supposed to be processed
            {
                EV << "RECEIVING DATA AT " << simTime() << endl;

                // receive the sent frame from the sender's node and process it
                processReceivedData(receivedMessage);
            }
        }
    }
}

/**
 * Override the finish function to add closing the file and removing the undisposed messages
 */
void Node::finish()
{
    // Custom finish code here
    EV << "Simulation finished\n";

    Logger::close();
    cSimpleModule::finish();
}

// ---------------------------------- MAIN FUNCTIONS ---------------------------------- //

/**
 * @brief Process the next message to be sent from the messages queue
 *
 * Steps:
 * 1. frame the message by applying byte stuffing algorithm and set it to the payload
 * 2. calculate the checksum of the framed message and add it as the parity byte to the trailer
 * 3. writes the starting processing time to the output log file
 *
 */
void Node::processDataToSend()
{
    // TODO: implement the sending protocol

    EV << "MESSAGE TO SEND: " << messagesToSend[currentIndex] << endl;

    // declarations needed throughout the function
    int sequenceNumber = 0;    // the sequence number of the frame MAX_SEQ = WS
    std::bitset<8> parityByte; // parity byte after calculating the checksum
    std::string framedMessage; // message after applying the byte stuffung
    int parity;                // parity byte after being converted to int

    // creates a new message object
    CustomMessage_Base *currentMessage = new CustomMessage_Base("CurrentMessage");

    // call the frame message functions and pass the intended message
    framedMessage = this->frameMessage(messagesToSend[currentIndex]);
    char *framedCharacters = new char[framedMessage.length() + 1];
    std::strcpy(framedCharacters, framedMessage.c_str());
    currentMessage->setPayload(framedCharacters); // set the message payload to the framed message

    // calculate the sequence number of the current frame and set it to the message header
    sequenceNumber = currentIndex % (WS + 1);
    currentMessage->setHeader(sequenceNumber);

    // call the checksum calculations function  and set it to the message trailer
    parityByte = this->calculateChecksum(framedMessage);
    parity = (int)(parityByte.to_ulong()); // converts the parity byte to int for easier tracing (should change it)
    currentMessage->setTrailer(parity);

    // set the frametype to the specified data frametype
    currentMessage->setFrameType(2); // set the message trailer to the calculated checksum parity byte

    // rename the message to indicate that the message is being processed
    currentMessage->setName("End Processing");

    // if (resending)
    // {
    //     scheduleAt(simTime() + PT + 0.001, currentMessage);
    //     resending = false;
    // }
    // else
    scheduleAt(simTime() + PT, currentMessage);

    EV << " MESSAGE WITH SEQUENCE NUMBER " << sequenceNumber << " START PROCESSING AT " << simTime() << endl;

    // write the start processing time and messgae details to the output log file
    std::string log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] started processing, introducing channel error with CODE[" + errorCodes[currentIndex] + "]\n";
    Logger::write(log);
}

/**
 * @brief Process the received message from the sender
 *
 * Steps:
 * 1. check if the received frame has the expected sequence number => if it's not then send ACK on the expected frame
 * 2. get the parity and calculate the checksum to detect if there is an error
 * 3. deframe the received message if the message doesn't contain any errors
 * 4. save the received message and send the ACK
 * 5. if the message contains any errors then send the NACK for the sender to resend the errored message
 *
 *@param msg the received frame from the sender casted to our CustomMessage
 */
void Node::processReceivedData(CustomMessage_Base *msg)
{
    // TODO: implement the receiving protocol

    EV << "NACK SENT = " << nackSent << endl;

    // create a new message with the same to be able to save the message received
    CustomMessage_Base *receivedMessage = msg->dup();

    EV << receivedMessage->getPayload() << endl;

    // check that the received message is the expected frame by the receiver
    if (receivedMessage->getHeader() != n_ackNumber) // the received message is the intended message or is a timeout message
    {
        // messages where lost in the system or the message is duplicated
        // send ACK with the message we're waiting for

        // check if the message was received before
        bool duplicate = isDuplicate(msg);
        EV << duplicate << endl;

        if (duplicate) // if the message is duplicated then we don't send ACKS/NACKS (throw the message)
        {
            cancelAndDelete(msg); // cancel and delete the received message (has no use)
            return;               // return without sending anything
        }
        EV << "MESSAGE NOT RECEIVED DIFFERENT SEQUENCE NUMBER" << endl;

        // if the frame is not the expected frame but it's not duplicated

        sendAck();            // then we send an ACK with the expected frame
        cancelAndDelete(msg); // cancel and delete the received message (has no use)
        return;
    }

    /**
     * the frame is the expected frame by the receiver
     */

    // get the payload to process the data sent
    std::string receivedPayload = receivedMessage->getPayload();

    // check if the message contains any errors by calculating the checksum
    bool error = detectMessageError(receivedPayload, receivedMessage->getTrailer());
    EV << "ERROR DETECTED: " << error << endl;

    if (error) // if the message contains an error
    {
        // send NACK with the errored frame number
        EV << "MESSAGE NOT RECEIVED CONTAINS ERROR" << endl;
        if (!nackSent) // checks if a NACK has been sent before (NACKS are only sent once)
        {
            sendNack();
        }
        cancelAndDelete(msg); // cancel and delete the received message
        return;
    }

    nackSent = false; // no NACKS should be sent at this point

    // deframe the received message
    std::string originalMessage = deframeMessage(receivedPayload);

    // set the deframed (original) message after deframing to the message's payload
    char *payload = new char[originalMessage.length() + 1];
    std::strcpy(payload, originalMessage.c_str());
    receivedMessage->setPayload(payload);

    EV << "MESSAGE RECEIVED: " << receivedMessage->getPayload() << endl;

    // save the recived message
    receivedMessages.push_back(receivedMessage);

    // write to the output file
    std::string log = "UPLOADING PAYLOAD[" + originalMessage + "] with SEQ_NUM[" + std::to_string(receivedMessage->getHeader()) + "] to the network layer " + "at time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "]\n";
    Logger::write(log);

    // handle the frame's ACK
    n_ackNumber = (n_ackNumber + 1) % (WS + 1); // increment the ACK?NACK number to correspond to the next frame expected

    sendAck();            // send the ACK for the next expected frame
    cancelAndDelete(msg); // cancel and delete the received message
    return;
}

/**
 * @brief Process the ACK and schedule to send it
 *
 * Steps:
 * 1. create a new cMessage
 * 2. set it's frametype and the ACK number accordingly
 * 3. schedule the ACK to be sent after processing
 *
 */
void Node::sendAck()
{
    // TODO: send the ack of the received protocol

    // create a new message for the acknowledgement and set its name to simulate the message's processing time
    CustomMessage_Base *ack = new CustomMessage_Base("End Processing");

    // set the ACK information
    ack->setFrameType(1);         // sets the framtype to the ack's frametype
    ack->setAckNack(n_ackNumber); // sets the messages's ack number to the next expected frame

    EV << "ACK " << n_ackNumber << " START PROCESSING" << endl;

    scheduleAt(simTime() + PT, ack);
}

/**
 * @brief Process the NACK and schedule to send it
 *
 * Steps:
 * 1. create a new cMessage
 * 2. set it's frametype and the NACK number accordingly
 * 3. set the parameter nackSent to indicate that a NACK has been sent => avoids sending multiple NACKs for the same message
 * 4. schedule the NACK to be sent after processing
 *
 */
void Node::sendNack()
{
    // TODO: send the ack of the received protocol

    // create a new message for the nack and set its name to simulate the message's processing time
    CustomMessage_Base *nack = new CustomMessage_Base("End Processing");

    // set the NACK message information
    nack->setFrameType(0);         // sets the framtype to the nack's frametype
    nack->setAckNack(n_ackNumber); // sets the messages's ack number to the frame that should be resent

    EV << "NACK " << n_ackNumber << " START PROCESSING" << endl;
    nackSent = true; // indicates that a NACK was sent by the receiver but still hasn't received the correct message

    scheduleAt(simTime() + PT, nack); // schedule to send message after ending processing
}

/**
 * @brief Receives the ACK from the receiver's node and updates the window indices accordingly
 *
 * Steps:
 * 1. get the ACK number from the message and calculate the sequence number of the ACKed frame
 * 2. check if the ACK handles one or more ACKs and update the window indices with the number of messages ACKed
 * 3. cancel all the timer of the ACKed messages and set them to false
 * 4. update the timer indices with the number of messages ACKed
 *
 * @param msg the received frame from the sender
 * @return bool whether the received ack is the expected ack or not
 */
bool Node::receiveAck(CustomMessage_Base *msg)
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

    // then check all the previuos messages to handle accumulative ACKS
    // in case any of the acks got lost in the process but the timer hasn't set off yet
    EV << "CANCELING AND DELETING TIMEOUT EVENT FOR CURRENT AND PREVIOUS ACKS" << endl;

    // loops on all previous messages from the sequence number (ACKed frame) to the start of the window
    for (int i = sequenceNumber; i >= startTimer; i--)
    {
        if (timers[i].first) // if the timer is not false (still on)
        {
            EV << "CANCELING AND DELETING TIMEOUT EVENT FOR " << i << " " << timers[i].second->getName() << endl;

            cancelEvent(timers[i].second); // cancel the timer
            timers[i].first = false;       // set its parameter to false

            // slide the window to the right one step
            // checks that the end of the window still didn't reach the end of the messages
            // if it is at the end then we shouldn't increment it
            if (end < --messagesToSend.end()) // if the end index still hasn't reached the end of the messages
            {
                // increment the start window index
                startWindow++;
                start++;

                // increment the end window index
                endWindow++;
                end++;
            }
            messagesCancelled++; // increment the messages cancelled => this handles if the received ack is not the expected one
        }
        else // then all the previous timers are cancelled as well => break from the loop
        {
            break;
        }
    }

    EV << "NUMBER OF MESSAGES CANCELLED: " << messagesCancelled << endl;

    // adjust the indices of the timer vector according to the number of messages cancelled (ACKed)
    startTimer = (startTimer + messagesCancelled) % (WS + 1);
    endTimer = (endTimer + messagesCancelled) % (WS + 1);

    EV << "START TIMER INDEX " << startTimer << endl;
    EV << "END TIMER INDEX " << endTimer << endl;

    EV << "DONE CANCELLING" << endl;
    cancelAndDelete(msg); // camcel and delete the message received

    if (messagesCancelled == 0) // if no messages were cancelled then the received ACK is not the expected ACK
        return false;

    // the received ACK is the expected one
    return true;
}

// ---------------------------------- FILE FUNCTIONS ---------------------------------- //

/**
 * @brief Reads the error codes and messages from the input file specified
 *
 * @param fileName the file name from which we should read from
 */
void Node::readFile(std::string fileName)
{
    // TODO: read the input file

    // declare and open the file
    std::ifstream inputFile;
    inputFile.open("../inputs/" + fileName);

    // declarations of temporary variables to read the different parts of the line
    std::string message;
    std::string errorCode;

    if (inputFile.is_open()) // check if the file is opened
    {
        while (!inputFile.eof()) // read all the lines till the end of the file
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

    inputFile.close(); // close the file
}

// ---------------------------------- PROTOCOL FUNCTIONS ---------------------------------- //
/**
 * @brief Given a string of the apply the byte stuffing algorithm to frame the message
 *
 * @param message the message string to be framed
 * @return string the framed message after applying byte stuffing
 */
std::string Node::frameMessage(std::string message)
{
    // TODO: given a string (message) return the framed message to be sent

    // add the flag byte at the beginning
    std::string framedMessage = "$";

    // apply byte stuffing to the whole message
    for (int i = 0; i < message.length(); i++)
    {
        if (message[i] == '/' || message[i] == '$')
            framedMessage += '/';
        framedMessage += message[i];
    }

    // add the flag byte at the end
    framedMessage += '$';

    // return the message after framing
    return framedMessage;
}

/**
 * @brief Given a string of the framed message reverse the byte stuffing algorithm to obtain the original message
 *
 * @param message the framed payload after receinving the message on the receiver's side
 * @return string the original message after deframing it
 **/
std::string Node::deframeMessage(std::string message)
{
    // TODO: given a framed message return the original message

    std::string originalMessage = ""; // stores the original message after deframing

    // loop an all the characters and removes added flags and slashes
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

    // return the original message
    return originalMessage;
}

/**
 * @brief Given a string of the framed message apply the checksum algorithm to obtain the pairty byte
 *
 * Steps:
 * 1. convert the message into its binary representation
 * 2. add all bytes of binary message
 * 3. get the one's complement of the final sum
 *
 * @param message the message after applying the framing
 * @return bitset<8> parity byte checksum in bits of the message
 */
std::bitset<8> Node::calculateChecksum(std::string message)
{
    std::vector<std::bitset<8>> sums; // stores the sums of each byte with the next one
    std::bitset<8> zero(0);           // used as an initialization and comparator

    sums.push_back(zero); // add the zero to the start of the vector to add it to the first byte

    // converts the string into its binary representaion and divides it into a vector of characters
    // each character is represented in 8 bits
    std::vector<std::bitset<8>> binaryMessage = this->convertToBinary(message);

    int counter = 0; // initialize counter to access bytes in the sums vector

    // loop on all bytes in the returned binary message
    for (std::bitset<8> byte : binaryMessage)
    {
        sums.push_back(this->byteAdder(sums[counter], byte)); // adds the current byte to the previuos one
        counter++;                                            // increment counter
    }

    std::bitset<8> checksum(sums[sums.size() - 1]); // get the last element in the vector => final sum of the message
    checksum = checksum.flip();                     // get the one's complement of the sum

    // return the final checksum
    return checksum;
}

/**
 * @brief Given a string of the framed message apply the checksum algorithm to obtain the pairty byte
 *
 * Steps:
 * 1. convert the message into its binary representation
 * 2. add all bytes of binary message
 * 3. get the one's complement of the final sum
 * 4. convert the received checksum to it binary representaion
 * 5. add the received checksum to the calculated checksum
 * 6. compare the final sum to 0
 *
 * @param message the message after applying the framing
 * @param checksum the checksum sent with the message (calculated at the sender's side)
 * @return bool whether the message contains an error or not
 */
bool Node::detectMessageError(std::string message, int checksum)
{
    // TODO: calcuate the checksum at the receiver to detect if there's an error

    std::vector<std::bitset<8>> sums; // stores the sums of each byte with the next one
    std::bitset<8> zero(0);           // used as an initialization and comparator

    sums.push_back(zero); // add the zero to the start of the vector to add it to the first byte

    // convert sent received checksum to its binary representaion
    std::bitset<8> receivedChecksum(checksum);

    // converts the string into its binary representaion and divides it into a vector of characters
    // each character is represented in 8 bits
    std::vector<std::bitset<8>> binaryMessage = this->convertToBinary(message);

    int counter = 0; // initialize counter to access bytes in the sums vector

    // loop on all bytes in the returned binary message
    for (std::bitset<8> byte : binaryMessage)
    {
        sums.push_back(this->byteAdder(sums[counter], byte)); // adds the current byte to the previuos one
        counter++;                                            // increment counter
    }

    // add the received checksum to the calculated checksum
    std::bitset<8> finalSum = this->byteAdder(sums[sums.size() - 1], receivedChecksum);

    // get the one's complement and compare it to 0
    if (finalSum.flip() == zero)
    {
        return false;
    }
    return true;
}

/**
 * @brief Given the error codes read from the file sets the types of errors that the message should encounter
 *
 * @param modification passed by reference => 1 indicates that the frame should be modified
 * @param loss passed by reference => 1 indicates that the frame should be lost
 * @param duplication passed by reference => 1 indicates that the frame should be duplicated
 * @param delay passed by reference => 1 indicates that the frame should be delayed
 **/
void Node::getErrors(std::string errorCode, bool &modification, bool &loss, bool &duplication, bool &delay)
{
    /**
     * BITS POSITIONS
     * 0 -> Modification
     * 1 -> Loss
     * 2 -> Duplication
     * 3 -> Delay
     **/

    if (errorCode[0] == '1') // modification bit
        modification = true;

    if (errorCode[1] == '1') // loss bit
        loss = true;

    if (errorCode[2] == '1') // duplication bit
        duplication = true;

    if (errorCode[3] == '1') // delay bit
        delay = true;
}

/**
 * @brief Schedules the frame to be sent with the specified delays according to the given errors
 *
 * @param errorCode the error code associated with the frame that should be sent
 * @param msg the frame that should be sent
 **/
void Node::sendMessage(std::string errorCode, CustomMessage_Base *msg)
{
    // error bits
    bool modification = false;
    bool loss = false;
    bool duplication = false;
    bool delay = false;
    int modifiedBit;

    // get the trailer to write to the output log file
    std::bitset<8> trialer(msg->getTrailer());

    // set the error bits according to the error code read from the file
    getErrors(errorCode, modification, loss, duplication, delay);

    EV << "ERROR CODE: " << errorCodes[currentIndex] << " MODIFICATION: " << modification << " LOSS: " << loss << " DUPLICATION: " << duplication << " DELAY: " << delay << endl;

    // write to the output log file
    // this is the start of the string to be written
    // we should add the rest of the output message according to the types of errors associated with this frame in the if statements
    std::string log;

    if (loss) // if the frame is supposed to be lost
    {
        // check for other errors to write the errors to the output file
        if (modification)
            modifiedBit = modifyMessage(msg);
        else
            modifiedBit = -1;

        int delayError = 0;
        if (delay)
            delayError = ED;

        // write to the output file
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [Yes], DUPLICATE [0], DELAY [" + std::to_string(delayError) + "] \n"; //
        Logger::write(log);

        // return without scheduling anything
        return;
    }

    if (modification && duplication && delay)
    {
        // TODO: handle this error case

        // 1. modify message
        modifiedBit = modifyMessage(msg);

        // 2. send modified delayed message
        scheduleAt(simTime() + TD + ED, msg);
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        Logger::write(log + "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [1], DELAY [" + std::to_string(ED) + "] \n");

        // 3. send duplicated modified delayed message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + ED + DD, dupliactedMessage);

        // write to the output log file
        log = "At time [" + (simTime() + DD).str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        Logger::write(log + "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [2], DELAY [" + std::to_string(ED) + "] \n");

        return;
    }

    if (modification && duplication)
    {
        // TODO: handle this error case

        // 1. modify message
        modifiedBit = modifyMessage(msg);

        // 2. send modified message
        scheduleAt(simTime() + TD, msg);
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [1], DELAY [0] \n";
        Logger::write(log);

        // 3. send duplicated modified message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + DD, dupliactedMessage);

        // write to the output log file
        log = "At time [" + (simTime() + DD).str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [2], DELAY [0] \n";
        Logger::write(log);

        return;
    }

    if (modification && delay)
    {
        // TODO: handle this error case

        // 1. modify message
        modifiedBit = modifyMessage(msg);

        // 2. send delayed modified message
        scheduleAt(simTime() + TD + ED, msg);

        // write to the output log file
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [0], DELAY[" + std::to_string(ED) + "] \n";
        Logger::write(log);

        return;
    }

    if (duplication && delay)
    {
        // TODO: handle this error case

        // 1. delay the message
        scheduleAt(simTime() + TD + ED, msg);
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [-1], LOST [No], DUPLICATE [1], DELAY [" + std::to_string(ED) + "]\n";
        Logger::write(log);

        // 2. duplicate the message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + ED + DD, dupliactedMessage);

        // write to the output log file
        log = "At time [" + (simTime() + DD).str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [-1], LOST [No], DUPLICATE [2], DELAY [" + std::to_string(ED) + "]\n";
        Logger::write(log);
        return;
    }

    if (modification)
    {
        // TODO: handle this error case

        // 1. modify message
        modifiedBit = modifyMessage(msg);

        // 2. send modified message
        scheduleAt(simTime() + TD, msg);

        // write to the output log file
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [" + std::to_string(modifiedBit) + "], LOST [No], DUPLICATE [0], DELAY [0] \n";
        Logger::write(log);

        return;
    }

    if (duplication)
    {
        // TODO: handle this error case

        // 1. send original message
        scheduleAt(simTime() + TD, msg);

        // write the first version of the message to the output log file
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [-1], LOST [No], DUPLICATE [1], DELAY [0] \n";
        Logger::write(log);

        // 2. send duplicated message
        CustomMessage_Base *dupliactedMessage = msg->dup();
        scheduleAt(simTime() + TD + DD, dupliactedMessage);

        // write the first version of the message to the output log file
        log = "At time [" + (simTime() + DD).str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [-1], LOST [No], DUPLICATE [2], DELAY [0] \n";
        Logger::write(log);

        return;
    }

    if (delay)
    {
        // TODO: handle this error case

        // send the delayed message
        scheduleAt(simTime() + TD + ED, msg);

        // write the to the output log file
        log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
        log += "MODIFIED [-1], LOST [No], DUPLICATE [0], DELAY [" + std::to_string(ED) + "] \n";
        Logger::write(log);

        return;
    }

    // send the message without any errors after the TD specified if the error code = 0000
    scheduleAt(simTime() + TD, msg);

    // write to the output file
    log = "At time [" + simTime().str() + "] NODE[" + std::to_string(nodeID) + "] [SENT] frame with SEQ_NUM[" + std::to_string(msg->getHeader()) + "] and PAYLOAD[" + msg->getPayload() + "] and TRAILER[" + trialer.to_string() + "] ";
    log += "Modified [-1], Lost [No], Duplicate [0], Delay [0] \n";
    Logger::write(log);
}

/**
 * @brief modifies a single bit in the message to simulate the frame sent being corrupted
 *
 * @param msg the frame that should be
 * @return int the modified bit
 */
int Node::modifyMessage(CustomMessage_Base *msg) // get the payload of the frame
{
    std::string payload = msg->getPayload();

    EV << "TEST MODIFY MESSAGE" << endl;
    EV << "Payload: " << payload << endl;

    // convert the message into binary vector
    std::vector<std::bitset<8>> binaryBitset = this->convertToBinary(payload);
    std::string binaryMessage = "";

    // convert the vector to a continuous string
    for (int i = 0; i < binaryBitset.size(); i++)
    {
        binaryMessage += binaryBitset[i].to_string();
    }

    // get a random bit in the binary message to modify
    int errorBit = int(uniform(0, binaryMessage.length()));

    // toggle the error bit
    if (binaryMessage[errorBit] == '0')
        binaryMessage[errorBit] = '1';
    else
        binaryMessage[errorBit] = '0';

    // convert the message to its original form again
    std::string modifiedMessage = "";
    std::istringstream in(binaryMessage);
    std::bitset<8> bs;
    while (in >> bs)
    {
        modifiedMessage += char(bs.to_ulong());
    }

    // set the payload with the new modified message
    char *modifiedCharacters = new char[modifiedMessage.length() + 1];
    std::strcpy(modifiedCharacters, modifiedMessage.c_str());
    msg->setPayload(modifiedCharacters);

    EV << "MODIFIED MESSAGE: " << modifiedMessage << " " << msg->getPayload() << " ERROR BIT: " << errorBit << endl;

    // return the modified error bit
    return errorBit;
}

/**
 * @brief Checks if the received message is duplicated
 *
 * @param msg the frame that should be sent
 * @return bool whether the message is a duplicate or not
 */
bool Node::isDuplicate(CustomMessage_Base *msg)
{
    // get the original payload of the message first
    std::string deframedMessage = deframeMessage(msg->getPayload());
    char *tempPayload = new char[deframedMessage.length() + 1];
    std::strcpy(tempPayload, deframedMessage.c_str());

    EV << "CHECKING DUPLICATES" << endl;

    // loops on all the received messages and compares it to the current message
    for (int i = 0; i < receivedMessages.size(); i++)
    {
        EV << receivedMessages[i]->getHeader() << " " << receivedMessages[i]->getPayload() << " " << receivedMessages[i]->getTrailer() << endl;

        // skip if the headers are not the same
        if (receivedMessages[i]->getHeader() != msg->getHeader())
            continue;

        // return true if the message is received before
        if (std::strcmp(tempPayload, receivedMessages[i]->getPayload()) == 0 && msg->getTrailer() == receivedMessages[i]->getTrailer())
        {
            EV << "DUPLICATED MESSAGE" << endl;
            return true;
        }
    }

    // the message is not received before
    return false;
}

/**
 * @brief Simulates the ACKS/NACKS being lost in the program according to the loss probability
 *
 * @param LP the loss probability specified in the initial of the program
 * @return bool whether the ACK should be list or not
 */
bool Node::isLost(double LP)
{
    srand(static_cast<int>(simTime().inUnit(SIMTIME_S) + 0.5)); // changes the seed according to the time of the program

    double randomProbability; // random probability that will indicate if the ACK/NACK is being lost or not

    // changes the random probabilty for each ACK/NACK being sent
    for (int i = 0; i < 3; i++)
    {
        randomProbability = ((double)rand()) / RAND_MAX;
    }

    EV << "PROBABILITY: " << randomProbability << endl;

    // if the random probaility generated is less than or equal the LP given then the ACK/NACK should be lost
    if (randomProbability <= LP)
    {
        return true;
    }

    // the ACK/NACK should not be lost
    return false;
}

/**
 * @brief Starts the timer for the next frame to be sent
 *
 * @param msg the frame that should be sent
 */
void Node::startMessageTimer(CustomMessage_Base *msg)
{
    // get the sequence number of the frame
    int sequenceNumber = msg->getHeader();

    // if there is already a timer in the
    if (timers[sequenceNumber].first)
        return;

    // should start timer of the message sent
    EV << "STARTED TIMER FOR MESSAGE, TO = " << TO << " " << sequenceNumber << " " << msg->getPayload() << endl;

    // starts the timer at the index of the sequence number
    timers[sequenceNumber].first = true;                      // there is a timer scheduled
    timers[sequenceNumber].second->setHeader(sequenceNumber); // set the header of the timer to indicate the sequence number associated with the current timer

    scheduleAt(simTime() + TO, timers[sequenceNumber].second);
}

/**
 * @brief Updates current index to resend the messages in the window
 *
 * @param sequenceNumber the sequence number of the frame from which we need to start resending
 */
void Node::resendMessages(int sequenceNumber)
{
    // move the current index to the message that timed out in the window
    currentIndex = startWindow;
    current = start;

    while ((currentIndex % (WS + 1)) != sequenceNumber)
    {
        current++;
        currentIndex++;
    }

    // resend the timeout messages without any errors
    errorCodes[currentIndex] = "0000";

    EV << "START TIMER " << startTimer << " END TIMER " << endTimer << endl;

    // cancel the timers for all the subsequent messages in the window
    int index = startTimer;

    EV << "CANCELING TIMEOUT MESSAGES" << endl;

    while (index != endTimer)
    {
        if (timers[index].first) // if a timer has started for the message with the same sequence number
        {
            EV << "CANCELLING MESSAGE WITH SEQ_NUM " << index << " AND PAYLOAD " << timers[index].first << endl;

            cancelEvent(timers[index].second); // cancel the timer
            timers[index].first = false;       // set that the timer is off
        }
        index = ((index + 1) % (WS + 1)); // update the index to move to the next place in the window => ensure that the index wrops around when reaching the end of the window
    }

    // handles the last element as the loop exits when it reaches the end timer index without cancelling it
    index = index % (WS + 1);
    if (timers[index].first)
    {
        EV << "CANCELLING MESSAGE WITH SEQ_NUM " << index << " AND PAYLOAD " << timers[index].first << endl;

        cancelEvent(timers[index].second);
        timers[index].first = false;
    }
}

// ---------------------------------- UTILITY FUNCTIONS ---------------------------------- //
/**
 * @brief Given a string of characters convert it to its binary representation
 *
 * @param String string of characters that should be converted to binary
 * @return bitset<8> the binary representation of String divided into vector of bytes => each character is represented in one byte
 */
std::vector<std::bitset<8>> Node::convertToBinary(std::string String)
{
    // TODO: given a string (message) return a vector of its binary representation

    std::vector<std::bitset<8>> binaryMessage; // stores the final binary message

    // loop on each character in the string and convert it to 8-bit binary bitset
    for (int i = 0; i < String.length(); i++)
    {
        char character = String[i];
        std::bitset<8> asciiBitset(character);
        binaryMessage.push_back(asciiBitset);
    }

    // return the converted message
    return binaryMessage;
}

/**
 * @brief Given a string of characters convert it to its binary representation
 *
 * @param String string of characters that should be converted to binary
 * @return bool the binary representation of String divided into vector of bytes => each character is represented in one byte
 */
bool Node::fullAdder(bool firstBit, bool secondBit, bool &carry)
{
    //    EV << "previous carry: " << carry;
    bool sum = firstBit ^ secondBit ^ carry;
    carry = (firstBit & secondBit) | ((firstBit ^ secondBit) & carry);
    //    EV << " FB:" <<firstBit << " SB:" << secondBit << " bitSum: " << sum << " bitCarry: " << carry << endl;
    return sum;
}

/**
 * @brief Adds the two bytes given => bit wise
 *
 * @param firstByte, secondByte string of characters that should be converted to binary
 * @return bitset<8> the sum of the firstByte and secondByte as a bitset
 **/
std::bitset<8> Node::byteAdder(std::bitset<8> firstByte, std::bitset<8> secondByte)
{
    std::bitset<8> byteSum(0); // initialize the sum
    bool carry = false;        // the carry from the full adder

    // loops to add the bits using full adder
    for (int i = 0; i < 8; i++)
    {
        byteSum[i] = this->fullAdder(firstByte[i], secondByte[i], carry);
    }

    // handle overflow if the final carry is 1
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

    // return the final sum
    return byteSum;
}

/**
 * DESTRUCTOR
 **/
Node::~Node()
{
    // delete all allocated messages
    for (auto message : receivedMessages)
    {
        delete message;
        message = nullptr;
    }
    receivedMessages.clear();

    for (auto &timer : timers)
    {
        if (timer.first)
            cancelAndDelete(timer.second);
        else
        {
            delete timer.second;
            timer.second = nullptr;
        }
    }
    timers.clear();
}
