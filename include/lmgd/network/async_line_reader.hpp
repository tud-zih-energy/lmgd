#pragma once

#include <lmgd/network/callback.hpp>
#include <lmgd/network/data.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <asio/completion_condition.hpp>
#include <asio/read_until.hpp>
#include <asio/serial_port.hpp>
#include <asio/streambuf.hpp>

#include <memory>
#include <string>

namespace lmgd::network
{

template <typename Socket, typename CB>
class AsyncLineReader
{
public:
    AsyncLineReader(Socket& socket, CB completion_callback)
    : socket(socket), completion_callback(completion_callback)
    {
    }

    void read()
    {
        Log::trace() << "AsyncLineReader::read()";

        read_line();
    }

    ~AsyncLineReader()
    {
        if (buffer_.size())
        {
            Log::warn() << "There is still data in the buffer, but destructing now.";
            buffer_.consume(buffer_.size());
        }
    }

private:
    void read_line()
    {
        Log::trace() << "AsyncLineReader::read_line()";

        asio::async_read_until(
            socket,
            buffer_,
            '\n',
            [this](const asio::error_code& error, std::size_t bytes_transferred) {
                this->read_line_completed(error, bytes_transferred);
            });
    }

    void read_line_completed(const asio::error_code& ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            raise("Error while reading line: ", ec.message());
        }

        Log::trace() << "AsyncLineReader::read_line_completed()";

        data = std::make_shared<std::string>(
            asio::buffers_begin(buffer_.data()),
            asio::buffers_begin(buffer_.data()) + bytes_transferred - 1);
        buffer_.consume(bytes_transferred);

        Log::trace() << "Read line of " << bytes_transferred << " bytes length";

        if (completion_callback(data) == CallbackResult::repeat)
        {
            read();
        }
    }

private:
    std::shared_ptr<std::string> data;

    Socket& socket;
    CB completion_callback;
    asio::streambuf buffer_;
};
} // namespace lmgd::network
