#pragma once
#include <format>
#include <memory>
#include <vector>

#include "Core_export.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define VRTR_LOGGER "VRTR"
#ifndef NDEBUG
    #define VRTR_TRACE(...) VRTR::Logger::get()->trace(__VA_ARGS__)
    #define VRTR_DEBUG(...) VRTR::Logger::get()->debug(__VA_ARGS__)
    #define VRTR_INFO(...) VRTR::Logger::get()->info(__VA_ARGS__)
    #define VRTR_WARN(...) VRTR::Logger::get()->warn(__VA_ARGS__)
    #define VRTR_ERROR(...) VRTR::Logger::get()->error(__VA_ARGS__)
    #define VRTR_CRITICAL(...) VRTR::Logger::get()->critical(__VA_ARGS__)
#else
    #define VRTR_TRACE(...) void(0)
    #define VRTR_DEBUG(...) void(0)
    #define VRTR_INFO(...) void(0)
    #define VRTR_WARN(...) void(0)
    #define VRTR_ERROR(...) void(0)
    #define VRTR_CRITICAL(...) void(0)
#endif

namespace VRTR
{
    class CORE_EXPORT Logger{
    public:
        static void init();
        static void destroy();
        inline static std::shared_ptr<spdlog::logger> get() {return logger;};
    private:
        static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdout_sink;
        static std::shared_ptr<spdlog::logger> logger;
    };
}
