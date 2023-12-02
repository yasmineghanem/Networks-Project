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

#include "Message.h"

Message::Message()
{
    // TODO Auto-generated constructor stub
}

Message::~Message()
{
    // TODO Auto-generated destructor stub
}

void Message::setHeader(int sequenceNumber)
{
    this->header = sequenceNumber;
}

void Message::setPayload(std::string payload)
{
    this->payload = payload;
}

void Message::setTrailer(int parityByte)
{
    this->trailer = parityByte;
}

void Message::setFrameType(int frameType)
{
    this->frameType = frameType;
}

void Message::setAckNack(int number)
{
    this->ackNack = number;
}

int Message::getHeader()
{
    return header;
}

std::string Message::getPayload()
{
    return payload;
}

int Message::getTrailer()
{
    return trailer;
}

int Message::getFrameType()
{
    return frameType;
}

int Message::getAckNack()
{
    return ackNack;
}
