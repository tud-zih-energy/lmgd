#include <lmgd/network/connection.hpp>

#include <nitro/broken_options/parser.hpp>

#include <asio/io_service.hpp>

#include <date/date.h>
#include <date/tz.h>

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
    nitro::broken_options::parser parser("ilmg");

    parser.option("port", "The resource to connect to.").short_name("p");
    parser.toggle("help").short_name("h");
    parser.toggle("serial", "To use serial or network connection").short_name("s");

    try
    {
        auto options = parser.parse(argc, argv);

        if (options.given("help"))
        {
            parser.usage();

            return 0;
        }

        asio::io_service io_serivce;

        auto type = lmgd::network::Connection::Type::socket;

        if (options.given("serial"))
        {
            type = lmgd::network::Connection::Type::serial;
        }

        lmgd::network::Connection socket(io_serivce, type, options.get("port"));

        std::cout << "Connected." << std::endl;

        std::string line;

        while ((std::cout << "lmg $ ") && std::getline(std::cin, line) && line.size() > 1)
        {
            socket.send_command(line);

            if (line.back() == '?')
            {
                std::cout << socket.read_ascii() << std::endl;
            }

            socket.send_command(":SYST:ERR:ALL?");
            std::cerr << "errors before: " << socket.read_ascii() << '\n';
        }
    }
    catch (nitro::broken_options::parsing_error& e)
    {
        std::cerr << e.what() << '\n';

        parser.usage();

        return 1;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
