#pragma once

#include <lmgd/network/data.hpp>

#include <functional>

namespace lmgd::network
{
enum class CallbackResult
{
    repeat,
    cancel
};

using BinaryCallback = std::function<CallbackResult(std::shared_ptr<BinaryData>&)>;
using Callback = std::function<CallbackResult(std::shared_ptr<std::string>&)>;
} // namespace lmgd::network
