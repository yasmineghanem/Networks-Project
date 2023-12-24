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

#include "Logger.h"

Logger Logger::instance;

Logger::Logger()
{
    // TODO Auto-generated constructor stub
}

Logger::~Logger()
{
    // TODO Auto-generated destructor stub
}

void Logger::open(const std::string &logFile)
{
    if (instance.fileStream.is_open())
    {
        return;
    }
    instance.fileStream.open(logFile.c_str(), std::ios::out | std::ios::trunc);
    if (instance.fileStream.is_open())
    {
        std::cout << "File opened successfully" << std::endl;
    }
    else
    {
        std::cout << "Error opening file" << std::endl;
    }
}

void Logger::close()
{
    if (instance.fileStream.is_open())
    {
        std::cout << "closing file" << std::endl;

        instance.fileStream.close();

        if (!instance.fileStream.is_open())
        {
            std::cout << "File Closed" << std::endl;
        }
    }

    // if (instance.fileStream.is_open())
    // {
    //     std::cout << "File still open" << std::endl;
    // }
    // else
    // {
    //     std::cout << "File closed" << std::endl;
    // }
}

void Logger::write(const std::string &message)
{
    std::ostream &stream = instance.fileStream;
    stream << message << std::endl;
    stream.flush();
}
