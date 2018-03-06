#pragma once

#include <lmgd/network/data.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <asio/buffer.hpp>
#include <asio/completion_condition.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>

#include <memory>
#include <string>

namespace lmgd::network
{
enum class CallbackResult
{
    repeat,
    cancel
};

template <typename CB>
struct Checker
{
    Checker(CB cb, size_t expected_bytes = 0) : cb(cb), bytes_expected(expected_bytes)
    {
    }
    void operator()(asio::error_code ec, size_t bytes_transferred)
    {
        assert(!ec);
        assert(!bytes_expected || bytes_expected == bytes_transferred);
        cb();
    }

private:
    CB cb;
    size_t bytes_expected;
};

template <typename CB>
class AsyncBinaryLineReader
{
public:
    AsyncBinaryLineReader(asio::ip::tcp::socket& socket, CB completion_callback)
    : socket(socket), completion_callback(completion_callback)
    {
    }

    void read()
    {
        Log::trace() << "AsyncBinaryLineReader::read()";

        data = std::make_shared<BinaryData>();
        read_marker();
    }

private:
    void read_marker()
    {
        Log::trace() << "AsyncBinaryLineReader::read_marker()";

        asio::async_read(socket, asio::buffer(&marker, 1), asio::transfer_exactly(1),
                         Checker([this]() { this->read_chunk(); }, 1));
    }

    void read_chunk()
    {
        Log::trace() << "AsyncBinaryLineReader::read_chunk()";

        switch (marker)
        {
        case '#':
            read_size_size();
            break;
        case '\n':
            if (completion_callback(data) == CallbackResult::repeat)
            {
                read();
            }
            break;
        default:
            // TODO panic
            raise("Invalid marker in binary: ", std::to_string(marker));
        }
    }

    void read_size_size()
    {
        Log::trace() << "AsyncBinaryLineReader::read_size_size()";

        asio::async_read(socket, asio::buffer(&size_size_char, 1), asio::transfer_exactly(1),
                         Checker([this]() { this->read_size(); }, 1));
    }

    void read_size()
    {
        auto size_size = size_size_char - '0';

        Log::trace() << "AsyncBinaryLineReader::read_size(): " << size_size;

        size_str.resize(size_size);
        asio::async_read(socket, asio::buffer(size_str), asio::transfer_exactly(size_size),
                         Checker([this]() { this->read_data(); }, size_size));
    }

    void read_data()
    {
        Log::trace() << "AsyncBinaryLineReader::read_data()";

        auto data_size = std::stoll(size_str);
        auto buf = data->append(data_size);
        asio::async_read(socket, asio::buffer(buf, data_size), asio::transfer_exactly(data_size),
                         Checker([this]() { this->read_marker(); }, data_size));
    }

private:
    std::shared_ptr<BinaryData> data;

    asio::ip::tcp::socket& socket;
    CB completion_callback;

    char marker;
    char size_size_char;
    std::string size_str;
};
} // namespace lmgd::network
