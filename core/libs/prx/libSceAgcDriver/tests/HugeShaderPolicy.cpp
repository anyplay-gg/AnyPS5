#include "prx/libSceAgcDriver/Execution/include/HugeShaderPolicy.hpp"
#include <cstdio>

namespace {

int failures = 0;

void Expect(bool condition, const char* what) {
    if (condition) return;
    std::fprintf(stderr, "FAILED: %s\n", what);
    ++failures;
}

}

int main() {
    using namespace AgcDriver;
    Expect(HugeShaderPipelineFlags(2624211, DefaultHugeShaderWords) == VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT, "a 2.6 million word program is compiled without driver optimization");
    Expect(HugeShaderPipelineFlags(DefaultHugeShaderWords, DefaultHugeShaderWords) == VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT, "the limit itself is huge");
    Expect(HugeShaderPipelineFlags(DefaultHugeShaderWords - 1, DefaultHugeShaderWords) == 0, "a program under the limit keeps driver optimization");
    Expect(HugeShaderPipelineFlags(110000, DefaultHugeShaderWords) == 0, "a program of the size of the other captured pipelines keeps driver optimization");
    Expect(HugeShaderPipelineFlags(5000000, 0) == 0, "a limit of zero turns the policy off");
    Expect(ParseHugeShaderWords(nullptr) == DefaultHugeShaderWords, "no value keeps the default");
    Expect(ParseHugeShaderWords("0") == 0, "zero is accepted");
    Expect(ParseHugeShaderWords("12000") == 12000, "a decimal value is accepted");
    Expect(ParseHugeShaderWords("12x") == DefaultHugeShaderWords, "a value that is not a decimal number keeps the default");
    Expect(ParseHugeShaderWords("") == DefaultHugeShaderWords, "an empty value keeps the default");
    if (failures != 0) return 1;
    std::puts("huge shader policy tests passed");
    return 0;
}
