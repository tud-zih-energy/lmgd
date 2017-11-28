#include <lmgd/log.hpp>
#include <lmgd/network/connection.hpp>

#include <iostream>
#include <stdexcept>

using lmgd::Log;

int main(int argc, char* argv[])
{
    lmgd::Log::info() << "HELO!";

    std::string hostname = "localhost";

    try
    {
        lmgd::network::Connection socket(hostname);

        Log::info() << "Connected.";

        socket.send_command("GLCTRAC 0, \"P1111\"\n");
        socket.send_command("GLCTRAC 1, \"P1211\"\n");
        socket.send_command("GLCTRAC 2, \"P1311\"\n");

        socket.send_command("GLCSR 500000");

        socket.send_command("CYCLMOD SCOPE");
        socket.send_command("INIM");

        socket.send_command("GLCSR?");
        Log::info() << "Requested Rate: " << socket.read_ascii();

        socket.send_command("GLPSR?");
        Log::info() << "Rate: " << socket.read_ascii();

        socket.send_command("GLPTLEN?");
        auto gap_length = std::stoi(socket.read_ascii());
        Log::info() << "Gap length: " << gap_length;

        socket.send_command("ACTN; TSNORM?; GLPVAL? 0, (0:" + std::to_string(gap_length - 1) +
                            "); GLPVAL? 1, (0:" + std::to_string(gap_length - 1) +
                            "); GLPVAL? 2, (0:" + std::to_string(gap_length - 1) + ")");

        socket.mode(lmgd::network::Connection::Mode::binary);

        socket.send_command("ERRALL?");

        socket.read_binary();

        socket.send_command("CONT ON");

        for (int i = 0; i < 10; i++)
        {
            socket.read_binary();
        }

        socket.send_command("CONT OFF");
    }
    catch (std::exception& e)
    {
        lmgd::Log::fatal() << e.what();
    }

    return 0;
}
