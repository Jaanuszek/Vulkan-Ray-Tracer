#include "Logger.hpp"
#include <iostream>
#include <format>

void logSomething() {
    #ifdef CORE_EXPORT
        std::cout << "Logging from Core library\n";
    #endif
    spdlog::info("Welcome to spdlog!");
    std::cout << std::format("Hello {}!\n", "World");
}