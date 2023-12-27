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
  int nodeID;

protected:
  // sender
  std::vector<std::string> messagesToSend;
  std::vector<std::string> errorCodes;
  std::vector<std::pair<bool, CustomMessage_Base *>> timers;
  bool expectedACK;
  bool resending;

  // receiver
  std::vector<CustomMessage_Base *> receivedMessages;
  int n_ackNumber = 0;
  bool nackSent = false;

  // system indices
  int startWindow;
  int endWindow;
  int currentIndex;

  // system pointers
  // messages pointers
  std::vector<std::string>::iterator start;
  std::vector<std::string>::iterator end;
  std::vector<std::string>::iterator current;

  // error codes pointers
  // std::vector<std::string>::iterator startError;
  // std::vector<std::string>::iterator endError;
  // std::vector<std::string>::iterator currentError;

  // timers
  int startTimer;
  int endTimer;

  // shared variables
  static int sender;

  // system parameters
  int WS;
  double PT; // message processing time
  double ED; // message error delay
  double TD; // message transmission delay
  double DD; // message duplication delay
  double TO; // timeout
  double LP; // ack/nack loss probabilty

  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
  virtual void finish();

public:
  Node(){};
  virtual ~Node();

  // main functions
  void processDataToSend();
  void processReceivedData(CustomMessage_Base *msg);
  void sendAck();
  void sendNack();
  bool receiveAck(CustomMessage_Base *msg);
  void receiveNack(CustomMessage_Base *msg);

  // GoBackN protocol functions
  void addHeader(std::string message);                                                                   // TODO: adds the header to the message to be sent
  std::string frameMessage(std::string message);                                                         // TODO: takes a message and frames it using byte stuffing
  std::bitset<8> calculateChecksum(std::string message);                                                 // TODO: calculates the message checksum as an error detection technique
  bool detectError(std::string message, int checksum);                                                   // TODO: calculates the message checksum as an error detection technique
  void getErrors(std::string errorCode, bool &modification, bool &loss, bool &duplication, bool &delay); // TODO: given an error code handles the message accordingly
  void handleErrors(std::string errorCode, CustomMessage_Base *msg);                                     // TODO: given an error code handles the message accordingly
  std::string deframeMessage(std::string message);                                                       // TODO: defram the received message
  int modifyMessage(CustomMessage_Base *msg);
  bool checkDuplicate(CustomMessage_Base *msg);
  bool loseAck(double LP);
  void handleTimeout(CustomMessage_Base *msg);
  void resendTimeoutMessages(int seq_num);

  // utility functions
  void readFile(std::string fileName);
  std::vector<std::bitset<8>> convertToBinary(std::string String); // TODO: converts any given string to binary
  void sendMessage(std::string message, int time);                 // TODO: send the given message at the specified time
  bool fullAdder(bool, bool, bool &);
  std::bitset<8> byteAdder(std::bitset<8>, std::bitset<8>);
};

#endif
