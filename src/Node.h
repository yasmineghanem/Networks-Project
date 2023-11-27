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

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:
    int nodeID;
    std::vector<std::string> messagesToSend;
    static int sender;

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    // main functions
    void sending();
    void receiving();

    // dealing with files functions
    void readFile(std::string fileName);
    void writeFile();

    // GoBackN protocol functions
    void addHeader(std::string message);             // TODO: adds the header to the message to be sent
    std::string frameMessage(std::string message);   // TODO: takes a message and frames it using byte stuffing
    void calculateChecksum(std::string message);     // TODO: calculates the message checksum as an error detection technique
    void handleError(std::bitset<4> errorCode);      // TODO: given an error code handles the message accordingly

    // utility functions
    std::vector<std::bitset<8> > convertToBinary(std::string String);        // TODO: converts any given string to binary
    void sendMessage(std::string message, int time); // TODO: send the given message at the specified time
    void printVector(std::vector<std::bitset<8> >);                // TODO: print a given vector

};

#endif
