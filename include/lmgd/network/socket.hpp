#pragma once

#include <lmgd/log.hpp>

#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/streambuf.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <cstddef>

namespace lmgd
{
namespace network
{
    class Socket
    {

    public:
        Socket() = delete;
        Socket(asio::io_service& io_service);
        Socket(asio::io_service& io_service, const std::string& hostname, int port);

        template <typename T>
        void write(const T& data)
        {
            write(reinterpret_cast<const std::byte*>(&data), sizeof(T));
        }

        template <typename T>
        void read(T& data)
        {
            read(reinterpret_cast<std::byte*>(&data), sizeof(T));
        }

        template <typename T>
        void read(std::vector<T>& data, std::size_t num_elements)
        {
            data.resize(num_elements);

            read(reinterpret_cast<std::byte*>(data.data()), num_elements * sizeof(T));
        }

        void read(std::string& data, std::size_t num_elements)
        {
            std::vector<char> tmp(num_elements);

            read(tmp, num_elements);

            // if string is nullterminated, then cut off last char, as it's handled by std::string
            if (tmp.back() == '\0')
                tmp.resize(num_elements - 1);

            data = std::string(tmp.begin(), tmp.end());
        }

        std::string read_line(char delim = '\n');

        void read(std::byte* data, std::size_t bytes);

        void write(const std::byte* data, std::size_t bytes);

        void open(const std::string& host, int port);

        // After calling this function calls to read or write are not allowed anymore
        void close();

        bool is_open() const;

        Socket(Socket&) = delete;
        void operator=(Socket&) = delete;

        ~Socket();

    private:
        asio::io_service& io_service_;
        asio::ip::tcp::socket socket_;
        asio::streambuf recv_buffer_;
    };

    template <typename T>
    Socket& operator>>(Socket& s, T& t)
    {
        s.read(t);

        return s;
    }

    template <typename T>
    inline Socket& operator<<(Socket& s, const T& t)
    {
        s.write(t);

        return s;
    }

    template <typename T>
    inline Socket& operator<<(Socket& s, const std::vector<T>& v)
    {
        s.write(reinterpret_cast<const std::byte*>(v.data()), v.size() * sizeof(T));

        return s;
    }

    template <>
    inline Socket& operator<<(Socket& s, const std::string& v)
    {
        s.write(reinterpret_cast<const std::byte*>(v.data()), v.size() + 1);

        return s;
    }
} // namespace network
} // namespace lmgd
