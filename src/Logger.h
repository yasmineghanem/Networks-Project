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

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <fstream>
#include <iostream>

class Logger
{
public:
    Logger();
    virtual ~Logger();
    static void open(const std::string &logFile);  // opens the given file
    static void close();                           // closes the file instance
    static void write(const std::string &message); // writes the given message to the file

private:
    std::ofstream fileStream; // object of fileStream to handle the file
    static Logger instance;   // the instance that handles the file operations
};

#endif /* LOGGER_H_ */
