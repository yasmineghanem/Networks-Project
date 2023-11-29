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

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <string>
#include <omnetpp.h>

class Message: public omnetpp::cMessage {
    private:
        int header;           // the sequence number of the frame
        std::string payload;  // the actual message to be sent (characters)
        int trailer;          // the parity byte calculated
        int frameType;        // specifies the type of the message frame -> Data=2, ACK=1, NACK=0.
        int number;           // lesa hanshoof

    public:
        Message(const char *name = nullptr, int kind = 0) : cMessage(name, kind) {}
        Message();
        virtual ~Message();

        // setters
        void setHeader(int sequenceNumber);
        void setPayload(std::string payload);
        void setTrailer(int parityByte);
        void setFrameType(int frameType);

        //getters
        int getHeader();
        std::string getPayload();
        int getTrailer();
        int getFrameType();

};

#endif /* MESSAGE_H_ */
