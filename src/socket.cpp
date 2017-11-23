#include <lmgd/network/socket.hpp>

namespace lmgd
{
namespace network
{

    Socket::Socket(const std::string& hostname, int port) : s(io_service)
    {
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(tcp::v4(), hostname, std::to_string(port));
        tcp::resolver::iterator iterator = resolver.resolve(query);

        s.connect(*iterator);
    }

    void Socket::read(char* data, std::size_t bytes)
    {
        std::size_t reply_length = asio::read(s, asio::buffer(data, bytes));

        (void)reply_length;

        assert(reply_length == bytes);
    }

    void Socket::write(const char* data, std::size_t bytes)
    {
        std::size_t send_bytes = asio::write(s, asio::buffer(data, bytes));

        (void)send_bytes;

        assert(send_bytes == bytes);
    }

    void Socket::close()
    {
        s.close();
    }
}
}
