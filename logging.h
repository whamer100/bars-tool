#ifndef BARS_TOOL_LOGGING_H
#define BARS_TOOL_LOGGING_H

#include <styler.hpp>
#include <format>

namespace logging {
    template <typename... Args>
    void log(std::string_view fmt_str, Args&&... args)
    {
        std::cout
            << std::vformat(fmt_str, std::make_format_args(args...))
            << std::endl;
    }
    template <typename... Args>
    void info(std::string_view fmt_str, Args&&... args)
    {
        std::cout << "[INFO] "
            << std::vformat(fmt_str, std::make_format_args(args...))
            << std::endl;
    }
    template <typename... Args>
    void warn(std::string_view fmt_str, Args&&... args)
    {
        std::cout << styler::Foreground::Yellow << "[WARN] "
            << std::vformat(fmt_str, std::make_format_args(args...))
            << styler::Style::Reset << std::endl;
    }
    template <typename... Args>
    void fatal(std::string_view fmt_str, Args&&... args)
    {
        std::cout << styler::Foreground::Red << "[FATAL] "
            << std::vformat(fmt_str, std::make_format_args(args...))
            << styler::Style::Reset << std::endl;
    }
    template <typename... Args>
    void funny(std::string_view fmt_str, Args&&... args)
    {
        std::cout << styler::Foreground::Green << "[FUNNY] "
            << std::vformat(fmt_str, std::make_format_args(args...))
            << styler::Style::Reset << std::endl;
    }
}

#endif //BARS_TOOL_LOGGING_H
