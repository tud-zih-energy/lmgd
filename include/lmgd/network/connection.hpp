#pragma once

#include <lmgd/network/data.hpp>

#include <asio/io_service.hpp>

#include <functional>
#include <memory>

namespace lmgd::network
{

class Socket;

class Connection
{
public:
    using Callback = std::function<void(BinaryData&)>;

    enum class Mode
    {
        ascii = 0,
        binary = 1
    };

    Connection(asio::io_service& io_service, const std::string& hostname);

    ~Connection();

private:
    void start();

    void stop();

    void reset();

public:
    Mode mode() const;
    void mode(Mode mode);

public:
    void send_command(const std::string& cmd);
    void check_command(std::string cmd = "");

public:
    BinaryData read_binary(size_t reserved_size = 0);

    void read_binary_async(Callback callback);

    std::vector<char> read_binary_raw();

    std::string read_ascii();

private:
    asio::io_service& io_service_;
    std::string hostname_;
    std::unique_ptr<lmgd::network::Socket> socket_;
    Mode mode_ = Mode::ascii;
    Callback callback_;
};
} // namespace lmgd::network
