#pragma once

namespace lmgd
{
namespace parser
{
    template <typename T>
    inline std::vector<T> parse_list(char* data)
    {

        size_t size = *reinterpret_cast<std::int64_t*>(data);
        data += sizeof(std::int64_t);

        T* buf = reinterpret_cast<T*>(data);

        return { buf, buf + size };
    }

    template <typename T>
    inline T parse(char* data)
    {
        return *reinterpret_cast<T*>(data);
    }
}
}
