#pragma once
#include "pch.h"

namespace VRTR
{
    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    inline uint32_t currentFrame = 0;
    inline uint32_t semaphoreIndex = 0;
}