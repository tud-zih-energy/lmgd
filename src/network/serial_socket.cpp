#include <lmgd/network/serial_socket.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <asio/buffers_iterator.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>

namespace lmgd
{
namespace network
{

    SerialSocket::SerialSocket(asio::io_service& io_service, const std::string& port)
    : Socket(io_service), socket_(io_service_), baud_(57600)
    {
        open(port, -1);
    }

    void SerialSocket::read(std::byte* data, std::size_t bytes)
    {
        Log::trace() << "Reading " << bytes << " bytes from socket...";

        std::size_t reply_length =
            asio::read(socket_, asio::buffer(reinterpret_cast<char*>(data), bytes));

        Log::trace() << "Received " << reply_length << " bytes from socket.";

        if (reply_length != bytes)
        {
            raise("Unexpected number of bytes read from socket");
        }
    }

    void SerialSocket::write(const std::byte* data, std::size_t bytes)
    {
        Log::trace() << "Writing " << bytes << " bytes onto socket...";

        std::size_t send_bytes =
            asio::write(socket_, asio::buffer(reinterpret_cast<const char*>(data), bytes));

        Log::trace() << "Wrote " << send_bytes << " bytes onto socket.";

        if (send_bytes != bytes)
        {
            raise("Unexpected number of bytes written onto socket");
        }
    }

    std::string SerialSocket::read_line(char delim)
    {
        Log::trace() << "Reading one line from buffer...";

        std::size_t bytes_transferred = asio::read_until(socket_, recv_buffer_, delim);
        std::string line(
            asio::buffers_begin(recv_buffer_.data()),
            asio::buffers_begin(recv_buffer_.data()) + bytes_transferred - 1);

        Log::trace() << "Read line of " << bytes_transferred << " bytes length";

        recv_buffer_.consume(bytes_transferred);
        return line;
    }

    void SerialSocket::open(const std::string& port, int)
    {
        socket_.open(port);
        socket_.set_option(baud_);
    }

    void SerialSocket::close()
    {
        socket_.close();
        recv_buffer_.consume(recv_buffer_.size());
    }

    bool SerialSocket::is_open() const
    {
        return socket_.is_open();
    }

    SerialSocket::~SerialSocket()
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

    void SerialSocket::read_binary_async(BinaryCallback callback)
    {
        assert(!binary_line_reader_);
        assert(!line_reader_);
        assert(asio_buffer().size() == 0);

        binary_line_reader_ =
            std::make_unique<AsyncBinaryLineReader<asio::serial_port, BinaryCallback>>(
                asio_socket(), callback);
        binary_line_reader_->read();
    }

    void SerialSocket::read_async(Callback callback)
    {
        assert(!binary_line_reader_);
        assert(!line_reader_);
        assert(asio_buffer().size() == 0);

        line_reader_ =
            std::make_unique<AsyncLineReader<asio::serial_port, Callback>>(asio_socket(), callback);
        line_reader_->read();
    }
} // namespace network
} // namespace lmgd
