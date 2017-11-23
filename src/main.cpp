#include <lmgd/log.hpp>
#include <lmgd/network/socket.hpp>

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
    lmgd::Log::info() << "HELO!";

    try
    {
        lmgd::network::Socket socket("example.com", 8000);
    }
    catch (std::exception& e)
    {
        lmgd::Log::fatal() << e.what();
    }

    return 0;
}
