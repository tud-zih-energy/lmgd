#include <lmgd/log.hpp>
#include <lmgd/network/connection.hpp>

#include <asio/io_service.hpp>

#include <iostream>
#include <stdexcept>

using lmgd::Log;

int main(int argc, char* argv[])
{
    lmgd::set_severity_debug();

    lmgd::Log::info() << "HELO!";

    std::string hostname = "localhost";
    if (argc > 1)
        hostname = argv[1];

    try
    {
        asio::io_service io_serivce;

        lmgd::network::Connection socket(io_serivce, hostname);

        Log::info() << "Connected.";

        std::string line;

        while ((std::cout << "lmg $ ") && std::getline(std::cin, line) && line.size() > 1)
        {
            socket.send_command(line);

            if (line.back() == '?')
            {
                std::cout << socket.read_ascii() << std::endl;
            }

            socket.send_command("ERRALL?");
            Log::info() << "errors before: " << socket.read_ascii();
        }
    }
    catch (std::exception& e)
    {
        lmgd::Log::fatal() << e.what();
    }

    return 0;
}
