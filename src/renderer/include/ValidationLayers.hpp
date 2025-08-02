#pragma once

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif

namespace VRTR
{
    const std::vector<const char*> validationLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };

    class ValidationLayers
    {
        public:
            ValidationLayers() = default;
            ~ValidationLayers() = default;
            static bool checkLayerValidationSupport();
    };
}