#include <lmgd/network/connection.hpp>

#include <lmgd/network/socket.hpp>

#include <lmgd/except.hpp>
#include <lmgd/log.hpp>

#include <cassert>

namespace lmgd::network
{

Connection::Connection(asio::io_service& io_service, const std::string& hostname)
: io_service_(io_service), hostname_(hostname)
{
    reset();
    start();
}

Connection::~Connection()
{
    if (socket_)
    {
        stop();
    }
}

void Connection::start()
{
    socket_ = std::make_unique<lmgd::network::Socket>(io_service_, hostname_, 5025);

    send_command("*rst");
    send_command("*idn?");

    Log::info() << "Device: " << read_ascii();

    send_command("*zlang short");
}

void Connection::stop()
{
    send_command("*rst");
    send_command("gtl");

    socket_.reset();
    mode_ = Mode::ascii;
}

void Connection::reset()
{
    Log::trace() << "Sending reset to device...";

    lmgd::network::Socket socket(io_service_, hostname_, 5026);

    socket << "break\n";

    std::string line = socket.read_line();

    Log::trace() << "Result: " << line;

    if (line[0] != '0')
    {
        raise("Reset of interface failed: ", line);
    }
}

Connection::Mode Connection::mode() const
{
    return mode_;
}

void Connection::mode(Connection::Mode mode)
{
    send_command("FRMT " + std::to_string(static_cast<int>(mode)));
    mode_ = mode;
}

void Connection::send_command(const std::string& cmd)
{
    Log::debug() << "Sending command: " << cmd;
    *socket_ << cmd + '\n';
}

void Connection::check_command(std::string cmd)
{
    if (!cmd.empty())
    {
        cmd += ";";
    }
    cmd += "ERRALL?";

    send_command(cmd);

    auto result = read_ascii();

    if (result != "0,\"No error\"")
    {
        raise(result);
    }
}

BinaryData Connection::read_binary(size_t reserved_size)
{
    assert(mode_ == Mode::binary);

    char prefix;
    *socket_ >> prefix;

    BinaryData data(reserved_size);
    while (prefix == '#')
    {
        char size_size_buffer;
        *socket_ >> size_size_buffer;

        std::size_t size_size = size_size_buffer - '0';

        auto size_buffer = std::string(size_size, '$');

        socket_->read(reinterpret_cast<std::byte*>(size_buffer.data()), size_size);

        auto size = std::stoull(size_buffer);
        Log::trace() << "Reading binary chunk of size: " << size;

        socket_->read(data.append(size), size);

        *socket_ >> prefix;
    }

    assert(prefix == '\n');
    Log::trace() << "Returning total buffer size: " << data.size();

    return data;
}

std::vector<char> Connection::read_binary_raw()
{
    assert(mode_ == Mode::binary);

    char buffer;

    *socket_ >> buffer;

    std::vector<char> data;

    while (buffer == '#')
    {
        char size_size_buffer;
        *socket_ >> size_size_buffer;

        std::size_t size_size = size_size_buffer - '0';

        auto size_buffer = std::string(size_size, '$');

        socket_->read(reinterpret_cast<std::byte*>(size_buffer.data()), size_size);

        auto size = std::stoull(size_buffer);
        auto old_size = data.size();
        data.resize(old_size + size);

        Log::debug() << "Reading binary chunk of size: " << size;

        socket_->read(reinterpret_cast<std::byte*>(&data[old_size]), size);

        *socket_ >> buffer;
    }

    assert(buffer == '\n');

    Log::debug() << "Returning total buffer size: " << data.size();

    return data;
}

std::string Connection::read_ascii()
{
    assert(mode_ == Mode::ascii);

    return socket_->read_line();
}
} // namespace lmgd::network
