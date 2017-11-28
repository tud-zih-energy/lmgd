#include <lmgd/network/socket.hpp>

#include <lmgd/log.hpp>

#include <asio/buffers_iterator.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>

using asio::ip::tcp;

namespace lmgd
{
namespace network
{

    Socket::Socket(const std::string& hostname, int port) : socket_(io_service_)
    {
        open(hostname, port);
    }

    void Socket::read(char* data, std::size_t bytes)
    {
        Log::trace() << "Reading " << bytes << " bytes from socket...";

        std::size_t reply_length = asio::read(socket_, asio::buffer(data, bytes));

        assert(reply_length == bytes);

        Log::trace() << "Received " << reply_length << " bytes from socket.";
    }

    void Socket::write(const char* data, std::size_t bytes)
    {
        // TODO reduce severity
        Log::trace() << "Writing " << bytes << " bytes onto socket...";

        std::size_t send_bytes = asio::write(socket_, asio::buffer(data, bytes));

        Log::trace() << "Wrote " << send_bytes << " bytes onto socket.";

        // FIXME throw a proper exception here
        assert(send_bytes == bytes);
    }

    std::string Socket::read_line(char delim)
    {
        Log::trace() << "Reading one line from buffer...";

        std::size_t bytes_transferred = asio::read_until(socket_, recv_buffer_, delim);
        std::string line(asio::buffers_begin(recv_buffer_.data()),
                         asio::buffers_begin(recv_buffer_.data()) + bytes_transferred - 1);

        Log::trace() << "Read line of " << bytes_transferred << " bytes length";

        recv_buffer_.consume(bytes_transferred);
        return line;
    }

    void Socket::open(const std::string& hostname, int port)
    {
        tcp::resolver resolver(io_service_);
        tcp::resolver::query query(tcp::v4(), hostname, std::to_string(port));
        tcp::resolver::iterator iterator = resolver.resolve(query);

        socket_.connect(*iterator);
    }

    void Socket::close()
    {
        socket_.close();
        recv_buffer_.consume(recv_buffer_.size());
    }

    bool Socket::is_open() const
    {
        return socket_.is_open();
    }

    Socket::~Socket()
    {
        try
        {
            if (socket_.is_open())
            {
                close();
            }
        }
        catch (std::exception& e)
        {
            Log::error() << "Catched exception while closing socket: " << e.what();
        }
    }
}
}
