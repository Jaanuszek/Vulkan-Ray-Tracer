#include <pch.h>
#include "Logger.hpp"

namespace VRTR 
{
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> Logger::stdout_sink;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_st> Logger::validationLayers_sink;
    std::shared_ptr<spdlog::logger> Logger::basicLogger;
    std::shared_ptr<spdlog::logger> Logger::validationLogger;

    void Logger::init()
    {
        // Basic logger setup
        Logger::stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        Logger::stdout_sink->set_pattern("%^[%H:%M:%S.%e] [%n] [%l] %v%$");
        Logger::stdout_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks = {Logger::stdout_sink};

        auto customLogger = std::make_shared<spdlog::logger>(VRTR_LOGGER, sinks.begin(), sinks.end());
        // Log everything at trace level and above
        customLogger->set_level(spdlog::level::trace);
        customLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(customLogger);

        Logger::basicLogger = spdlog::get(VRTR_LOGGER);

        // Validation layers logger setup
        const char* validationLayersLoggerName = "ValidationLayers";
        Logger::validationLayers_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        Logger::validationLayers_sink->set_pattern("%^[%H:%M:%S.%e] [%n] [%l] %v%$");
        Logger::validationLayers_sink->set_level(spdlog::level::trace);

        auto validationLayersLogger = std::make_shared<spdlog::logger>(validationLayersLoggerName, Logger::validationLayers_sink);
        validationLayersLogger->set_level(spdlog::level::trace);
        validationLayersLogger->flush_on(spdlog::level::trace);
        spdlog::register_logger(validationLayersLogger);
        
        Logger::validationLogger = spdlog::get(validationLayersLoggerName);
    }

    void Logger::destroy()
    {
        spdlog::drop(VRTR_LOGGER);
        stdout_sink.reset();
        spdlog::shutdown();
    }

}