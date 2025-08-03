#pragma once
#include "Core_export.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define VRTR_LOGGER "VRTR"
// TODO add a compiler flag that enable/disable logs
#ifdef NDEBUG
    #define VRTR_TRACE(...) VRTR::Logger::getBasicLogger()->trace(__VA_ARGS__)
    #define VRTR_DEBUG(...) void(0)
    #define VRTR_INFO(...) VRTR::Logger::getBasicLogger()->info(__VA_ARGS__)
    #define VRTR_WARN(...) VRTR::Logger::getBasicLogger()->warn(__VA_ARGS__)
    #define VRTR_ERROR(...) VRTR::Logger::getBasicLogger()->error(__VA_ARGS__)
    #define VRTR_CRITICAL(...) VRTR::Logger::getBasicLogger()->critical(__VA_ARGS__)

    #define VRTR_VALIDATION_TRACE(...) VRTR::Logger::getValidationLogger()->trace(__VA_ARGS__)
    #define VRTR_VALIDATION_DEBUG(...) void(0)
    #define VRTR_VALIDATION_INFO(...) VRTR::Logger::getValidationLogger()->info(__VA_ARGS__)
    #define VRTR_VALIDATION_WARN(...) VRTR::Logger::getValidationLogger()->warn(__VA_ARGS__)
    #define VRTR_VALIDATION_ERROR(...) VRTR::Logger::getValidationLogger()->error(__VA_ARGS__)
    #define VRTR_VALIDATION_CRITICAL(...) VRTR::Logger::getValidationLogger()->critical(__VA_ARGS__)
#else
    #define VRTR_TRACE(...) VRTR::Logger::getBasicLogger()->trace(__VA_ARGS__)
    #define VRTR_DEBUG(...) VRTR::Logger::getBasicLogger()->debug(__VA_ARGS__)
    #define VRTR_INFO(...) VRTR::Logger::getBasicLogger()->info(__VA_ARGS__)
    #define VRTR_WARN(...) VRTR::Logger::getBasicLogger()->warn(__VA_ARGS__)
    #define VRTR_ERROR(...) VRTR::Logger::getBasicLogger()->error(__VA_ARGS__)
    #define VRTR_CRITICAL(...) VRTR::Logger::getBasicLogger()->critical(__VA_ARGS__)

    #define VRTR_VALIDATION_TRACE(...) VRTR::Logger::getValidationLogger()->trace(__VA_ARGS__)
    #define VRTR_VALIDATION_DEBUG(...) VRTR::Logger::getValidationLogger()->debug(__VA_ARGS__)
    #define VRTR_VALIDATION_INFO(...) VRTR::Logger::getValidationLogger()->info(__VA_ARGS__)
    #define VRTR_VALIDATION_WARN(...) VRTR::Logger::getValidationLogger()->warn(__VA_ARGS__)
    #define VRTR_VALIDATION_ERROR(...) VRTR::Logger::getValidationLogger()->error(__VA_ARGS__)
    #define VRTR_VALIDATION_CRITICAL(...) VRTR::Logger::getValidationLogger()->critical(__VA_ARGS__)
#endif

namespace VRTR
{
    class CORE_EXPORT Logger{
    public:
        static void init();
        static void destroy();
        inline static std::shared_ptr<spdlog::logger> getBasicLogger() {return basicLogger;};
        inline static std::shared_ptr<spdlog::logger> getValidationLogger() {return validationLogger;};
    private:
        static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdout_sink;
        static std::shared_ptr<spdlog::sinks::stdout_color_sink_st> validationLayers_sink;
        static std::shared_ptr<spdlog::logger> basicLogger;
        static std::shared_ptr<spdlog::logger> validationLogger;
    };
}
