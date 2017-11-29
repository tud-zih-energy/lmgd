#pragma once

#include <lmgd/network/socket.hpp>

#include <lmgd/data.hpp>
#include <lmgd/log.hpp>

#include <cassert>
#include <memory>

namespace lmgd
{
namespace network
{
    class Connection
    {
    public:
        enum class Mode
        {
            ascii = 0,
            binary = 1
        };

        Connection(const std::string& hostname) : hostname_(hostname)
        {
            reset();
            start();
        }

        ~Connection()
        {
            if (socket_)
            {
                stop();
            }
        }

    private:
        void start()
        {
            socket_ = std::make_unique<lmgd::network::Socket>(hostname_, 5025);

            send_command("*rst");
            send_command("*idn?");

            Log::info() << "Device: " << read_ascii();

            send_command("*zlang short");
        }

        void stop()
        {
            send_command("*rst");
            send_command("gtl");

            socket_.reset();
            mode_ = Mode::ascii;
        }

        void reset()
        {
            Log::trace() << "Sending reset to device...";

            lmgd::network::Socket socket(hostname_, 5026);

            socket << "break\n";

            std::string line = socket.read_line();

            Log::trace() << "Result: " << line;

            if (line[0] != '0')
            {
                // TODO throw exception
                Log::error() << "Reset of interface failed: " << line;
            }
        }

    public:
        Mode mode() const
        {
            return mode_;
        }

        void mode(Mode mode)
        {
            send_command("FRMT " + std::to_string(static_cast<int>(mode)));
            mode_ = mode;
        }

    public:
        void send_command(const std::string& cmd)
        {
            Log::trace() << "Sending command: " << cmd;
            *socket_ << cmd + '\n';
        }

    public:
        BinaryData read_binary(size_t reserved_size = 0)
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

                socket_->read(size_buffer.data(), size_size);

                auto size = std::stoull(size_buffer);
                Log::trace() << "Reading binary chunk of size: " << size;

                // TODO use byte* everywhere ...
                socket_->read((char*)data.append(size), size);

                *socket_ >> prefix;
            }

            assert(prefix == '\n');
            Log::trace() << "Returning total buffer size: " << data.size();

            return data;
        }

        std::vector<char> read_binary_raw()
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

                socket_->read(size_buffer.data(), size_size);

                auto size = std::stoull(size_buffer);
                auto old_size = data.size();
                data.resize(old_size + size);

                Log::debug() << "Reading binary chunk of size: " << size;

                socket_->read(&data[old_size], size);

                *socket_ >> buffer;
            }

            assert(buffer == '\n');

            Log::debug() << "Returning total buffer size: " << data.size();

            return data;
        }

        std::string read_ascii()
        {
            assert(mode_ == Mode::ascii);

            return socket_->read_line();
        }

    private:
        std::string hostname_;
        std::unique_ptr<lmgd::network::Socket> socket_;
        Mode mode_ = Mode::ascii;
    };
}
}
