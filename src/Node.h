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

#include "Message.h"

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
  int WS;

  // receiver
  std::vector<Message *> receivedMessages;
  int ackNumber = 0;

  // system indices
  int startWindow;
  int endWindow;
  int currentIndex;

  // system pointers
  std::vector<std::string>::iterator start;
  std::vector<std::string>::iterator end;

  static int sender;

  // Delays in the system
  double PT;
  double ED;
  double TD;

  virtual void initialize();
  virtual void handleMessage(cMessage *msg);

public:
  // main functions
  void processDataToSend();
  void processReceivedData(Message *msg);
  void sendAck();
  void sendNack();
  void receiveAck(Message *msg);
  void receiveNack(Message *msg);

  // dealing with files functions
  void readFile(std::string fileName);
  void writeFile();

  // GoBackN protocol functions
  void addHeader(std::string message);                                     // TODO: adds the header to the message to be sent
  std::string frameMessage(std::string message);                           // TODO: takes a message and frames it using byte stuffing
  std::bitset<8> calculateChecksum(std::string message);                   // TODO: calculates the message checksum as an error detection technique
  bool detectError(std::string message, int checksum);                     // TODO: calculates the message checksum as an error detection technique
  void handleError(std::string errorCode, bool &, bool &, bool &, bool &); // TODO: given an error code handles the message accordingly
  std::string deframeMessage(std::string message);                         // TODO: defram the received message

  // utility functions
  std::vector<std::bitset<8>> convertToBinary(std::string String); // TODO: converts any given string to binary
  void sendMessage(std::string message, int time);                 // TODO: send the given message at the specified time
  void printVector(std::vector<std::bitset<8>>);                   // TODO: print a given vector
  bool fullAdder(bool, bool, bool &);
  std::bitset<8> byteAdder(std::bitset<8>, std::bitset<8>);
};

#endif
