#pragma once

#include <string>
#include <vector>

#include <asio.hpp>

using asio::ip::tcp;

namespace lmgd
{
namespace network
{
    class Socket
    {

    public:
        Socket(const std::string& hostname, int port);

        template <typename T>
        void write(const T& data)
        {
            write(reinterpret_cast<const char*>(&data), sizeof(T));
        }

        template <typename T>
        void read(T& data)
        {
            read(reinterpret_cast<char*>(&data), sizeof(T));
        }

        template <typename T>
        void read(std::vector<T>& data, std::size_t num_elements)
        {
            data.resize(num_elements);

            read(reinterpret_cast<char*>(data.data()), num_elements * sizeof(T));
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

        void read(char* data, std::size_t bytes);

        void write(const char* data, std::size_t bytes);

        // After calling this function calls to read or write are not allowed anymore
        void close();

        Socket(Socket&) = delete;
        void operator=(Socket&) = delete;

    private:
        asio::io_service io_service;
        tcp::socket s;
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
        s.write(reinterpret_cast<const char*>(v.data()), v.size() * sizeof(T));

        return s;
    }

    template <>
    inline Socket& operator<<(Socket& s, const std::string& v)
    {
        s.write(v.c_str(), v.size() + 1);

        return s;
    }
}
}
