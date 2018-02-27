#pragma once

#include <lmgd/time.hpp>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace lmgd::network
{

using Buffer = std::vector<std::byte>;

template <typename T>
class BinaryList
{
public:
    BinaryList(const std::shared_ptr<Buffer>& buffer, std::byte* begin, size_t size)
    : buffer_(buffer), begin_(reinterpret_cast<T*>(begin)), size_(size)
    {
    }

    T* begin() const
    {
        return begin_;
    }

    T* end() const
    {
        return begin() + size_;
    }

    size_t size() const
    {
        return size_;
    }

private:
    std::shared_ptr<Buffer> buffer_;
    T* begin_;
    size_t size_;
};

class BinaryData
{
public:
    BinaryData(size_t reserved_size = 0) : buffer_(std::make_shared<Buffer>())
    {
        if (reserved_size > 0)
        {
            buffer_->reserve(reserved_size);
        }
    }

    std::byte* append(size_t size)
    {
        // Don't append once someone has read stuff... it's confusing
        assert(position_ == 0);
        auto old_size = buffer_->size();
        buffer_->resize(old_size + size);
        return buffer_->data() + old_size;
    }

    int64_t read_int()
    {
        return *reinterpret_cast<int64_t*>(read(sizeof(int64_t)));
    }

    time::Duration read_time()
    {
        return time::Duration(read_int());
    }

    time::TimePoint read_date()
    {
        return time::TimePoint(time::Duration(read_int()));
    }

    std::string read_string()
    {
        auto length = read_int();
        return std::string(reinterpret_cast<char*>(read(length)));
    }

    BinaryList<int64_t> read_int_list()
    {
        auto length = read_int();
        // Don't copy, just return a view
        return BinaryList<int64_t>(buffer_, read(length * sizeof(int64_t)), length);
    }

    BinaryList<float> read_float_list()
    {
        auto length = read_int();
        // Don't copy, just return a view
        return BinaryList<float>(buffer_, read(length * sizeof(float)), length);
    }

    std::vector<std::string> read_string_list()
    {
        // Can't be bothered to implement efficient stuff
        std::vector<std::string> list;
        size_t length = read_int();
        list.reserve(length);
        while (list.size() < length)
        {
            list.push_back(read_string());
        }
        return list;
    }

    size_t position() const
    {
        return position_;
    }

    size_t size() const
    {
        return buffer_->size();
    }

private:
    std::byte* read(size_t size)
    {
        assert(buffer_);
        assert(size > 0);
        assert(position_ + size <= buffer_->size());
        auto ret = buffer_->data() + position_;
        position_ += size;
        return ret;
    }

    std::shared_ptr<Buffer> buffer_;
    size_t position_ = 0;
};
} // namespace lmgd::network
