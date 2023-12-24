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

#ifndef TIMER_H_
#define TIMER_H_

#include <omnetpp.h>
#include <thread>
#include <chrono>
#include <atomic>

class Timer
{
private:
    std::atomic<bool> expired;
    std::atomic<bool> canceled;
    std::thread timerThread;

    void timerThreadFunc(int seconds);

public:
    Timer();
    virtual ~Timer();

    void start(int seconds);
    void cancel();
    bool isExpired() const;
};

#endif /* TIMER_H_ */
