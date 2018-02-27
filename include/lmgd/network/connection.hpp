#pragma once

#include <lmgd/network/data.hpp>

#include <memory>

namespace lmgd::network
{

class Socket;

class Connection
{
public:
    enum class Mode
    {
        ascii = 0,
        binary = 1
    };

    Connection(const std::string& hostname);

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

    std::vector<char> read_binary_raw();

    std::string read_ascii();

private:
    std::string hostname_;
    std::unique_ptr<lmgd::network::Socket> socket_;
    Mode mode_ = Mode::ascii;
};
} // namespace lmgd::network
