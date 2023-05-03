#pragma once

#include <iterator>
#include <ostream>
#include <string>
#include <utility>

#include <fmt/format.h>

// TODO: Replace with C++23's std::print(std::ostream&) overloads.

namespace util
{
template <class... Args>
void print(std::ostream& os, fmt::format_string<Args...> fmt, Args&&... args)
{
  fmt::memory_buffer buffer;
  fmt::format_to(std::back_inserter(buffer), std::move(fmt), std::forward<Args>(args)...);
  os.write(buffer.data(), std::ssize(buffer));
}
template <class... Args>
void println(std::ostream& os, fmt::format_string<Args...> fmt, Args&&... args)
{
  util::print(os, std::move(fmt), std::forward<Args>(args)...);
  std::endl(os);
}
}  // namespace util