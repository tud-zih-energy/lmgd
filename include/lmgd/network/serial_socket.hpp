#pragma once

#include <lmgd/network/async_binary_line_reader.hpp>
#include <lmgd/network/async_line_reader.hpp>
#include <lmgd/network/socket.hpp>

#include <asio/serial_port.hpp>

#include <memory>
#include <string>

namespace lmgd
{
namespace network
{
    class SerialSocket : public Socket
    {
    public:
        SerialSocket(asio::io_service& io_service, const std::string& port);
        ~SerialSocket();

        asio::serial_port& asio_socket()
        {
            return socket_;
        }

        void flush();

    public:
        std::string read_line(char delim = '\n') override;
        void read(std::byte* data, std::size_t bytes) override;
        void write(const std::byte* data, std::size_t bytes) override;

        void open(const std::string& host, int port) override;
        void close() override;
        bool is_open() const override;

        void read_binary_async(BinaryCallback callback) override;
        void read_async(Callback callback) override;

    private:
        asio::serial_port socket_;
        asio::serial_port_base::baud_rate baud_;
        std::unique_ptr<AsyncBinaryLineReader<asio::serial_port, BinaryCallback>>
            binary_line_reader_;
        std::unique_ptr<AsyncLineReader<asio::serial_port, Callback>> line_reader_;
    };

} // namespace network
} // namespace lmgd
