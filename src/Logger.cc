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

/**
 * Opens the file to write into
 *
 * @param logFile the file name that should be written into
 **/
void Logger::open(const std::string &logFile)
{
    // checks if the file is already open to not open it again (this is done because two nodes will be accessing the same file)
    if (instance.fileStream.is_open())
    {
        return;
    }

    // open the file and erase what is already there
    instance.fileStream.open(logFile.c_str(), std::ios::out | std::ios::trunc);

    // ensure that the file is opened succussfully
    if (instance.fileStream.is_open())
    {
        std::cout << "File opened successfully" << std::endl;
    }
    else
    {
        std::cout << "Error opening file" << std::endl;
    }
}

/**
 * Closes the file opened
 *
 * @param logFile the file name that should be written into
 **/
void Logger::close()
{
    // checks if the file is open to close it (done to handle the two nodes accessing the same file)
    if (instance.fileStream.is_open())
    {
        std::cout << "closing file" << std::endl;

        instance.fileStream.close(); // close the file

        // ensure that the file is closed
        if (!instance.fileStream.is_open())
        {
            std::cout << "File Closed" << std::endl;
        }
    }
}

/**
 * Writes the given message to the output file
 *
 * @param message the message to be written to the file
 **/
void Logger::write(const std::string &message)
{
    std::ostream &stream = instance.fileStream;
    stream << message << std::endl;
    stream.flush();
}
