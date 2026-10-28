#ifndef CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_HUGESHADERPOLICY_HPP
#define CORE_LIBS_PRX_LIBSCEAGCDRIVER_EXECUTION_INCLUDE_HUGESHADERPOLICY_HPP

#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdlib>

namespace AgcDriver {

inline constexpr std::size_t DefaultHugeShaderWords = 400000;

inline std::size_t ParseHugeShaderWords(const char* text) {
    if (text == nullptr || text[0] < '0' || text[0] > '9') return DefaultHugeShaderWords;
    char* end = nullptr;
    const auto value = std::strtoull(text, &end, 10);
    return end != nullptr && *end == 0 ? static_cast<std::size_t>(value) : DefaultHugeShaderWords;
}

inline std::size_t HugeShaderWords() {
    static const std::size_t limit = ParseHugeShaderWords(std::getenv("APS5_HUGE_SHADER_WORDS"));
    return limit;
}

inline VkPipelineCreateFlags HugeShaderPipelineFlags(std::size_t words, std::size_t limit) {
    return limit != 0 && words >= limit ? VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT : 0;
}

}

#endif
