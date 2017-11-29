#include <lmgd/log.hpp>
#include <lmgd/network/connection.hpp>
#include <lmgd/parser/list.hpp>

#include <nitro/lang/enumerate.hpp>

#include <iostream>
#include <stdexcept>

using lmgd::Log;

namespace std
{
template <typename T>
inline std::ostream& operator<<(std::ostream& s, const std::vector<T>& v)
{
    for (auto t : v)
    {
        s << t << ",";
    }

    return s;
}
}

int main(int argc, char* argv[])
{
    lmgd::set_severity_debug();

    lmgd::Log::info() << "HELO!";

    std::string hostname = "localhost";
    if (argc > 1)
        hostname = argv[1];

    try
    {
        lmgd::network::Connection socket(hostname);

        Log::info() << "Connected.";

        socket.send_command("LPFILT?");
        Log::info() << "LPCOFRNG: " << socket.read_ascii();

        socket.send_command("GROUP 6");

        socket.send_command("GLCTRAC 0, \"P1111\"");
        socket.send_command("GLCTRAC 1, \"P1211\"");
        socket.send_command("GLCTRAC 2, \"P1311\"");
        socket.send_command("GLCTRAC 3, \"P1411\"");
        socket.send_command("GLCTRAC 4, \"P1511\"");
        socket.send_command("GLCTRAC 5, \"P1611\"");

        socket.send_command("GLCSR 500000000");

        socket.send_command("CYCLMOD SCOPE");
        socket.send_command("INIM");

        socket.send_command("GLCSR?");
        Log::info() << "Requested Rate: " << socket.read_ascii();

        socket.send_command("GLPSR?");
        Log::info() << "Rate: " << socket.read_ascii();

        socket.send_command("GLPTLEN?");
        auto gap_length = std::stoi(socket.read_ascii());
        Log::info() << "Gap length: " << gap_length;

        socket.send_command("ACTN; TSSP?; GLPVAL? 0, (0:" + std::to_string(gap_length - 1) +
                            "); GLPVAL? 1, (0:" + std::to_string(gap_length - 1) +
                            "); GLPVAL? 2, (0:" + std::to_string(gap_length - 1) + ")");

        socket.send_command("ERRALL?");

        Log::info() << socket.read_ascii();

        socket.mode(lmgd::network::Connection::Mode::binary);

        socket.send_command("CONT ON");

        for (int i = 0; i < 10; i++)
        {
            auto dataline = socket.read_binary();

            auto timestamp = dataline.read_time();

            Log::info() << "Timestamp: " << timestamp;

            for (int i = 0; i < 3; i++)
            {
                auto list = dataline.read_float_list();
                Log::info() << "Track " << i << ": " << list.size() << " values:";
                for (auto value : nitro::lang::enumerate(list))
                {
                    if (value.index() > 10)
                        break;
                    std::cout << value.value() << ", ";
                }
                std::cout << std::endl;
            }
            Log::info() << "DataBuffer position: " << dataline.position() << " / "
                        << dataline.size();
        }

        socket.send_command("CONT OFF");
    }
    catch (std::exception& e)
    {
        lmgd::Log::fatal() << e.what();
    }

    return 0;
}
