#include <lmgd/log.hpp>
#include <lmgd/network/connection.hpp>
#include <lmgd/parser/list.hpp>

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

    try
    {
        lmgd::network::Connection socket(hostname);

        Log::info() << "Connected.";

        socket.send_command("LPFILT?");
        Log::info() << "LPCOFRNG: " << socket.read_ascii();

        socket.send_command("GROUP 6");

        socket.send_command("GLCTRAC 0, \"P1111\"\n");
        socket.send_command("GLCTRAC 1, \"P1211\"\n");
        socket.send_command("GLCTRAC 2, \"P1311\"\n");
        socket.send_command("GLCTRAC 3, \"P1411\"\n");
        socket.send_command("GLCTRAC 4, \"P1511\"\n");
        socket.send_command("GLCTRAC 5, \"P1611\"\n");

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

        socket.send_command("ACTN; TSNORM?; GLPVAL? 0, (0:" + std::to_string(gap_length - 1) +
                            "); GLPVAL? 1, (0:" + std::to_string(gap_length - 1) +
                            "); GLPVAL? 2, (0:" + std::to_string(gap_length - 1) + ")");

        socket.send_command("ERRALL?");

        Log::info() << socket.read_ascii();

        socket.mode(lmgd::network::Connection::Mode::binary);

        socket.send_command("CONT ON");

        for (int i = 0; i < 10; i++)
        {
            auto dataline = socket.read_binary();

            auto timestamp = lmgd::parser::parse<std::int64_t>(&dataline[0]);

            Log::info() << "Timestamp: " << timestamp;

            auto data = lmgd::parser::parse_list<float>(&dataline[8]);

            Log::info() << "Data: " << data.size();
        }

        socket.send_command("CONT OFF");
    }
    catch (std::exception& e)
    {
        lmgd::Log::fatal() << e.what();
    }

    return 0;
}
