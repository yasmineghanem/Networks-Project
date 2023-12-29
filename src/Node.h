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

#ifndef __GOBACKN_NODE_H_
#define __GOBACKN_NODE_H_

#include <omnetpp.h>
#include <string>
#include <vector>
#include <fstream>
#include <bitset>
#include <sstream>
#include <ctime>
#include <random>
#include <chrono>
#include <thread>
#include <memory>
#include <algorithm>

#include "customMessage_m.h"
#include "Logger.h"
#include "Timer.h"

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
private:
  int nodeID; // the id of the node

protected:
  // sender
  std::vector<std::string> messagesToSend;                   // messages queue that should be processed and sent
  std::vector<std::string> errorCodes;                       // the error codes corresponding tpo each message
  std::vector<std::pair<bool, CustomMessage_Base *>> timers; // the timers for each message sent
  bool expectedACK;                                          // bool to indicate whether the received ACK is the expected one by the sender

  // receiver
  std::vector<CustomMessage_Base *> receivedMessages; // a vector containing the received messages
  int n_ackNumber = 0;                                // the expected ack number
  bool nackSent = false;                              // bool indicating if a NACK has been sent by the receiver

  // system indices
  int startWindow; // the start index of the sending window
  int endWindow;   // the end index of the sending window
  int currentIndex;

  // system pointers
  // messages pointers
  std::vector<std::string>::iterator start;
  std::vector<std::string>::iterator end;
  std::vector<std::string>::iterator current;

  // timers indices
  int startTimer;
  int endTimer;

  // shared variables
  static int sender; // the ID of the sending node

  // system parameters
  int WS;    // sender's window size
  double PT; // message processing time
  double ED; // message error delay
  double TD; // message transmission delay
  double DD; // message duplication delay
  double TO; // timeout duration
  double LP; // ack/nack loss probabilty

  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
  virtual void finish();

public:
  Node(){};
  virtual ~Node();

  // main functions
  void processDataToSend();                          // processes the next frame to be sent
  void processReceivedData(CustomMessage_Base *msg); // processes the received frame
  void sendAck();                                    // sends the ACK on the correct received message
  void sendNack();                                   // sends NACK for the errored message
  bool receiveAck(CustomMessage_Base *msg);          // receives the ACK and update window indices

  // GoBackN protocol functions
  void addHeader(std::string message);                                                                   // adds the header to the message to be sent
  std::string frameMessage(std::string message);                                                         // takes a message and frames it using byte stuffing
  std::bitset<8> calculateChecksum(std::string message);                                                 // calculates the message checksum as an error detection technique
  bool detectMessageError(std::string message, int checksum);                                            // calculates the message checksum as an error detection technique
  void getErrors(std::string errorCode, bool &modification, bool &loss, bool &duplication, bool &delay); // given an error code handles the message accordingly
  void sendMessage(std::string errorCode, CustomMessage_Base *msg);                                      // given an error code handles the message accordingly
  std::string deframeMessage(std::string message);                                                       // deframes the received message
  int modifyMessage(CustomMessage_Base *msg);                                                            // modifies the message to simulate the frame being corrupted
  bool isDuplicate(CustomMessage_Base *msg);                                                             // checks if the received frame has been received before
  bool isLost(double LP);                                                                                // simulates the losing of the ACK/NACK
  void startMessageTimer(CustomMessage_Base *msg);                                                       // starts the timer for the given frame to be sent
  void resendMessages(int sequenceNumber);                                                               // updates the current index to resend messages when they timeout

  // utility functions
  void readFile(std::string fileName);
  std::vector<std::bitset<8>> convertToBinary(std::string String); // converts any given string to binary
  // void sendMessage(std::string message, int time);                 // send the given message at the specified time
  bool fullAdder(bool, bool, bool &);                       // adds two bits with carry
  std::bitset<8> byteAdder(std::bitset<8>, std::bitset<8>); // adds two bytes given in binary format
};

#endif
