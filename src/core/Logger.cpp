#include <pch.h>
#include "Logger.hpp"

namespace VRTR 
{
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> Logger::stdout_sink;
    std::shared_ptr<spdlog::logger> Logger::logger;

    void Logger::init()
    {
        Logger::stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        Logger::stdout_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] %v%$");
        Logger::stdout_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks = {Logger::stdout_sink};

        auto customLogger = std::make_shared<spdlog::logger>(VRTR_LOGGER, sinks.begin(), sinks.end());

        // Log everything at trace level and above
        customLogger->set_level(spdlog::level::trace);
        customLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(customLogger);

        Logger::logger = spdlog::get(VRTR_LOGGER);
    }

    void Logger::destroy()
    {
        spdlog::drop(VRTR_LOGGER);
        stdout_sink.reset();
        spdlog::shutdown();
    }

}