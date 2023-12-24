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

#include "Timer.h"
#include <iostream>

Timer::Timer() : expired(false), canceled(false) {}

Timer::~Timer()
{
    // Make sure the timer thread is joined before destruction
    if (timerThread.joinable())
    {
        timerThread.join();
    }
}

void Timer::start(int seconds)
{
    std::cout << "TIMER HAS STARTED AT " << omnetpp::simTime() << std::endl;
    expired = false;
    canceled = false;
    timerThread = std::thread(&Timer::timerThreadFunc, this, seconds);
}

void Timer::cancel()
{
    canceled = true;
    expired = false;

    // Join the timer thread to ensure proper cleanup
    if (timerThread.joinable())
    {
        timerThread.join();
    }
}

bool Timer::isExpired() const
{
    return expired;
}

void Timer::timerThreadFunc(int seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds));

    if (!canceled)
    {
        expired = true;
        std::cout << "Timer expired at " << omnetpp::simTime() << std::endl;
    }
}
