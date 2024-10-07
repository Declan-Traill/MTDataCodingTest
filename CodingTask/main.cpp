#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <json/json.h>
#include <boost/asio.hpp>
#include <chrono>
#include <thread>
#include <ctime>

using namespace std;          // Allows omitting the 'std::' prefix
using namespace boost::asio;  // Allows omitting the 'boost::asio::' prefix

// Constant definitions used in these functions
//
#define BAUD_RATE               2400            // Baud rate to use for the serial port
#define READ_TIMEOUT_MS         1000            // Timeout for file read (in milliseconds)
#define START_PACKET_STRING     "/\r\n"         // Start of packet
#define END_PACKET_STRING       "\\\r\n"        // End of packet
#define MASS_UNIT_STRING        "Kg"            // Mass unit string
#define JSON_CHANNELS_KEY       "channels"      // JSON key for channels
#define JSON_TOTAL_KEY          "TOTAL"         // JSON key for total
#define JSON_VALID_KEY          "VALID"         // JSON key for validity
#define VALUE_START_INDEX       7               // The number of characters to skip to get to the value string

#define IS_CAPITAL_LETTER(ch)   ((ch) >= 'A' && (ch) <= 'Z')    // Macro checking for a capital letter

namespace SerialReaderNamespace
{
    class SerialDataReader
    {
    public:
        SerialDataReader(const string& port, unsigned long baudRate)
            : serial_(io_context, port)
        {
            serial_.set_option(boost::asio::serial_port_base::baud_rate(baudRate));
        }

        void readSerialData()
        {
            string buffer;
            Json::Value jsonData;
            boost::system::error_code error;

            // Create a buffer for reading
            char readBuffer[1024]; // Adjust size as needed
            int grand_total = 0;

            // Calculate the first print time at a 10-second boundary
            time_t nextTime = time(nullptr);
            tm* nextPrintTimeInfo = localtime(&nextTime);
            int nextSeconds = nextPrintTimeInfo->tm_sec;
            nextSeconds = (nextSeconds / 10 + 1) * 10; // Round up to the next 10-second boundary
            nextSeconds %= 60;

            while (true)
            {
                size_t bytesRead = 0;
                buffer.clear();

                // Attempt to read data from the serial port
                try {
                    bytesRead = serial_.read_some(boost::asio::buffer(readBuffer));
                } catch (const boost::system::system_error& e) {
                    // Handle the error appropriately
                    cerr << "Error reading from serial port: " << e.what() << endl;
                    throw boost::system::system_error(error); // Handle any error
                }

                // Debug: Print the raw data read
                //cout << "Raw data read: " + to_string(bytesRead) + " " << string(readBuffer, bytesRead) << endl;

                if (error && error != boost::asio::error::eof)
                {
                    throw boost::system::system_error(error); // Handle any error
                }

                // Append the read data to the buffer
                buffer.append(readBuffer, bytesRead);

                size_t start = buffer.find(START_PACKET_STRING);
                size_t end = buffer.find(END_PACKET_STRING);
                int total = 0;

                // Check that both the Start and End strings are present in the buffer
                if (start != string::npos && end != string::npos)
                {
                    string packet = buffer.substr(start, end - start + strlen(END_PACKET_STRING));
                    parsePacketData(packet, jsonData, total);
                    grand_total += total;
                    buffer.erase(0, end + strlen(END_PACKET_STRING));
                }

                // Get the current time
                time_t currentTime = time(nullptr);
                tm* currentTimeInfo = localtime(&currentTime);
                int currentSeconds = currentTimeInfo->tm_sec;

                // Debug
                //cout << "nextPrintTime: " + to_string(nextSeconds);
                //cout << "currentTime: " + to_string(currentSeconds);

                // On each 10 second time interval
                if ((currentSeconds >= nextSeconds) && ((currentSeconds - nextSeconds) < 30))
                {
                    // Update the TOTAL key in jsonData with the grand_total value
                    jsonData[JSON_TOTAL_KEY] = grand_total;

                    Json::StreamWriterBuilder writer;
                    string jsonOutput = Json::writeString(writer, jsonData);

                    // Output the JSON string to stdout
                    cout << jsonOutput << endl;

                    // Clear jsonData before processing new data
                    jsonData.clear();

                    // reset the grand total
                    grand_total = 0;

                    // Update the next print time to the next 10-second boundary
                    nextSeconds += 10;
                    nextSeconds %= 60;
                }

                // Give other threads a chance to run
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }

    private:
        io_service io_context;
        serial_port serial_;

        bool parsePacketData(const string& data, Json::Value& jsonData, int& total)
        {
            stringstream ss(data);
            string line;
            int calculatedTotal = 0;

            while (getline(ss, line))
            {
                // Check that the Total string is found in the current line
                if (line.find(JSON_TOTAL_KEY) != string::npos)
                {
                    string total_kg = line.substr(VALUE_START_INDEX, line.find(MASS_UNIT_STRING));

                    try
                    {
                        total = stoi(total_kg);
                    }
                    catch (const exception& e)
                    {
                        total = 0;
                    }
                }
                else if (!line.empty() && IS_CAPITAL_LETTER(line[0]))
                {
                    string value_str = line.substr(VALUE_START_INDEX, line.find(MASS_UNIT_STRING));
                    int value;

                    try
                    {
                        value = stoi(value_str);
                    }
                    catch (const exception& e)
                    {
                        value = 0;
                    }
                    jsonData[JSON_CHANNELS_KEY].append(value);
                    calculatedTotal += value;
                }
            }

            jsonData[JSON_TOTAL_KEY] = total;
            jsonData[JSON_VALID_KEY] = (total == calculatedTotal);

            return true;
        }
    };
}

int main(int argc, char** argv)
{
    // Make sure an argument has been provided (for the serial COM port to use)
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <serial_port>\n", argv[0]);
        return 1;
    }

    string port = argv[1];
    unsigned long baud = BAUD_RATE;

    try
    {
        SerialReaderNamespace::SerialDataReader reader(port, baud);
        reader.readSerialData();
    }
    catch (const boost::system::system_error& e)
    {
        fprintf(stderr, "Serial IOException: %s\n", e.what());
        return 1;
    }
    catch (const exception& e)
    {
        fprintf(stderr, "Exception: %s\n", e.what());
        return 1;
    }

    return 0;
}
