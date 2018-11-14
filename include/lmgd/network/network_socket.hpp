#pragma once

#include <lmgd/network/async_binary_line_reader.hpp>
#include <lmgd/network/async_line_reader.hpp>
#include <lmgd/network/socket.hpp>

#include <asio/ip/tcp.hpp>

#include <memory>
#include <string>

namespace lmgd
{
namespace network
{
    class NetworkSocket : public Socket
    {
    public:
        NetworkSocket(asio::io_service& io_service, const std::string& hostname, int port);
        ~NetworkSocket();

        asio::ip::tcp::socket& asio_socket()
        {
            return socket_;
        }

    public:
        std::string read_line(char delim = '\n') override;
        void read(std::byte* data, std::size_t bytes) override;
        void write(const std::byte* data, std::size_t bytes) override;

        void open(const std::string& port, int) override;
        void close() override;
        bool is_open() const override;

        void read_binary_async(BinaryCallback callback) override;
        void read_async(Callback callback) override;

    private:
        asio::ip::tcp::socket socket_;
        std::unique_ptr<AsyncBinaryLineReader<asio::ip::tcp::socket, BinaryCallback>>
            binary_line_reader_;
        std::unique_ptr<AsyncLineReader<asio::ip::tcp::socket, Callback>> line_reader_;
    };
} // namespace network
} // namespace lmgd
