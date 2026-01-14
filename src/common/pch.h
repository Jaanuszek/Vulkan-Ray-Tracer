#pragma once

// C/C++
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <functional>
#include <chrono>
#include <thread>
#include <optional>
#include <variant>
#include <array>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <atomic>
#include <filesystem>
#include <cassert>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


// VULKAN
#include "GLFW/glfw3.h"
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>
#include <vulkan/vulkan_hpp_macros.hpp>

#define assertm(exp, msg) assert((void(msg), exp))