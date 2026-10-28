#include "prx/libc/include/Api.hpp"
#include "BdaTests.hpp"
#include "GraphicsTests.hpp"
#include "prx/libSceAgcDriver/Execution/include/GuestMemory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Recorder.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Resources.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Pipeline.hpp"
#include "prx/libSceAgcDriver/Graphics/include/PipelineJournal.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureDetiler.hpp"
#include "prx/libSceAgcDriver/Graphics/include/VertexInput.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "prx/libSceAgcDriver/Graphics/include/BufferPool.hpp"
#include "prx/libSceAgcDriver/Graphics/include/GuestBufferMemory.hpp"
#include "ShaderDiskCache.hpp"
#include "ShaderCacheDirectory.hpp"
#include "prx/libSceAgcDriver/Graphics/include/MultisampleTarget.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ShaderInputState.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DccMetadata.hpp"
#include "prx/libSceAgcDriver/Graphics/include/ParallelCompare.hpp"
#include "prx/libSceAgcDriver/Graphics/include/DepthSurface.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Texture.hpp"
#include "prx/libSceAgcDriver/Graphics/include/TextureTiling.hpp"
#include "SceShaders.hpp"
#include "prx/libSceAgc/Shader/include/ShaderConstants.hpp"
#include "ControlFlow/RequestSerializer.hpp"
#include "CacheKey.hpp"
#include "Optimization/ResourceProgram.hpp"
#include "BdaAbi.hpp"
#include "prx/libc/include/GuestAllocations.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/GuestHeap.hpp"
#include "ImageTableAbi.hpp"
#include "SpirvBackend/SpirvSpecialization.hpp"
#include <spirv/unified1/spirv.hpp>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <random>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <thread>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;

alignas(256) std::array<std::byte, 1024> colorMemory{};
alignas(256) std::array<std::byte, 2048> sliceMemory{};
alignas(256) std::array<std::uint8_t, 4> volumeKeyMemory{};

AgcDriver::QueueState makeState() {
    AgcDriver::QueueState queue;
    queue.userConfig[0x242] = 4;
    queue.context = {
        {0x2d5, 0x2000},
        {0x1b6, 0}, {0x1b8, 0}, {0x207, 0}, {0x200, 0}, {0x203, 0x800},
        {0x2dc, 0xaa00}, {0x2f8, 0}, {0x292, 2}, {0x293, 0},
        {0x80, 0}, {0x8d, 0}, {0x83, 0xffff}, {0x8c, 0xa},
        {0x2f9, 0x2d}, {0x313, 0x6000}, {0x30e, 0xffffffff}, {0x30f, 0xffffffff},
        {0x206, 0x43f}, {0x204, 0x80000}, {0x205, 0x240},
        {0x8e, 0xf}, {0x8f, 0xf}, {0x202, 0xcc0010},
        {0x1c4, 0}, {0x1c5, 9}, {0x1c3, 4}, {0x31c, 0x28028},
        {0x31b, 0}, {0x31d, 0}, {0x3b0, (63u << 14u) | 3u},
        {0x3b8, 0x9000000}, {0x1e0, 0},
        {0xc, 0}, {0xd, 0x40040},
        {0x81, 0x80000000}, {0x82, 0x40040},
        {0x90, 0x80000000}, {0x91, 0x40040},
        {0x94, 0x80000000}, {0x95, 0x40040}
    };
    const auto address = reinterpret_cast<std::uintptr_t>(colorMemory.data());
    queue.context[0x318] = static_cast<std::uint32_t>(address >> 8u);
    queue.context[0x390] = static_cast<std::uint32_t>(address >> 40u);
    queue.context[0x10f] = std::bit_cast<std::uint32_t>(32.0f);
    queue.context[0x110] = std::bit_cast<std::uint32_t>(32.0f);
    queue.context[0x111] = std::bit_cast<std::uint32_t>(-2.0f);
    queue.context[0x112] = std::bit_cast<std::uint32_t>(2.0f);
    queue.context[0x113] = std::bit_cast<std::uint32_t>(1.0f);
    queue.context[0x114] = 0;
    queue.context[0xb4] = 0;
    queue.context[0xb5] = std::bit_cast<std::uint32_t>(1.0f);
    queue.shader[0x008] = 0x100;
    queue.shader[0x009] = 0;
    return queue;
}

template<typename TAction>
void expectFailure(TAction action, std::string_view reason) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        Require(std::string_view(error.what()).find(reason) != std::string_view::npos, std::string("unexpected failure: ") + error.what());
        return;
    }
    throw std::runtime_error("expected graphics rejection: " + std::string(reason));
}

void inactiveVertexPointSizeTests() {
    const auto pointSizeState = [] {
        auto queue = makeState();
        queue.context[0x1b3] = queue.context[0x1b4] = 2;
        queue.context[0x207] = 0x01210000;
        return queue;
    };
    for (const auto primitive : {2u, 4u, 5u, 6u, 7u, 17u, 19u, 21u}) {
        auto queue = pointSizeState();
        queue.userConfig[0x242] = primitive;
        for (const auto raster : {0u, 0x240u, 0x248u}) {
            queue.context[0x205] = raster;
            const auto rejection = AgcDriver::Graphics::DrawRejection(queue, false);
            Require(rejection.empty(), "inactive vertex point size failed the draw precheck: " + rejection);
            static_cast<void>(AgcDriver::Graphics::DecodeState(queue));
        }
    }
    for (const auto raster : {0u, 0x240u, 0x248u}) {
        auto queue = pointSizeState();
        queue.userConfig[0x242] = 3;
        queue.context[0x205] = raster;
        const auto rejection = AgcDriver::Graphics::DrawRejection(queue, false);
        Require(rejection.empty(), "inactive line-strip point size failed the draw precheck: " + rejection);
        const auto state = AgcDriver::Graphics::DecodeState(queue);
        Require(state.topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP && !state.primitiveRestart, "inactive point size changed line-strip decoding");
        Require(AgcDriver::Graphics::RasterPrimitiveOf(state) == ShaderRecompiler::RasterPrimitive::Line, "inactive point size changed the line primitive class");
        queue.userConfig[0x24b] = 1;
        queue.context[0x103] = 0xffffffffu;
        const auto indexedRejection = AgcDriver::Graphics::DrawRejection(queue, true);
        Require(indexedRejection.empty(), "inactive line-strip point size with primitive restart failed the draw precheck: " + indexedRejection);
        const auto indexedState = AgcDriver::Graphics::DecodeState(queue);
        Require(indexedState.topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP && indexedState.primitiveRestart, "inactive point size changed indexed line-strip decoding");
    }
    const auto rejected = [](AgcDriver::QueueState queue) {
        Require(AgcDriver::Graphics::DrawRejection(queue, false).find("auxiliary vertex exports") != std::string::npos, "unsupported vertex exports passed the draw precheck");
        try {
            static_cast<void>(AgcDriver::Graphics::DecodeState(queue));
        } catch (const std::runtime_error&) {
            return;
        }
        throw std::runtime_error("potentially active point size passed the state decoder");
    };
    {
        auto queue = pointSizeState();
        queue.userConfig[0x242] = 9;
        rejected(queue);
    }
    {
        auto queue = pointSizeState();
        queue.userConfig[0x242] = 1;
        queue.context[0x2d5] = 0x2020;
        rejected(queue);
    }
    for (const auto stages : {0x2004u, 0x2020u}) {
        auto queue = pointSizeState();
        queue.context[0x2d5] = stages;
        rejected(queue);
    }
    for (const auto raster : {8u, 0x208u, 0x48u}) {
        auto queue = pointSizeState();
        queue.context[0x205] = raster;
        rejected(queue);
    }
    for (const auto flag : {1u << 17u, 1u << 20u}) {
        auto queue = pointSizeState();
        queue.context[0x207] = 0x01210000 | flag;
        rejected(queue);
    }
    for (const auto distances : {1u, 0x100u, 0x00c000ffu, 0x00c0ff00u}) {
        auto queue = pointSizeState();
        queue.context[0x207] |= distances;
        Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "inactive point size rejected supported clip/cull distances");
        Require(AgcDriver::Graphics::DecodeState(queue).paClVsOutCntl == queue.context[0x207], "inactive point size discarded clip/cull distance state");
    }
    {
        auto queue = pointSizeState();
        queue.context[0x207] |= 0x004001ffu;
        Require(AgcDriver::Graphics::DrawRejection(queue, false).find("more than eight") != std::string::npos, "inactive point size bypassed the clip/cull count limit");
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "more than eight clip and cull distances");
    }
    for (const auto offset : {0x2d5u, 0x205u}) {
        auto queue = pointSizeState();
        queue.context.erase(offset);
        rejected(queue);
    }
    auto queue = pointSizeState();
    queue.userConfig.erase(0x242);
    rejected(queue);
}

void PolygonPrimitiveTests() {
    auto queue = makeState();
    queue.userConfig[0x242] = 21;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, "a polygon did not decode to a triangle fan");
    Require(!state.primitiveRestart, "polygon decoding enabled primitive restart by default");
    Require(AgcDriver::Graphics::RasterPrimitiveOf(state) == ShaderRecompiler::RasterPrimitive::Triangle, "a polygon was not classified as a triangle primitive");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "the precheck rejected a direct polygon draw");
    queue.userConfig[0x24b] = 1;
    queue.context[0x103] = 0xffffffffu;
    Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "the precheck rejected an indexed polygon draw with restart");
    const auto indexedState = AgcDriver::Graphics::DecodeState(queue);
    Require(indexedState.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN && indexedState.primitiveRestart, "indexed polygon decoding did not keep primitive restart");
}

void vertexPointListTests() {
    const auto pointState = [](std::uint32_t control, std::uint32_t size, std::uint32_t limits) {
        auto queue = makeState();
        queue.userConfig[0x242] = 1;
        queue.context[0x1b3] = queue.context[0x1b4] = 2;
        queue.context[0x207] = control;
        queue.context[0x280] = size;
        queue.context[0x281] = limits;
        return queue;
    };
    {
        const auto queue = pointState(0x01210000u, 0x00080008u, 0x00080008u);
        const auto rejection = AgcDriver::Graphics::DrawRejection(queue, false);
        Require(rejection.empty(), "a vertex-path point list with a vertex point size failed the draw precheck: " + rejection);
        const auto state = AgcDriver::Graphics::DecodeState(queue);
        Require(state.topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST && state.paClVsOutCntl == 0x01210000u, "a vertex-path point list with a vertex point size did not decode as a point list");
        Require(AgcDriver::Graphics::RasterPrimitiveOf(state) == ShaderRecompiler::RasterPrimitive::Point, "a vertex-path point list is not rasterized as points");
        Require(state.vertexPoints && state.pointSize == 1.0f && state.pointSizeMin == 1.0f && state.pointSizeMax == 1.0f, "PA_SU_POINT_SIZE and PA_SU_POINT_MINMAX of 8 did not decode as one-pixel points");
        ShaderRecompiler::ShaderVertexStageInfo vertex{};
        vertex.paClVsOutCntl = 1u << 21u;
        AgcDriver::Graphics::ApplyPointSize(state, vertex);
        Require(vertex.pointList && vertex.paClVsOutCntl == ((1u << 16u) | (1u << 21u)) && vertex.pointSize == 1.0f && vertex.pointSizeMin == 1.0f && vertex.pointSizeMax == 1.0f, "the vertex stage did not receive the vertex point size");
    }
    {
        const auto state = AgcDriver::Graphics::DecodeState(pointState(0, 0x00200020u, 0x00100004u));
        Require(state.vertexPoints && state.pointSize == 4.0f && state.pointSizeMin == 0.5f && state.pointSizeMax == 2.0f, "a register point size did not decode as diameters");
        ShaderRecompiler::ShaderVertexStageInfo vertex{};
        vertex.paClVsOutCntl = 1u << 21u;
        AgcDriver::Graphics::ApplyPointSize(state, vertex);
        Require(vertex.pointList && vertex.paClVsOutCntl == (1u << 21u) && vertex.pointSize == 4.0f, "a register point size turned on the vertex point size");
    }
    {
        const auto state = AgcDriver::Graphics::DecodeState(pointState(0x01210000u, 0x00200010u, 0x00400000u));
        Require(state.pointSizeMin == 0.0f && state.pointSizeMax == 8.0f, "a vertex point size refused a register size whose width and height differ");
    }
    {
        auto queue = makeState();
        queue.context[0x1b3] = queue.context[0x1b4] = 2;
        queue.context[0x207] = 0x01210000u;
        const auto state = AgcDriver::Graphics::DecodeState(queue);
        Require(!state.vertexPoints, "a triangle draw decoded as vertex points");
        ShaderRecompiler::ShaderVertexStageInfo vertex{};
        AgcDriver::Graphics::ApplyPointSize(state, vertex);
        Require(!vertex.pointList && vertex.paClVsOutCntl == 0, "a triangle draw received a point size");
    }
    expectFailure([&] { AgcDriver::Graphics::DecodeState(pointState(0, 0x00200010u, 0x00080008u)); }, "points whose PA_SU_POINT_SIZE width and height differ are unsupported");
    expectFailure([&] { AgcDriver::Graphics::DecodeState(pointState(0, 0x00080008u, 0x00080010u)); }, "PA_SU_POINT_MINMAX with a minimum above its maximum is unsupported");
    expectFailure([&] { AgcDriver::Graphics::DecodeState(pointState(1u << 16u, 0x00080008u, 0x00080008u)); }, "USE_VTX_POINT_SIZE without VS_OUT_MISC_VEC_ENA is unsupported");
    expectFailure([&] { AgcDriver::Graphics::DecodeState(pointState(0, 0, 0x00080008u)); }, "points of size 0 (PA_SU_POINT_SIZE 0) are unsupported");
    expectFailure([&] { AgcDriver::Graphics::DecodeState(pointState(0x01210000u, 0x00080008u, 0)); }, "a vertex point size clamped to 0 (PA_SU_POINT_MINMAX maximum 0) is unsupported");
    {
        const auto state = AgcDriver::Graphics::DecodeState(pointState(0x01210000u, 0, 0x00200000u));
        Require(state.pointSize == 0.0f && state.pointSizeMin == 0.0f && state.pointSizeMax == 4.0f, "a vertex point size refused a zero register size or a zero clamp minimum");
        static_cast<void>(AgcDriver::Graphics::DecodeState(pointState(0, 0x00080008u, 0)));
    }
    for (const auto sprite : {1u << 17u, 1u << 23u}) {
        auto queue = pointState(0x01210000u, 0x00080008u, 0x00080008u);
        queue.context[0x1b5] = 0x486bu;
        queue.context[0x1b6] = 2;
        queue.context[0x191] = 0;
        queue.context[0x192] = sprite;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "point sprite texture coordinates (SPI_PS_INPUT_CNTL PT_SPRITE_TEX) are not implemented");
        queue.context[0x1b5] = 0x4869u;
        Require(AgcDriver::Graphics::DecodeState(queue).vertexPoints, "PT_SPRITE_TEX was refused with point sprites off");
    }
}

void retainedPixelStageTests() {
    using namespace AgcDriver::Graphics;
    auto queue = makeState();
    queue.context[0x8e] = queue.context[0x8f] = queue.context[0x203] = 0;
    for (const auto format : {0u, 1u, 2u, 3u, 9u}) {
        queue.context[0x1c4] = format;
        Require(PixelProgramSkipped(queue), "a retained export format forced an unused pixel program to run");
        Require(DrawRejection(queue, true).empty(), "an unused pixel program with a retained export format was rejected");
        for (const auto control : {1u, 2u, 4u, 0x40u, 0x80u, 0x100u, 0x200u, 0x400u, 0x20000u}) {
            queue.context[0x203] = control;
            Require(!PixelProgramSkipped(queue), "an enabled export or execution flag was skipped");
        }
        queue.context[0x203] = 0;
        queue.context[0x8e] = 0xf0000000u;
        queue.context[0x8f] = 0x0f000000u;
        Require(PixelProgramSkipped(queue), "disjoint color masks ran an unused pixel program");
        queue.context[0x8f] = 0xf0000000u;
        Require(!PixelProgramSkipped(queue), "an enabled high render target lost its pixel program");
        queue.context[0x8e] = queue.context[0x8f] = 0;
    }
    queue.context[0x1b3] = queue.context[0x1b4] = 2;
    for (const auto [format, control] : {std::pair{1u, 1u}, std::pair{2u, 2u}, std::pair{9u, 0x100u}, std::pair{9u, 0x103u}}) {
        queue.context[0x1c4] = format;
        queue.context[0x203] = control;
        const auto state = DecodeState(queue);
        Require(DrawRejection(queue, true).empty(), "a valid pixel export combination was rejected");
        const auto pixel = DecodePixelStageInfo(queue.context, ExportMappings(state), PixelProgramSkipped(queue));
        Require(pixel.depthExportEnable == ((control & 1u) != 0) && pixel.stencilRefExportEnable == ((control & 2u) != 0) && pixel.sampleMaskExportEnable == ((control & 0x100u) != 0), "decoding lost an enabled pixel export");
    }
    queue.context[0x203] = 0;
    for (const auto alpha : {0xaa00u, 0xaa01u}) {
        queue.context[0x2dc] = alpha;
        Require(PixelProgramSkipped(queue) == ((alpha & 1u) == 0), "alpha-to-coverage enable was not respected");
    }
    queue.context[0x2dc] = 0xaa00u;
    for (const auto offset : {0x8eu, 0x8fu, 0x203u}) {
        auto missing = queue;
        missing.context.erase(offset);
        Require(!PixelProgramSkipped(missing), "incomplete execution state skipped a bound pixel program");
    }
}

void hdrComponentSwapTests() {
    using namespace AgcDriver::Graphics;
    for (const auto number : {0u, 1u, 4u, 7u}) {
        auto queue = makeState();
        queue.context[0x31c] = (queue.context[0x31c] & ~0x1f7cu) | (12u << 2u) | (number << 8u) | (1u << 11u);
        const auto address = reinterpret_cast<std::uintptr_t>(sliceMemory.data());
        queue.context[0x318] = static_cast<std::uint32_t>(address >> 8u);
        queue.context[0x390] = static_cast<std::uint32_t>(address >> 40u);
        queue.context[0x1b3] = queue.context[0x1b4] = 2;
        auto state = DecodeState(queue);
        const auto expected = number == 0 ? VK_FORMAT_R16G16B16A16_UNORM : number == 1 ? VK_FORMAT_R16G16B16A16_SNORM : number == 4 ? VK_FORMAT_R16G16B16A16_UINT : VK_FORMAT_R16G16B16A16_SFLOAT;
        Require(state.color.format == expected && state.color.elementBytes == 8, "alternate 16-bit color format was not decoded");
        Require(state.color.componentMapping == 0xc6u && ExportMappings(state)[0] == 0xc6u, "alternate 16-bit color export did not swap red and blue");
        Require(DecodePixelStageInfo(queue.context, ExportMappings(state)).targetExportMapping[0] == 0xc6u, "pixel export lost the alternate color mapping");
        queue.context[0x8e] = 1;
        state = DecodeState(queue);
        Require(state.blend.colorWriteMask == VK_COLOR_COMPONENT_B_BIT, "alternate red-only write did not select the third storage component");
        queue.context[0x8e] = 0xf;
        queue.context[0x1e0] = (1u << 30u) | 13u;
        for (std::uint32_t i = 0; i < 4; ++i) queue.context[0x105 + i] = std::bit_cast<std::uint32_t>(float(i + 1) / 8.0f);
        state = DecodeState(queue);
        Require(state.blendConstants == std::array<float, 4>{0.375f, 0.25f, 0.125f, 0.5f}, "alternate color blend constants were not swizzled with the export");
        queue.context[0x31c] &= ~(1u << 11u);
        state = DecodeState(queue);
        Require(state.color.componentMapping == 0xe4u && state.blendConstants == std::array<float, 4>{0.125f, 0.25f, 0.375f, 0.5f}, "standard color mapping changed");
        queue.context[0x31c] |= 2u << 11u;
        expectFailure([&] { DecodeState(queue); }, "reversed component order");
    }
    auto queue = makeState();
    queue.context[0x31c] = (queue.context[0x31c] & ~0x1f7cu) | (12u << 2u) | (7u << 8u) | (1u << 11u);
    for (const auto offset : {0x318u, 0x31bu, 0x31cu, 0x31du}) queue.context[offset + 0xfu] = queue.context.at(offset);
    for (const auto offset : {0x390u, 0x3b0u, 0x3b8u}) queue.context[offset + 1u] = queue.context.at(offset);
    queue.context[0x31c + 0xfu] &= ~(1u << 11u);
    queue.context[0x1c5] = 0x99u;
    queue.context[0x8e] = queue.context[0x8f] = 0xffu;
    queue.context[0x1e0] = queue.context[0x1e1] = (1u << 30u) | 13u;
    for (std::uint32_t i = 0; i < 4; ++i) queue.context[0x105 + i] = std::bit_cast<std::uint32_t>(float(i + 1) / 8.0f);
    expectFailure([&] { DecodeState(queue); }, "conflicting blend constants");
    queue.context[0x107] = queue.context[0x105];
    Require(DecodeState(queue).colors.size() == 2, "compatible constants rejected mixed component orders");
}

void stateTests() {
    AgcDriver::QueueState initial;
    Require(initial.context.at(0x200) == 0 && initial.context.at(0x83) == 0xffff, "initial context state is missing");
    initial.context[0x200] = 7;
    initial.context[0xdead] = 1;
    initial.ClearContext();
    Require(initial.context.at(0x200) == 0 && !initial.context.contains(0xdead), "context reset did not restore defaults");
    for (std::uint32_t slot = 0; slot < 8; ++slot) {
        const auto info = 0x31cu + 0xfu * slot;
        Require(initial.context.at(info) == 0, "reset color target was not disabled");
        initial.context[info] = 10u << 2u;
    }
    initial.ClearContext();
    initial.context[0x8e] = initial.context[0x8f] = 0xffffffffu;
    Require(AgcDriver::Graphics::ColorWriteMask(initial.context) == 0, "context clear kept old color targets enabled");
    Require(initial.userConfig.at(0x24b) == 0, "primitive restart must be disabled in initial queue state");
    initial.userConfig[0x24b] = 1;
    initial.ClearContext();
    Require(initial.userConfig.at(0x24b) == 1, "context clear must preserve user configuration");
    initial = AgcDriver::QueueState{};
    Require(initial.userConfig.at(0x24b) == 0, "queue reset must disable primitive restart");
    auto queue = makeState();
    auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.color.address == reinterpret_cast<std::uintptr_t>(colorMemory.data()) && state.color.bytes == colorMemory.size(), "render-target address or size changed");
    Require(state.viewport.y == 4 && state.viewport.height == -4, "negative viewport height was lost");
    Require(state.color.format == VK_FORMAT_R8G8B8A8_UNORM, "RGBA format changed");
    queue.userConfig[0x24b] = 1;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    (void)AgcDriver::Graphics::DecodeState(queue);
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "primitive restart rejected a non-indexed draw");
    queue.userConfig[0x242] = 9;
    Require(AgcDriver::Graphics::DrawRejection(queue, true).find("point, line and triangle") != std::string::npos, "primitive restart was accepted for patches");
    queue.userConfig[0x242] = 6;
    queue.context[0x103] = 0xffffffffu;
    Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "primitive restart was rejected for an indexed strip");
    Require(AgcDriver::Graphics::DecodeState(queue).primitiveRestart, "primitive restart was not decoded for a strip");
    queue.userConfig[0x242] = 21;
    Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "primitive restart was rejected for an indexed polygon");
    Require(AgcDriver::Graphics::DecodeState(queue).primitiveRestart, "primitive restart was not decoded for a polygon");
    queue.context[0x103] = 5;
    Require(AgcDriver::Graphics::DrawRejection(queue, true).find("all ones") != std::string::npos, "a restart index other than all ones was accepted");
    queue = makeState();
    queue.context[0x293] = 0x06020000u;
    (void)AgcDriver::Graphics::DecodeState(queue);
    Require(AgcDriver::Graphics::DrawRejection(queue, false).find("out-of-order") == std::string::npos, "per-engine primitive discard was rejected");
    queue.context[0x293] = 0x760201bcu;
    (void)AgcDriver::Graphics::DecodeState(queue);
    Require(AgcDriver::Graphics::DrawRejection(queue, false).find("out-of-order") == std::string::npos, "an out-of-order watermark without out-of-order rasterization was rejected");
    queue.context[0x293] = 0x7e0201bcu;
    Require(AgcDriver::Graphics::DrawRejection(queue, false).find("out-of-order") != std::string::npos, "out-of-order rasterization was accepted");
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "out-of-order");
    queue.context[0x293] = 0x06030000u;
    Require(AgcDriver::Graphics::DrawRejection(queue, false).find("out-of-order") == std::string::npos, "per-sample iteration of a single-sample target was rejected");
    Require(!AgcDriver::Graphics::DecodeState(queue).perSampleShading, "per-sample iteration of a single-sample target requested sample shading");
    queue.context[0x293] = 0;
    for (const auto disabled : {0x6000u, 0x00100000u, 0u, 0x000e6000u}) {
        queue.context[0x313] = disabled;
        Require(AgcDriver::Graphics::DecodeState(queue).conservativeRasterization == VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT, "disabled conservative rasterization decoded as enabled");
        Require(AgcDriver::Graphics::DrawRejection(queue, false).find("PA_SC_CONSERVATIVE") == std::string::npos, "disabled conservative rasterization was rejected");
    }
    for (const auto enabled : {0x00e00001u, 0x01e00022u, 0x000e6001u, 0x000e6020u}) {
        queue.context[0x313] = enabled;
        Require(AgcDriver::Graphics::DrawRejection(queue, false).find("PA_SC_CONSERVATIVE") != std::string::npos, "conservative rasterization was accepted");
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "PA_SC_CONSERVATIVE");
    }
    queue.context[0x313] = 0x6000u;
    queue = makeState();
    queue.userConfig.erase(0x24b);
    queue.context[0x2a5] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "user-config bank at DWORD 0x24b");
    queue = makeState();
    queue.context[0x90] = 0x80010003;
    queue.context[0x91] = 0x30020;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.scissor.offset.x == 3 && state.scissor.offset.y == 1 && state.scissor.extent.width == 29 && state.scissor.extent.height == 2, "scissor intersection changed");
    queue.context[0x90] = 0x10003;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.scissor.offset.x == 3 && state.scissor.offset.y == 1 && state.scissor.extent.width == 29 && state.scissor.extent.height == 2, "a scissor that applies the zero window offset changed");
    queue.context[0x90] = 0x80040003;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.scissor.extent.height == 0, "a scissor whose top is below its bottom passes pixels");
    queue.context[0x90] = 0x80010021;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.scissor.extent.width == 0, "a scissor whose left edge is right of its right edge passes pixels");
    queue.context[0x90] = 0x80008000;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "scissor reserved bits");
    queue.context[0x90] = 0x80010003;
    queue.context[0x31c] |= 0x10000000;
    queue.context[0x325] = 0x1234;
    Require(AgcDriver::Graphics::DecodeState(queue).color.dccAddress == 0x123400, "DCC key address decode changed");
    queue = makeState();
    queue.context.erase(0x3b8);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "missing register");
    queue = makeState();
    queue.context[0x3b8] |= 1u << 14u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported color tile mode");
    queue = makeState();
    queue.context[0x3b0] = (62u << 14u) | 3u;
    Require(AgcDriver::Graphics::DecodeState(queue).color.bytes == 64u * 4u * 4u, "padded linear pitch changed");
    queue = makeState();
    queue.context[0x8e] = 0xff;
    queue.context[0x8f] = 0xff;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "color export format 0");
    queue = makeState();
    queue.context[0x200] = 2;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "depth");
    queue = makeState();
    queue.context[0x200] = 0x007007b4;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    (void)AgcDriver::Graphics::DecodeState(queue);
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "a depth write without the depth test was rejected");
    queue = makeState();
    queue.context[0x200] = 0x007007b6;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "depth");
    queue = makeState();
    queue.context[0x010] = 0x80000180;
    queue.context[0x011] = 0x20000180;
    queue.context[0x200] = 0x007007b3;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.depthTest && !state.stencilTest, "tests on absent depth and stencil planes were kept");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "tests on absent depth and stencil planes were rejected");
    queue.context[0x200] = 0x007007bbu;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.depth.has_value() && !state.depthTest && !state.depthBoundsTest && !state.stencilTest, "a depth bounds test on absent depth and stencil planes was kept");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "a depth bounds test on absent depth and stencil planes was rejected");
    queue.context[0x200] = 8u;
    Require(!AgcDriver::Graphics::DecodeState(queue).depthBoundsTest && AgcDriver::Graphics::DrawRejection(queue, false).empty(), "a depth bounds test alone on absent depth and stencil planes was kept or rejected");
    queue.context[0x200] = 0x007007b3;
    queue.context[0x011] = 0x20000181;
    queue.context[0x012] = 0x00001000;
    queue.context[0x013] = 0x00002000;
    queue.context[0x015] = 0x00002000;
    queue.context[0x007] = 0x003f003f;
    queue.context[0x000] = 0;
    queue.context[0x002] = 0;
    queue.context[0x00a] = 0;
    queue.context[0x00b] = 0x3f800000;
    queue.context[0x10b] = 0;
    queue.context[0x10c] = 0x01ffff00;
    queue.context[0x10d] = 0x01ffff00;
    state =AgcDriver::Graphics::DecodeState(queue);
    Require(!state.depthTest && state.stencilTest, "a depth test on an absent depth plane was kept beside a stencil plane");
    queue.context[0x200] = 0x007007bbu;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "depth bounds without a depth plane");
    queue = makeState();
    queue.context[0x010] = 0x80000183;
    queue.context[0x011] = 0x20000180;
    queue.context[0x012] = 0x00001000;
    queue.context[0x014] = 0x00001000;
    queue.context[0x007] = 0x0001001f;
    queue.context[0x000] = 0;
    queue.context[0x002] = 0;
    queue.context[0x00a] = 0;
    queue.context[0x00b] = 0x3f800000;
    queue.context[0x200] = 0x007007b1;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.depth.has_value() && !state.stencilTest && state.renderExtent.width == state.color.extent.width && state.renderExtent.height == state.color.extent.height, "a stencil test without a stencil plane bound the depth plane it does not test");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "a stencil test without a stencil plane beside an untested depth plane was rejected");
    queue.context[0x200] = 0x00700231;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.depth.has_value() && !state.stencilTest && state.renderExtent.width == state.color.extent.width && state.renderExtent.height == state.color.extent.height, "a failing stencil test without a stencil plane bound the depth plane it does not test");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "a failing stencil test without a stencil plane beside an untested depth plane was rejected");
    queue.context[0x200] = 0x007007b3;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.depth.has_value() && state.depthTest && !state.stencilTest && state.renderExtent.width == 32 && state.renderExtent.height == 2, "a depth test on a bound depth plane lost its surface");
    queue = makeState();
    queue.context[0x10f] = 0x7fc00000;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "non-finite");
    // The register facade: every register the decoders read is in DrawKeyRegisters (a wrong hit of
    // the draw key otherwise), and nothing is recorded without a log pointer.
    queue = makeState();
    std::vector<AgcDriver::Graphics::RegisterRead> log;
    {
        const AgcDriver::Graphics::RegisterReadLogScope logScope(&log);
        queue.context[0x1b3] = 2;
        queue.context[0x1b4] = 2;
        state = AgcDriver::Graphics::DecodeState(queue);
        Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "precheck rejected the reference state");
        static_cast<void>(AgcDriver::Graphics::DecodePixelStageInfo(queue.context, AgcDriver::Graphics::ExportMappings(state)));
    }
    Require(!log.empty(), "the register facade recorded nothing");
    const auto recorded = log.size();
    static_cast<void>(AgcDriver::Graphics::DecodeState(queue));
    Require(log.size() == recorded, "the register facade recorded without an open log");
    for (const auto read : log) Require(AgcDriver::Graphics::DrawKeyCovers(read), "DrawKeyRegisters lacks a register the decoders read: " + std::string(AgcDriver::Graphics::RegisterBankName(read.bank)) + " " + std::to_string(read.offset));
    Require(!AgcDriver::Graphics::DrawKeyCovers({AgcDriver::Graphics::RegisterBank::Context, 0x100}) && AgcDriver::Graphics::DrawKeyCovers({AgcDriver::Graphics::RegisterBank::Shader, 0xab}) && !AgcDriver::Graphics::DrawKeyCovers({AgcDriver::Graphics::RegisterBank::Shader, 0xac}), "DrawKeyRegisters coverage changed");
    log.clear();
    static_cast<void>(AgcDriver::Graphics::DecodeState(queue));
    Require(log.empty(), "the register facade recorded without a log");
    queue = makeState();
    queue.context.erase(0x1b3);
    queue.context.erase(0x1b4);
    queue.context.erase(0x1c5);
    queue.shader[0x008] = 0x100;
    queue.shader[0x009] = 0;
    Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "a pixel program address was read as unset");
    Require(AgcDriver::Graphics::DrawRejection(queue, true).find("missing register at DWORD 0x1b3") != std::string::npos, "a real pixel program without SPI_PS_INPUT_ENA was accepted");
    expectFailure([&] { AgcDriver::Graphics::DecodePixelStageInfo(queue.context, std::array<std::uint8_t, 8>{0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u}); }, "missing register");
    AgcDriver::QueueState cleared;
    cleared.context[0x1b3] = 2;
    cleared.context[0x1b4] = 2;
    cleared.context[0x1b6] = 2;
    cleared.context[0x192] = 7;
    const auto clearedPixel = AgcDriver::Graphics::DecodePixelStageInfo(cleared.context, std::array<std::uint8_t, 8>{0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u});
    Require(clearedPixel.interpolatorSettings[0] == 0 && clearedPixel.interpolatorSettings[1] == 7, "an unwritten SPI_PS_INPUT_CNTL_0 did not read as its clear-state value");
    cleared.ClearContext();
    for (std::uint32_t i = 0; i < 32; ++i) Require(cleared.context.at(0x191 + i) == 0, "CLEAR_STATE did not reset SPI_PS_INPUT_CNTL");
    queue.shader[0x008] = 0;
    Require(AgcDriver::Graphics::PixelProgramSkipped(queue), "a zero pixel program address was not read as unset");
    Require(AgcDriver::Graphics::DrawRejection(queue, true).find("writes color") != std::string::npos, "a draw without a pixel program that writes color was accepted");
    auto colorDisabled = queue;
    colorDisabled.context[0x202] = colorDisabled.context.at(0x202) & ~0x70u;
    Require(AgcDriver::Graphics::DrawRejection(colorDisabled, true).empty(), "a draw without a pixel program was rejected while CB_COLOR_CONTROL disables color");
    Require(!AgcDriver::Graphics::DecodeState(colorDisabled).hasColorTarget, "a draw with CB_COLOR_CONTROL disabling color decoded a color target");
    colorDisabled.context.erase(0x202);
    Require(AgcDriver::Graphics::DrawRejection(colorDisabled, true).find("writes color") != std::string::npos, "a draw without a pixel program and an unwritten CB_COLOR_CONTROL was accepted");
    queue.shader.erase(0x008);
    queue.shader.erase(0x009);
    Require(AgcDriver::Graphics::PixelProgramSkipped(queue), "an unwritten pixel program address was not read as unset");
    Require(AgcDriver::Graphics::DrawRejection(queue, true).find("writes color") != std::string::npos, "a draw with an unwritten pixel program that writes color was accepted");
    queue.context[0x8e] = 0;
    Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "a depth-only draw with an unwritten pixel program was rejected");
    queue.shader[0x008] = 0;
    queue.shader[0x009] = 0;
    Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "a depth-only draw without a pixel program was rejected");
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.hasColorTarget, "a depth-only draw without a pixel program decoded a color target");
    const auto unset = AgcDriver::Graphics::DecodePixelStageInfo(queue.context, AgcDriver::Graphics::ExportMappings(state), true);
    Require(unset.inputAddr == ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::PerspectiveCenter), "unset pixel inputs of the null program did not read as PERSP_CENTER_ENA");
    for (const auto mode : unset.targetOutputMode) Require(mode == 0, "an unset SPI_SHADER_COL_FORMAT exported a color");
    queue.shader[0x008] = 0x100;
    queue.context[0x1c4] = 0;
    queue.context[0x203] = 0;
    Require(AgcDriver::Graphics::PixelProgramSkipped(queue), "a pixel program that writes nothing and cannot run was not skipped");
    Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "a depth-only draw whose pixel program cannot run was rejected");
    for (const auto [control, what] : {std::pair{0x400u, "EXEC_ON_NOOP"}, std::pair{0x40u, "KILL_ENABLE"}}) {
        queue.context[0x203] = control;
        Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), std::string("a pixel program with ") + what + " was skipped");
        Require(AgcDriver::Graphics::DrawRejection(queue, true).find("missing register at DWORD 0x1b3") != std::string::npos, std::string("a pixel program with ") + what + " ran without SPI_PS_INPUT_ENA");
    }
    queue.context[0x203] = 1;
    Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "a pixel program with Z_EXPORT_ENABLE was skipped");
    queue.context[0x203] = 0;
    queue.context[0x1c4] = 1;
    Require(AgcDriver::Graphics::PixelProgramSkipped(queue), "a retained Z export format ran a pixel program with depth exports disabled");
    Require(AgcDriver::Graphics::DrawRejection(queue, true).empty(), "a depth-only draw with a retained Z export format was rejected");
    for (const auto control : {1u, 2u, 4u, 0x40u, 0x80u, 0x100u, 0x200u, 0x400u, 0x20000u}) {
        queue.context[0x203] = control;
        Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "an enabled pixel export or execution flag was ignored with a retained Z format");
    }
    queue.context[0x203] = 0;
    queue.context[0x2dc] = 0xaa01;
    Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "alpha-to-coverage skipped the pixel program");
    queue.context[0x203] = 0x800;
    Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "the Vulkan alpha-to-coverage state lost its pixel program with DB's override set");
    queue.context[0x203] = queue.context[0x2dc] = 0;
    queue.context[0x1c4] = 0;
    queue.context[0x8e] = 0xf;
    Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "a pixel program writing color was skipped");
    queue.context[0x1b3] = queue.context[0x1b4] = queue.context[0x1b6] = 0xffffffffu;
    queue.context[0x1c5] = queue.context[0x203] = 0xffffffffu;
    const auto disabled = AgcDriver::Graphics::DecodePixelStageInfo(queue.context, AgcDriver::Graphics::ExportMappings(state), true);
    Require(disabled.interpolatorCount == 0 && disabled.inputAddr == ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::PerspectiveCenter), "the null pixel program inherited stale input state");
    Require(!disabled.pixelKillEnable && !disabled.depthExportEnable && !disabled.sampleMaskExportEnable, "the null pixel program inherited stale exports");
    for (const auto mode : disabled.targetOutputMode) Require(mode == 0, "the null pixel program inherited stale color exports");
    expectFailure([&] { AgcDriver::Graphics::DecodePixelStageInfo(queue.context, AgcDriver::Graphics::ExportMappings(state), false); }, "input count exceeds 32");
}

VkFormatFeatureFlags srgb8Features = 0;

void srgb8FormatProperties(VkPhysicalDevice, VkFormat format, VkFormatProperties* properties) {
    *properties = {};
    if (format == VK_FORMAT_R8_SRGB) properties->optimalTilingFeatures = srgb8Features;
}

void setProxyVariable(const char* value) {
#ifdef _WIN32
    _putenv_s("APS5_SRGB_ATTACHMENT_PROXY", value);
#else
    if (*value == 0) unsetenv("APS5_SRGB_ATTACHMENT_PROXY");
    else setenv("APS5_SRGB_ATTACHMENT_PROXY", value, 1);
#endif
}

void srgb8TargetTests() {
    setProxyVariable("");
    auto queue = makeState();
    queue.context[0x31c] = 0x8604;
    const auto color = AgcDriver::Graphics::DecodeState(queue).color;
    Require(color.format == VK_FORMAT_R8_SRGB && color.elementBytes == 1 && color.componentMapping == 0xe4u, "the 8_SRGB color target did not decode as R8_SRGB");
    for (std::uint32_t swap = 1; swap < 4; ++swap) {
        queue.context[0x31c] = 0x8604 | (swap << 11u);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported color format 1 number type 6 component swap " + std::to_string(swap));
    }
    queue.context[0x31c] = 0x860c;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported color format 3 number type 6 component swap 0");
    AgcDriver::Graphics::Context context{};
    context.formatProperties = srgb8FormatProperties;
    const auto proxyOn = [&](std::uintptr_t device, VkFormatFeatureFlags features, VkFormat format = VK_FORMAT_R8_SRGB) {
        srgb8Features = features;
        context.physical = reinterpret_cast<VkPhysicalDevice>(device);
        return AgcDriver::Graphics::AttachmentProxyFormat(context, format);
    };
    constexpr VkFormatFeatureFlags attachment = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    constexpr VkFormatFeatureFlags blend = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
    Require(proxyOn(0x1000, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == VK_FORMAT_R8G8B8A8_SRGB, "a device without R8_SRGB attachments must render 8_SRGB through RGBA8_SRGB");
    Require(proxyOn(0x2000, attachment) == VK_FORMAT_R8G8B8A8_SRGB, "a device without R8_SRGB blending must render 8_SRGB through RGBA8_SRGB");
    Require(proxyOn(0x3000, attachment | blend) == VK_FORMAT_UNDEFINED, "a device with R8_SRGB attachments must render 8_SRGB directly");
    Require(proxyOn(0x3000, 0) == VK_FORMAT_UNDEFINED, "the attachment proxy decision is not kept per device");
    Require(proxyOn(0x4000, 0, VK_FORMAT_R8_UNORM) == VK_FORMAT_UNDEFINED && proxyOn(0x4000, 0, VK_FORMAT_R8G8_SRGB) == VK_FORMAT_UNDEFINED, "only 8_SRGB targets may render through a proxy");
    setProxyVariable("1");
    Require(proxyOn(0x5000, attachment | blend) == VK_FORMAT_R8G8B8A8_SRGB, "APS5_SRGB_ATTACHMENT_PROXY=1 did not force the proxy");
    setProxyVariable("");
}

void hardwareScreenOffsetTests() {
    auto queue = makeState();
    queue.context[0x90] = 0x80010003;
    queue.context[0x91] = 0x30020;
    const auto reference = AgcDriver::Graphics::DecodeState(queue);
    for (const auto offset : {0u, 1u, 0x10000u, 0x0020003cu, 0x01ff0000u, 0x000001ffu, 0x01ff01ffu}) {
        queue.context[0x8d] = offset;
        const auto state = AgcDriver::Graphics::DecodeState(queue);
        Require(state.viewport.x == reference.viewport.x && state.viewport.y == reference.viewport.y && state.viewport.width == reference.viewport.width && state.viewport.height == reference.viewport.height && state.viewport.minDepth == reference.viewport.minDepth && state.viewport.maxDepth == reference.viewport.maxDepth, "hardware guard-band offset changed the viewport");
        Require(state.scissor.offset.x == reference.scissor.offset.x && state.scissor.offset.y == reference.scissor.offset.y && state.scissor.extent.width == reference.scissor.extent.width && state.scissor.extent.height == reference.scissor.extent.height, "hardware guard-band offset changed the scissor");
        Require(state.renderExtent.width == reference.renderExtent.width && state.renderExtent.height == reference.renderExtent.height, "hardware guard-band offset changed the framebuffer extent");
    }
    for (std::uint32_t bit = 0; bit < 32; ++bit) {
        if (bit < 9 || (bit >= 16 && bit < 25)) continue;
        queue.context[0x8d] = 1u << bit;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "reserved PA_SU_HARDWARE_SCREEN_OFFSET bits");
    }
    queue.context.erase(0x8d);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "missing register");
}

void depthCopyFormatTests() {
    using AgcDriver::Graphics::Texture;
    Require(Texture::DepthCopyFormat(VK_FORMAT_R32_SFLOAT) == VK_FORMAT_D32_SFLOAT, "a 32-bit float color format did not map to the 32-bit float depth format");
    Require(Texture::DepthCopyFormat(VK_FORMAT_R16_UNORM) == VK_FORMAT_D16_UNORM, "a 16-bit unorm color format did not map to the 16-bit unorm depth format");
    for (const auto format : {VK_FORMAT_UNDEFINED, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16_UINT, VK_FORMAT_R32_UINT, VK_FORMAT_D32_SFLOAT}) {
        Require(Texture::DepthCopyFormat(format) == VK_FORMAT_UNDEFINED, "a color format without a depth counterpart mapped to a depth format");
    }
}

void UnwrittenSampleLocationTests() {
    using AgcDriver::Graphics::DecodeState;
    using AgcDriver::Graphics::DrawRejection;
    using AgcDriver::Graphics::SampleLocations;
    const auto fresh = [](std::uint32_t config, std::uint32_t attributes, std::uint32_t scanMode = 3) {
        auto written = makeState();
        written.context[0x2f8] = config;
        written.context[0x292] = scanMode;
        written.context[0x3b8] = 0x0dc00000u;
        written.context[0x31d] = attributes;
        written.context[0x1b3] = 2;
        written.context[0x1b4] = 2;
        AgcDriver::QueueState queue;
        queue.shader = written.shader;
        queue.userConfig = written.userConfig;
        written.context.ForEach([&](std::uint32_t offset, std::uint32_t value) { queue.context[offset] = value; });
        return queue;
    };
    AgcDriver::QueueState initial;
    for (std::uint32_t offset = 0x2fe; offset <= 0x30d; ++offset) Require(initial.context.contains(offset) && initial.context.at(offset) == 0, "an unwritten PA_SC_AA_SAMPLE_LOCS register did not hold its clear-state value 0");
    for (const auto samples : {2u, 4u, 8u}) {
        const auto log2 = static_cast<std::uint32_t>(std::countr_zero(samples));
        auto queue = fresh((log2 << 20u) | log2, (log2 << 12u) | (log2 << 15u));
        const auto state = DecodeState(queue);
        Require(state.samples == samples && state.colors.size() == 1 && state.colors[0].samples == samples, "a multisampled draw with unwritten sample locations did not decode its sample count");
        Require(state.customSampleLocations, "unwritten sample locations did not decode as the clear-state pattern");
        for (std::uint32_t sample = 0; sample < samples; ++sample) Require(state.sampleLocations[sample] == 0, "an unwritten sample location was not at the pixel center");
        const auto centered = SampleLocations(state);
        for (std::uint32_t sample = 0; sample < samples; ++sample) Require(centered[sample].x == 0.5f && centered[sample].y == 0.5f, "unwritten sample locations did not place every sample at the pixel center");
        const auto rejection = DrawRejection(queue, false);
        Require(rejection.empty(), "the precheck refused a multisampled draw with unwritten sample locations: " + rejection);
    }
    auto queue = fresh(0x00200002, 0x12000);
    for (std::uint32_t pixel = 0; pixel < 4; ++pixel) queue.context[0x2fe + pixel * 4u] = 0x622ae6ae;
    auto state = DecodeState(queue);
    Require(state.samples == 4 && !state.customSampleLocations, "explicit standard 4x sample locations changed");
    queue = fresh(0x00200002, 0x12000);
    for (std::uint32_t pixel = 0; pixel < 4; ++pixel) queue.context[0x2fe + pixel * 4u] = 0x22eea66a;
    state = DecodeState(queue);
    Require(state.samples == 4 && state.customSampleLocations, "explicit custom 4x sample locations changed");
    const auto custom = SampleLocations(state);
    const std::array<VkSampleLocationEXT, 4> expected{{{0.125f, 0.875f}, {0.875f, 0.125f}, {0.375f, 0.375f}, {0.625f, 0.625f}}};
    for (std::size_t sample = 0; sample < expected.size(); ++sample) Require(custom[sample].x == expected[sample].x && custom[sample].y == expected[sample].y, "explicit custom 4x sample coordinates changed");
    queue.ClearContext();
    for (std::uint32_t offset = 0x2fe; offset <= 0x30d; ++offset) Require(queue.context.at(offset) == 0, "CLEAR_STATE did not reset PA_SC_AA_SAMPLE_LOCS");
    expectFailure([&] { DecodeState(fresh(0x00400004, 0x12000)); }, "more than eight samples");
    expectFailure([&] { DecodeState(fresh(0x00100002, 0x12000)); }, "exposing fewer samples");
    queue = fresh(0, 0, 3);
    state = DecodeState(queue);
    Require(state.samples == 1 && DrawRejection(queue, false).empty(), "MSAA_ENABLE on a single-sample target with unwritten sample locations was refused");
}

void msaaEnableClearOneSampleTargetTests() {
    using AgcDriver::Graphics::DecodeState;
    using AgcDriver::Graphics::DrawRejection;
    const auto drawC = [](std::uint32_t config = 0x0020c002u) {
        auto queue = makeState();
        queue.context[0x2f8] = config;
        queue.context[0x292] = 2;
        for (std::uint32_t offset = 0x2fe; offset <= 0x30d; ++offset) queue.context[offset] = (offset - 0x2feu) % 4u == 0 ? 0x622ae6aeu : 0u;
        queue.context[0x31c] = 0x8028;
        queue.context[0x31d] = 0;
        queue.context[0x010] = 0x80000183;
        queue.context[0x200] = 0x710;
        queue.context[0x201] = 0x00010002;
        queue.context[0x1b3] = 2;
        queue.context[0x1b4] = 2;
        return queue;
    };
    const auto requireOneSample = [](const AgcDriver::QueueState& queue, const std::string& what) {
        const auto state = DecodeState(queue);
        Require(state.samples == 1 && state.colors.size() == 1 && state.colors[0].samples == 1 && !state.depth, what + " was not drawn as one sample");
        Require(!state.customSampleLocations && !state.sampleLocationGrid && !state.perSampleShading && state.sampleLocations == decltype(state.sampleLocations){}, what + " kept the multisampled rasterizer's sample state");
        const auto rejection = DrawRejection(queue, false);
        Require(rejection.empty(), "the precheck refused " + what + ": " + rejection);
    };
    auto queue = drawC();
    requireOneSample(queue, "PPSA18216's draw C (a one-sample color target under a 4x PA_SC_AA_CONFIG without MSAA_ENABLE)");
    auto reference = drawC(0);
    const auto state = DecodeState(queue);
    const auto expected = DecodeState(reference);
    Require(state.samples == expected.samples && state.customSampleLocations == expected.customSampleLocations && state.sampleLocations == expected.sampleLocations && state.perSampleShading == expected.perSampleShading && state.colors[0].format == expected.colors[0].format, "draw C decoded differently from the same draw with a one-sample PA_SC_AA_CONFIG");
    for (const auto config : {0x00100001u, 0x00306003u}) requireOneSample(drawC(config), "a one-sample color target under a 2x or 8x PA_SC_AA_CONFIG without MSAA_ENABLE");
    queue = drawC();
    queue.context[0x302] = 0xae62e62a;
    requireOneSample(queue, "a one-sample color target under a 4x pattern that varies between pixels");
    for (const auto input : {ShaderRecompiler::PixelInput::PerspectiveSample, ShaderRecompiler::PixelInput::PerspectiveCentroid, ShaderRecompiler::PixelInput::LinearSample, ShaderRecompiler::PixelInput::LinearCentroid}) {
        queue = drawC();
        queue.context[0x1b3] = queue.context[0x1b4] = 2u | ShaderRecompiler::PixelInputBit(input);
        requireOneSample(queue, "draw C with sample or centroid inputs");
    }
    queue = drawC();
    queue.context[0x1b3] |= ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::Ancillary);
    requireOneSample(queue, "draw C with an enabled but unaddressed ANCILLARY input");
    constexpr std::string_view refused = "a one-sample color target under a multisampled rasterizer without MSAA_ENABLE is unsupported with a sample index or coverage input, or a sample-mask export";
    for (const auto input : {ShaderRecompiler::PixelInput::Ancillary, ShaderRecompiler::PixelInput::SampleCoverage}) {
        queue = drawC();
        queue.context[0x1b3] = queue.context[0x1b4] = 2u | ShaderRecompiler::PixelInputBit(input);
        expectFailure([&] { DecodeState(queue); }, refused);
    }
    queue = drawC();
    queue.context[0x293] = 0x10000u;
    queue.context[0x1b3] = queue.context[0x1b4] = 2u | ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::PerspectiveSample);
    requireOneSample(queue, "draw C with PS_ITER_SAMPLE and a sample-interpolated input");
    Require(!DecodeState(queue).perSampleShading, "PS_ITER_SAMPLE into a one-sample color target without MSAA_ENABLE asked for sample shading");
    queue.context[0x1b3] = queue.context[0x1b4] = 2u | ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::Ancillary);
    expectFailure([&] { DecodeState(queue); }, refused);
    queue = drawC();
    queue.context[0x1c4] = 9;
    queue.context[0x203] |= 0x100u;
    expectFailure([&] { DecodeState(queue); }, refused);
    queue = drawC();
    queue.context[0x2dc] = 0x0000aa01u;
    expectFailure([&] { DecodeState(queue); }, "without MSAA_ENABLE are unsupported for points or alpha-to-coverage");
    for (const auto primitive : {2u, 3u}) {
        queue = drawC();
        queue.userConfig[0x242] = primitive;
        requireOneSample(queue, "a line list or strip into PPSA18216's draw C target");
    }
    const auto twoTargets = [&](std::uint32_t first, std::uint32_t second) {
        auto pair = drawC();
        for (const auto offset : {0x318u, 0x31bu, 0x31cu}) pair.context[offset + 0xfu] = pair.context.at(offset);
        for (const auto offset : {0x390u, 0x3b0u, 0x3b8u}) pair.context[offset + 1u] = pair.context.at(offset);
        pair.context[0x31d] = first;
        pair.context[0x32c] = second;
        pair.context[0x1c5] = 0x99u;
        pair.context[0x8e] = pair.context[0x8f] = 0xffu;
        pair.context[0x1e1] = 0;
        return pair;
    };
    queue = twoTargets(0, 0);
    Require(DecodeState(queue).samples == 1 && DecodeState(queue).colors.size() == 2, "two one-sample color targets under a 4x PA_SC_AA_CONFIG without MSAA_ENABLE were not drawn as one sample");
    for (const auto [first, second] : {std::pair{0u, 0x12000u}, std::pair{0x12000u, 0u}}) {
        queue = twoTargets(first, second);
        expectFailure([&] { DecodeState(queue); }, "a color target's sample count differs from the rasterizer's");
    }
    const auto depthTarget = [&](bool color, std::uint32_t config = 0x0020c002u) {
        auto depth = drawC(config);
        depth.context[0x0] = 0;
        depth.context[0x002] = 0;
        depth.context[0x007] = (3u << 16u) | 63u;
        depth.context[0x00a] = 0;
        depth.context[0x00b] = 0;
        depth.context[0x011] = 0x20000180;
        for (const auto offset : {0x012u, 0x014u}) depth.context[offset] = 0x100;
        depth.context[0x200] = 0x16;
        if (!color) depth.context[0x8e] = 0;
        return depth;
    };
    const auto requireOneSampleDepth = [](const AgcDriver::QueueState& queue, std::size_t colors, const std::string& what) {
        const auto state = DecodeState(queue);
        Require(state.samples == 1 && state.depth && state.depth->samples == 1 && state.colors.size() == colors && std::ranges::all_of(state.colors, [](const auto& color) { return color.samples == 1; }), what + " was not drawn as one sample");
        Require(!state.customSampleLocations && !state.sampleLocationGrid && !state.perSampleShading && state.sampleLocations == decltype(state.sampleLocations){}, what + " kept the multisampled rasterizer's sample state");
        const auto rejection = DrawRejection(queue, false);
        Require(rejection.empty(), "the precheck refused " + what + ": " + rejection);
    };
    queue = depthTarget(false);
    requireOneSampleDepth(queue, 0, "PPSA18216's opening-movie draw (a one-sample depth target and no color target under a 4x PA_SC_AA_CONFIG without MSAA_ENABLE)");
    const auto movie = DecodeState(queue);
    const auto movieReference = DecodeState(depthTarget(false, 0));
    Require(!movie.hasColorTarget && movie.renderExtent.width == movieReference.renderExtent.width && movie.renderExtent.height == movieReference.renderExtent.height && movie.depth->format == movieReference.depth->format && movie.sampleLocations == movieReference.sampleLocations && movie.customSampleLocations == movieReference.customSampleLocations, "the opening-movie draw decoded differently from the same draw with a one-sample PA_SC_AA_CONFIG");
    requireOneSampleDepth(depthTarget(true), 1, "a one-sample depth and color pair under a 4x PA_SC_AA_CONFIG without MSAA_ENABLE");
    queue = depthTarget(false);
    queue.context[0x302] = 0xae62e62a;
    requireOneSampleDepth(queue, 0, "a one-sample depth target under a 4x pattern that varies between pixels");
    for (const auto config : {0x00100001u, 0x00306003u}) {
        requireOneSampleDepth(depthTarget(false, config), 0, "a one-sample depth target under a 2x or 8x PA_SC_AA_CONFIG without MSAA_ENABLE");
        requireOneSampleDepth(depthTarget(true, config), 1, "a one-sample depth and color pair under a 2x or 8x PA_SC_AA_CONFIG without MSAA_ENABLE");
    }
    for (const auto input : {ShaderRecompiler::PixelInput::PerspectiveSample, ShaderRecompiler::PixelInput::PerspectiveCentroid, ShaderRecompiler::PixelInput::LinearSample, ShaderRecompiler::PixelInput::LinearCentroid}) {
        queue = depthTarget(false);
        queue.context[0x203] |= 0x40u;
        queue.context[0x1b3] = queue.context[0x1b4] = 2u | ShaderRecompiler::PixelInputBit(input);
        requireOneSampleDepth(queue, 0, "a one-sample depth target under a 4x PA_SC_AA_CONFIG without MSAA_ENABLE with sample or centroid inputs");
    }
    queue = depthTarget(false);
    queue.context[0x293] = 0x10000u;
    queue.context[0x1b3] = queue.context[0x1b4] = 2u | ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::Ancillary) | ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::SampleCoverage);
    Require(AgcDriver::Graphics::PixelProgramSkipped(queue), "the depth-only draw did not skip its pixel program");
    requireOneSampleDepth(queue, 0, "a one-sample depth target whose skipped pixel program has PS_ITER_SAMPLE and sample index and coverage inputs");
    for (const bool color : {false, true}) {
        queue = depthTarget(color);
        queue.context[0x203] |= 0x40u;
        queue.context[0x1b3] = queue.context[0x1b4] = 2u | ShaderRecompiler::PixelInputBit(ShaderRecompiler::PixelInput::Ancillary);
        Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "the ANCILLARY draw skipped its pixel program");
        requireOneSampleDepth(queue, color ? 1u : 0u, "a one-sample depth target whose running pixel program reads ANCILLARY without PS_ITER_SAMPLE");
        queue.context[0x1b3] = queue.context[0x1b4] = 0x2b01u;
        requireOneSampleDepth(queue, color ? 1u : 0u, "a one-sample depth target whose running pixel program reads PERSP_SAMPLE, the position and ANCILLARY without PS_ITER_SAMPLE");
        queue = depthTarget(color);
        queue.context[0x203] |= 0x40u;
        queue.context[0x293] = 0x10000u;
        Require(!AgcDriver::Graphics::PixelProgramSkipped(queue), "the PS_ITER_SAMPLE draw skipped its pixel program");
        requireOneSampleDepth(queue, color ? 1u : 0u, "a one-sample depth target whose running pixel program has PS_ITER_SAMPLE and no sample index or coverage input");
        Require(!DecodeState(queue).perSampleShading, "a PS_ITER_SAMPLE program drawn as one sample still shades per sample");
        for (const auto eqaa : {0x00040000u, 0x00050000u}) {
            queue = depthTarget(color);
            queue.context[0x201] = eqaa;
            requireOneSampleDepth(queue, color ? 1u : 0u, "a one-sample depth target under a 4x PA_SC_AA_CONFIG without MSAA_ENABLE with DB_EQAA INTERPOLATE_COMP_Z");
        }
    }
    queue = depthTarget(true);
    queue.context[0x31d] = 0x12000;
    expectFailure([&] { DecodeState(queue); }, "a color target's sample count differs from the rasterizer's");
}

void UnwrittenDepthPlaneTests() {
    using AgcDriver::Graphics::DecodeState;
    using AgcDriver::Graphics::DrawRejection;
    const auto fresh = [](std::uint32_t depthControl) {
        auto written = makeState();
        written.context[0x200] = depthControl;
        written.context[0x1b3] = 2;
        written.context[0x1b4] = 2;
        AgcDriver::QueueState queue;
        queue.shader = written.shader;
        queue.userConfig = written.userConfig;
        written.context.ForEach([&](std::uint32_t offset, std::uint32_t value) { queue.context[offset] = value; });
        return queue;
    };
    auto queue = fresh(0x777);
    auto state = DecodeState(queue);
    Require(!state.depth && !state.depthTest && !state.depthWrite && !state.stencilTest, "depth and stencil tests on unwritten depth and stencil planes were kept (PPSA24040's DB_DEPTH_CONTROL)");
    auto rejection = DrawRejection(queue, false);
    Require(rejection.empty(), "the precheck refused depth and stencil tests on unwritten depth and stencil planes: " + rejection);
    queue = fresh(0x77f);
    state = DecodeState(queue);
    Require(!state.depth && !state.depthBoundsTest, "a depth bounds test on unwritten depth and stencil planes was kept");
    rejection = DrawRejection(queue, false);
    Require(rejection.empty(), "the precheck refused a depth bounds test on unwritten depth and stencil planes: " + rejection);
    queue = fresh(0x007007b3);
    queue.context[0x010] = 0x80000183;
    queue.context[0x012] = 0x00001000;
    queue.context[0x014] = 0x00001000;
    queue.context[0x007] = 0x0001001f;
    queue.context[0x00b] = 0x3f800000;
    state = DecodeState(queue);
    Require(state.depth && state.depthTest && !state.stencilTest && state.renderExtent.width == 32 && state.renderExtent.height == 2, "a depth plane beside an unwritten DB_STENCIL_INFO did not decode as depth only");
    rejection = DrawRejection(queue, false);
    Require(rejection.empty(), "the precheck refused a depth plane beside an unwritten DB_STENCIL_INFO: " + rejection);
    queue = fresh(0x007007b3);
    queue.context[0x011] = 0x20000181;
    queue.context[0x013] = 0x00002000;
    queue.context[0x015] = 0x00002000;
    queue.context[0x007] = 0x003f003f;
    queue.context[0x00b] = 0x3f800000;
    queue.context[0x10c] = 0x01ffff00;
    queue.context[0x10d] = 0x01ffff00;
    state = DecodeState(queue);
    Require(!state.depthTest && state.stencilTest, "a stencil plane beside an unwritten DB_Z_INFO kept the depth test or lost the stencil test");
    rejection = DrawRejection(queue, false);
    Require(rejection.empty(), "the precheck refused a stencil plane beside an unwritten DB_Z_INFO: " + rejection);
    queue.context[0x010] = 0x80000183;
    queue.ClearContext();
    for (const auto offset : {0x010u, 0x011u}) Require(queue.context.contains(offset) && queue.context.at(offset) == 0, "CLEAR_STATE did not reset DB_Z_INFO and DB_STENCIL_INFO to their clear-state value 0");
}

void MsaaEnableSingleSampleTests() {
    using AgcDriver::Graphics::DecodeState;
    using AgcDriver::Graphics::DrawRejection;
    const auto enabled = [] {
        auto queue = makeState();
        queue.context[0x1b3] = 2;
        queue.context[0x1b4] = 2;
        queue.context[0x292] = 3;
        for (std::uint32_t pixel = 0; pixel < 4; ++pixel) queue.context[0x2fe + pixel * 4u] = 0;
        return queue;
    };
    const auto refused = [](const AgcDriver::QueueState& queue, std::string_view reason) {
        expectFailure([&] { DecodeState(queue); }, reason);
        const auto rejection = DrawRejection(queue, false);
        Require(rejection.find(reason) != std::string::npos, "the precheck did not refuse MSAA_ENABLE on a single-sample target (" + std::string(reason) + "): " + rejection);
    };
    auto queue = enabled();
    auto clear = queue;
    clear.context[0x292] = 2;
    const auto baseline = DecodeState(clear);
    for (const auto locations : {0x00000000u, 0xcc44cc00u}) {
        for (std::uint32_t pixel = 0; pixel < 4; ++pixel) queue.context[0x2fe + pixel * 4u] = locations;
        const auto state = DecodeState(queue);
        Require(state.samples == 1 && state.colors.size() == 1 && state.colors[0].samples == 1 && !state.alphaToCoverage, "MSAA_ENABLE on a single-sample target did not decode as single-sampled");
        Require(state.topology == baseline.topology && state.conservativeRasterization == baseline.conservativeRasterization && state.cullMode == baseline.cullMode && state.frontFace == baseline.frontFace, "MSAA_ENABLE on a single-sample target changed the rasterizer state");
        Require(state.scissor.offset.x == baseline.scissor.offset.x && state.scissor.offset.y == baseline.scissor.offset.y && state.scissor.extent.width == baseline.scissor.extent.width && state.scissor.extent.height == baseline.scissor.extent.height, "MSAA_ENABLE on a single-sample target changed the scissor");
        Require(state.colors[0].address == baseline.colors[0].address && state.colors[0].format == baseline.colors[0].format && state.blend.colorWriteMask == baseline.blend.colorWriteMask, "MSAA_ENABLE on a single-sample target changed the color target");
        const auto rejection = DrawRejection(queue, false);
        Require(rejection.empty(), "the precheck refused MSAA_ENABLE on a single-sample target: " + rejection);
    }
    for (std::uint32_t pixel = 0; pixel < 4; ++pixel) {
        for (const auto location : {0x04u, 0x40u, 0xc4u}) {
            queue = enabled();
            queue.context[0x2fe + pixel * 4u] = location;
            refused(queue, "off the pixel center");
        }
    }
    queue = enabled();
    queue.context.erase(0x306);
    expectFailure([&] { DecodeState(queue); }, "missing register");
    queue = enabled();
    queue.context[0x203] &= ~0x800u;
    queue.context[0x2dc] = 0xaa01;
    refused(queue, "MSAA_ENABLE on a single-sample target with alpha-to-coverage");
    queue = enabled();
    queue.context[0x2dc] = 0xaa01;
    refused(queue, "MSAA_ENABLE on a single-sample target with alpha-to-coverage");
    queue.context[0x2dc] = 0x18701;
    Require(!DecodeState(queue).alphaToCoverage && DrawRejection(queue, false).empty(), "MSAA_ENABLE on a single-sample target ignored the dithered alpha disable override");
    queue = enabled();
    queue.context[0x313] = 0x6001;
    refused(queue, "MSAA_ENABLE on a single-sample target with conservative rasterization");
    for (const auto primitive : {2u, 3u}) {
        queue = enabled();
        queue.userConfig[0x242] = primitive;
        refused(queue, "primitives other than triangles and rectangles");
    }
    queue = enabled();
    queue.userConfig[0x242] = 1;
    Require(DrawRejection(queue, false).find("primitives other than triangles and rectangles") != std::string::npos, "the precheck accepted MSAA_ENABLE on a single-sample target for points");
    for (const auto primitive : {5u, 6u, 7u, 17u, 21u}) {
        queue = enabled();
        queue.userConfig[0x242] = primitive;
        Require(DecodeState(queue).samples == 1 && DrawRejection(queue, false).empty(), "MSAA_ENABLE on a single-sample target was refused for a filled triangle or rectangle primitive");
    }
    queue = enabled();
    queue.context[0x292] = 7;
    refused(queue, "scan conversion mode");
    clear.context[0x2fe] = 0x44;
    clear.context[0x203] &= ~0x800u;
    clear.context[0x2dc] = 0xaa01;
    Require(DecodeState(clear).alphaToCoverage && DrawRejection(clear, false).empty(), "a single-sample target without MSAA_ENABLE was refused over its sample locations or alpha-to-coverage");
}

void ShaderStageTests() {
    auto queue = makeState();
    for (const auto routing : {0x2000u, 0x2010u, 0x02002000u, 0x02002010u}) {
        for (const auto vertexWave32 : {false, true}) {
            for (const auto fragmentWave32 : {false, true}) {
                queue.context[0x2d5] = routing | (vertexWave32 ? 0x00400000u : 0u);
                queue.context[0x1b6] = fragmentWave32 ? 0x8000u : 0u;
                const auto state = AgcDriver::Graphics::DecodeState(queue);
                Require(state.stages.path == AgcDriver::Graphics::ShaderPath::Vertex, "vertex routing changed");
                Require(state.stages.vertexWaveSize == (vertexWave32 ? 32u : 64u), "incorrect vertex wave size");
                Require(state.stages.fragmentWaveSize == (fragmentWave32 ? 32u : 64u), "incorrect fragment wave size");
            }
        }
    }
    {
        auto vertex = makeState();
        vertex.context[0x2d5] = 0x02002008u;
        const auto state = AgcDriver::Graphics::DecodeState(vertex);
        Require(state.stages.path == AgcDriver::Graphics::ShaderPath::Vertex
            && state.stages.registerValue == 0x02002008u,
            "the reported vertex-routing state did not decode as vertex");
        vertex.context[0x2d5] |= 0x10u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(vertex); }, "reserved LS_EN, ES_EN or VS_EN");
        vertex.context[0x2d5] = 0x02002008u | 0x800000u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(vertex); }, "unsupported vertex routing");
        vertex.context[0x2d5] = 0x2008u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(vertex); }, "unsupported vertex routing");
        vertex.context.erase(0x2d5);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(vertex); }, "missing register");
    }
    queue = makeState();
    queue.context[0x2d5] = 0x2020;
    queue.userConfig[0x25b] = (64u << 9u) | 21u;
    queue.context[0x1ff] = 64;
    queue.context[0x2ce] = 3;
    queue.context[0x29b] = 2;
    queue.context[0x2ab] = 4;
    queue.shader[0x8a] = 3u << 29u;
    queue.shader[0x8b] = 3u << 16u;
    auto stages = AgcDriver::Graphics::DecodeState(queue).stages;
    Require(stages.path == AgcDriver::Graphics::ShaderPath::Geometry && stages.mesh && stages.mesh->primitivesPerGroup == 21 && stages.mesh->verticesPerGroup == 63, "geometry assembly changed");
    Require(stages.mesh->maxVertices == 64 && stages.mesh->maxPrimitives == 21 && stages.mesh->threadsPerGroup == 64 && stages.mesh->esgsItemSize == 4, "geometry subgroup outputs changed");
    queue.shader[0x8a] = 0x222c0007u;
    queue.shader[0x8b] = 0x00c00010u;
    stages = AgcDriver::Graphics::DecodeState(queue).stages;
    Require(stages.path == AgcDriver::Graphics::ShaderPath::Geometry && stages.mesh && stages.mesh->primitivesPerGroup == 21 && stages.mesh->esgsItemSize == 4, "a geometry program with fewer GS and ES input VGPRs did not decode");
    queue.shader[0x8a] = 3u << 29u;
    queue.shader[0x8b] = 3u << 16u;
    {
        auto fan = makeState();
        fan.userConfig[0x242] = 5;
        fan.userConfig[0x24b] = 1;
        fan.context[0x103] = 0xffffffffu;
        fan.context[0x2d5] = 0x2030;
        fan.userConfig[0x25b] = 0x4020;
        fan.context[0x1ff] = 256;
        fan.context[0x2ce] = 8;
        fan.context[0x29b] = 2;
        fan.context[0x2ab] = 4;
        fan.shader[0x8a] = 3u << 29u;
        fan.shader[0x8b] = 3u << 16u;
        fan.context[0x1b3] = 2;
        fan.context[0x1b4] = 2;
        const auto state = AgcDriver::Graphics::DecodeState(fan);
        Require(AgcDriver::Graphics::DrawRejection(fan, true).empty(), "the precheck rejected an indexed triangle fan with restart into a geometry shader");
        Require(state.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN && state.primitiveRestart && state.stages.path == AgcDriver::Graphics::ShaderPath::Geometry && state.stages.mesh, "a triangle fan did not decode as geometry input");
        const auto& mesh = *state.stages.mesh;
        Require(mesh.inputPrimitive == 5 && mesh.primitivesPerGroup == 30 && mesh.verticesPerGroup == 32 && mesh.maxVertices == 256 && mesh.maxPrimitives == 192 && mesh.threadsPerGroup == 256 && mesh.esgsItemSize == 4, "triangle fan subgroup assembly changed");
        fan.userConfig[0x25b] = (3u << 9u) | 3u;
        Require(AgcDriver::Graphics::DecodeState(fan).stages.mesh->primitivesPerGroup == 1, "a three-vertex subgroup did not take one fan triangle");
        fan.userConfig[0x25b] = (2u << 9u) | 3u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(fan); }, "invalid geometry subgroup");
        fan.userConfig[0x25b] = 0x4020;
        fan.userConfig[0x242] = 3;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(fan); }, "unsupported geometry input or output assembly");
    }
    {
        auto fast = makeState();
        fast.userConfig[0x242] = 1;
        fast.context[0x2d5] = 0x482030;
        fast.userConfig[0x25b] = 0x201;
        fast.context[0x1ff] = 32;
        fast.context[0x2ce] = 32;
        fast.context[0x2d3] = 64;
        fast.context[0x29b] = 2;
        fast.context[0x2ab] = 4;
        fast.shader[0x8a] = 0x022c0245;
        fast.shader[0x8b] = 0x9002a;
        const auto state = AgcDriver::Graphics::DecodeState(fast);
        Require(state.stages.path == AgcDriver::Graphics::ShaderPath::Geometry && state.stages.vertexWaveSize == 32 && state.stages.mesh, "a fast-launch draw did not decode as wave32 geometry");
        const auto& mesh = *state.stages.mesh;
        Require(mesh.inputPrimitive == 1 && mesh.primitivesPerGroup == 1 && mesh.verticesPerGroup == 1 && mesh.maxVertices == 32 && mesh.maxPrimitives == 64 && mesh.threadsPerGroup == 64 && mesh.ldsSizeDwords == 128, "a fast-launch subgroup did not decode as one point amplified by PRIM_AMP_FACTOR");
        fast.context[0x29b] = 1;
        const auto fastLines = AgcDriver::Graphics::DecodeState(fast);
        Require(fastLines.stages.mesh->outputPrimitiveSize == 2 && fastLines.stages.mesh->maxPrimitives == 64 && AgcDriver::Graphics::RasterPrimitiveOf(fastLines) == ShaderRecompiler::RasterPrimitive::Line, "a fast-launch line strip did not take its line count from PRIM_AMP_FACTOR");
        fast.context[0x29b] = 2;
        fast.context[0x2d5] = 0x502030;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(fast); }, "unsupported geometry routing, fast launch or wave-ID state");
        fast.context[0x2d5] = 0x482030;
        fast.userConfig[0x25b] = 0x402;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(fast); }, "fast launch other than one point per subgroup");
        fast.userConfig[0x25b] = 0x201;
        fast.userConfig[0x242] = 4;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(fast); }, "fast launch other than one point per subgroup");
        fast.userConfig[0x242] = 1;
        fast.context[0x2d3] = 0;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(fast); }, "amplifies to no primitives");
    }
    {
        auto lines = makeState();
        lines.userConfig[0x242] = 1;
        lines.context[0x2d5] = 0x2030;
        lines.userConfig[0x25b] = 0x6633;
        lines.context[0x1ff] = 255;
        lines.context[0x2ce] = 5;
        lines.context[0x29b] = 1;
        lines.context[0x2ab] = 4;
        lines.shader[0x8a] = 3u << 29u;
        lines.shader[0x8b] = 3u << 16u;
        auto state = AgcDriver::Graphics::DecodeState(lines);
        Require(state.stages.path == AgcDriver::Graphics::ShaderPath::Geometry && state.stages.mesh && AgcDriver::Graphics::RasterPrimitiveOf(state) == ShaderRecompiler::RasterPrimitive::Line, "a geometry program that outputs line strips did not decode as geometry rasterized as lines");
        auto mesh = *state.stages.mesh;
        Require(mesh.inputPrimitive == 1 && mesh.primitivesPerGroup == 51 && mesh.verticesPerGroup == 51 && mesh.maxVertices == 255 && mesh.maxPrimitives == 255 && mesh.threadsPerGroup == 256 && mesh.outputPrimitiveSize == 2, "a line-strip subgroup did not decode as 51 points with a line slot per output vertex");
        lines.context[0x29b] = 2;
        state = AgcDriver::Graphics::DecodeState(lines);
        mesh = *state.stages.mesh;
        Require(mesh.maxPrimitives == 153 && mesh.threadsPerGroup == 256 && mesh.outputPrimitiveSize == 3 && AgcDriver::Graphics::RasterPrimitiveOf(state) == ShaderRecompiler::RasterPrimitive::Triangle, "the triangle-strip subgroup of the same draw changed");
        Require(mesh.provokingVertex == 0, "a first-vertex geometry draw does not take its flat inputs and layer from the first vertex of each output primitive");
        const auto raster = lines.context.at(0x205);
        lines.context[0x205] = raster | (1u << 19u);
        Require(AgcDriver::Graphics::DecodeState(lines).stages.mesh->provokingVertex == 2 && AgcDriver::Graphics::DecodeShaderStages(lines).mesh->provokingVertex == 2, "a last-vertex geometry draw does not take its flat inputs and layer from the third vertex of each output triangle");
        lines.context[0x29b] = 1;
        Require(AgcDriver::Graphics::DecodeState(lines).stages.mesh->provokingVertex == 1, "a last-vertex geometry draw does not take its flat inputs and layer from the second vertex of each output line");
        lines.context[0x29b] = 0;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(lines); }, "missing register in context bank at DWORD 0x280");
        lines.context[0x280] = 0x00080008u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(lines); }, "missing register in context bank at DWORD 0x281");
        lines.context[0x281] = 0xffff0000u;
        state = AgcDriver::Graphics::DecodeState(lines);
        mesh = *state.stages.mesh;
        Require(mesh.outputPrimitiveSize == 1 && mesh.maxPrimitives == 255 && AgcDriver::Graphics::RasterPrimitiveOf(state) == ShaderRecompiler::RasterPrimitive::Point, "a point-list subgroup did not decode as points");
        Require(mesh.provokingVertex == 0, "a point draw has provoking vertex 0");
        Require(state.pointSize == 1.0f && state.pointSizeMin == 0.0f && state.pointSizeMax == 8191.875f, "geometry points did not decode the queue's default point size and limits");
        lines.context[0x205] = raster;
        auto points = lines;
        points.userConfig[0x242] = 6;
        points.userConfig[0x25b] = 0x5c2e;
        points.context[0x1ff] = 0x8a;
        points.context[0x2ce] = 3;
        points.context[0x29b] = 0;
        points.context[0x280] = 0x00180018u;
        points.context[0x281] = 0x00400008u;
        const auto pointState = AgcDriver::Graphics::DecodeState(points);
        const auto& pointMesh = *pointState.stages.mesh;
        ShaderRecompiler::ShaderVertexStageInfo pointInfo{};
        AgcDriver::Graphics::ApplyPointSize(pointState, pointInfo);
        Require(pointInfo.pointList && pointInfo.pointSize == 3.0f && pointInfo.pointSizeMin == 1.0f && pointInfo.pointSizeMax == 8.0f, "geometry points lost their register sizes");
        Require(pointMesh.outputPrimitiveSize == 1u && pointMesh.primitivesPerGroup == 44u && pointMesh.verticesPerGroup == 46u && pointMesh.maxVertices == 138u && pointMesh.maxPrimitives == 138u && pointMesh.threadsPerGroup == 192u && AgcDriver::Graphics::RasterPrimitiveOf(pointState) == ShaderRecompiler::RasterPrimitive::Point, "triangle-strip input with point output did not decode the reported subgroup");
        lines.context[0x29b] = 1;
        lines.context[0x2ce] = 1;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(lines); }, "unsupported geometry input or output assembly");
        lines.context[0x2ce] = 5;
        for (const auto output : {3u, 0x80000001u, 0x101u}) {
            lines.context[0x29b] = output;
            expectFailure([&] { AgcDriver::Graphics::DecodeState(lines); }, "VGT_GS_OUT_PRIM_TYPE other than a triangle strip, or a point list or line strip from a geometry program, is unsupported");
        }
        lines.userConfig[0x242] = 4;
        lines.context[0x205] = 0x2a40;
        lines.context[0x29b] = 2;
        Require(AgcDriver::Graphics::DecodeState(lines).stages.mesh->primitivesPerGroup == 17, "triangle input with a triangle-strip output and a triangle polygon offset did not decode");
        lines.context[0x29b] = 1;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(lines); }, "depth bias or nonstandard rasterization");
    }
    queue.context[0x2ab] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "invalid VGT_ESGS_RING_ITEMSIZE");
    queue.context[0x2ab] = 4;
    queue.userConfig[0x25b] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "invalid geometry subgroup");
    queue = makeState();
    queue.context[0x2d5] = 0x200d;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "Patch topology and HS_EN disagree");
    queue.userConfig[0x242] = 9;
    queue.userConfig[0x25b] = 0x1fe55;
    queue.context[0x2d6] = 0xc355;
    queue.context[0x2db] = 0x40049;
    queue.context[0x286] = 0x42800000;
    queue.context[0x287] = 0x3f800000;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported tessellation routing");
    queue.context[0x2d5] = 0x0200010d;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported tessellation routing");
    queue.context[0x2d5] = 0x0200210d;
    {
        std::vector<AgcDriver::Graphics::RegisterRead> tessellationLog;
        {
            const AgcDriver::Graphics::RegisterReadLogScope logScope(&tessellationLog);
            stages = AgcDriver::Graphics::DecodeState(queue).stages;
        }
        for (const auto read : tessellationLog) Require(AgcDriver::Graphics::DrawKeyCovers(read), "DrawKeyRegisters lacks a tessellation register the decoder reads: " + std::to_string(read.offset));
    }
    Require(stages.path == AgcDriver::Graphics::ShaderPath::Tessellation && stages.tessellation, "tessellation routing changed");
    const auto& tessellation = *stages.tessellation;
    Require(tessellation.inputControlPoints == 3 && tessellation.outputControlPoints == 3 && tessellation.domain == 1 && tessellation.partitioning == 2 && tessellation.outputTopology == 2 && stages.minTessellationLevel == 1.0f && stages.maxTessellationLevel == 64.0f && tessellation.patchesPerGroup == 85 && tessellation.primitiveGroupSize == 85 && !tessellation.breakAtInstance, "the passthrough triangle-patch configuration decoded differently");
    for (const auto [parameters, reason] : {std::pair{0x44049u, "VGT_TF_PARAM"}, std::pair{0x140049u, "VGT_TF_PARAM"}, std::pair{0x2040049u, "VGT_TF_PARAM"}, std::pair{0x40059u, "fractional-odd partitioning"}, std::pair{0x40045u, "fractional-odd partitioning"}, std::pair{0x40048u, "fractional-odd partitioning"}, std::pair{0x40029u, "fractional-odd partitioning"}, std::pair{0x4004bu, "fractional-odd partitioning"}}) {
        queue.context[0x2db] = parameters;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, reason);
    }
    queue.context[0x2db] = 0x40041;
    Require(AgcDriver::Graphics::DecodeState(queue).stages.tessellation->partitioning == 0, "integer-partitioned triangle patches decoded differently");
    queue.context[0x2db] = 0x4004d;
    Require(AgcDriver::Graphics::DecodeState(queue).stages.tessellation->partitioning == 3, "fractional-even-partitioned triangle patches decoded differently");
    queue.context[0x2d6] = 0x1043f;
    queue.context[0x2db] = 0x4006a;
    queue.context[0x286] = 0x41100000;
    queue.userConfig[0x25b] = 0x5fe3f;
    const auto quadStages = AgcDriver::Graphics::DecodeState(queue).stages;
    const auto& quads = quadStages.tessellation;
    Require(quads && quads->inputControlPoints == 4 && quads->outputControlPoints == 4 && quads->domain == 2 && quads->partitioning == 2 && quads->outputTopology == 3 && quadStages.minTessellationLevel == 1.0f && quadStages.maxTessellationLevel == 9.0f && quads->patchesPerGroup == 63 && quads->primitiveGroupSize == 63 && quads->breakAtInstance, "counter-clockwise quad patches with HOS levels [1, 9], 63 patches per group and BREAK_WAVE_AT_EOI decoded differently");
    queue.userConfig[0x25b] = 0x8040;
    const auto shared = AgcDriver::Graphics::DecodeState(queue).stages.tessellation;
    Require(shared && shared->primitiveGroupSize == 64 && !shared->breakAtInstance, "GE_CNTL 0x8040 did not decode as 64 primitives per group with BREAK_WAVE_AT_EOI clear");
    queue.userConfig.erase(0x25b);
    const auto missing = AgcDriver::Graphics::DecodeState(queue).stages.tessellation;
    Require(missing->primitiveGroupSize == 0 && !missing->breakAtInstance, "a draw without GE_CNTL decoded a primitive group size or BREAK_WAVE_AT_EOI");
    queue.context[0x2db] = 0x40069;
    Require(AgcDriver::Graphics::DecodeState(queue).stages.tessellation->outputTopology == 3, "counter-clockwise triangle patches decoded differently");
    for (const auto [maximum, minimum, reason] : {std::tuple{0x3f800000u, 0x40000000u, "tessellation level clamps"}, std::tuple{0x41100000u, 0xbf800000u, "tessellation level clamps"}, std::tuple{0x3f000000u, 0x00000000u, "tessellation level clamps"}, std::tuple{0x42800000u, 0x42800000u, "tessellation level clamps"}, std::tuple{0x7fc00000u, 0x3f800000u, "non-finite register"}, std::tuple{0x7f800000u, 0x3f800000u, "non-finite register"}}) {
        queue.context[0x286] = maximum;
        queue.context[0x287] = minimum;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, reason);
    }
    queue.context[0x2d6] = 0xc355;
    queue.context[0x2db] = 0x40049;
    queue.context[0x286] = 0x42800000;
    queue.context[0x287] = 0x3f800000;
    queue.context[0x2d5] = 0x0260210d;
    {
        const auto wave32 = AgcDriver::Graphics::DecodeState(queue).stages;
        Require(wave32.path == AgcDriver::Graphics::ShaderPath::Tessellation && wave32.hullWaveSize == 32 && wave32.vertexWaveSize == 32 && !wave32.tessellationGeometry, "HS_W32 and GS_W32 tessellation routing decoded differently");
    }
    queue.context[0x2d5] = 0x60212d;
    queue.context[0x29b] = 2;
    queue.context[0x2ce] = 6;
    queue.context[0x1ff] = 0xc0;
    queue.context[0x2ab] = 4;
    queue.userConfig[0x25b] = 0x4020;
    queue.shader[0x8b] = 0x67001c;
    {
        std::vector<AgcDriver::Graphics::RegisterRead> geometryLog;
        AgcDriver::Graphics::ShaderStages geometry;
        {
            const AgcDriver::Graphics::RegisterReadLogScope logScope(&geometryLog);
            geometry = AgcDriver::Graphics::DecodeState(queue).stages;
        }
        for (const auto read : geometryLog) Require(AgcDriver::Graphics::DrawKeyCovers(read), "DrawKeyRegisters lacks a tessellation geometry register the decoder reads: " + std::to_string(read.offset));
        Require(geometry.path == AgcDriver::Graphics::ShaderPath::TessellationGeometry && geometry.tessellation && !geometry.mesh && geometry.hullWaveSize == 32 && geometry.vertexWaveSize == 32, "tessellation followed by an NGG geometry stage decoded differently");
        const auto& subgroup = geometry.tessellationGeometry;
        Require(subgroup && subgroup->inputPrimitive == 4 && subgroup->primitivesPerGroup == 10 && subgroup->verticesPerGroup == 30 && subgroup->maxVertices == 192 && subgroup->maxPrimitives == 128 && subgroup->threadsPerGroup == 192 && subgroup->ldsSizeDwords == 1536 && subgroup->esgsItemSize == 4, "the geometry subgroup after tessellation decoded differently");
    }
    queue.context[0x2d5] = 0x0260212d;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported tessellation and geometry routing");
    queue.context[0x2d5] = 0x60012d;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported tessellation and geometry routing");
    queue.context.erase(0x29b);
    queue.context.erase(0x2ce);
    queue.context.erase(0x1ff);
    queue.context.erase(0x2ab);
    queue.shader.erase(0x8b);
    queue.userConfig.erase(0x25b);
    queue.context[0x2d5] = 0x0200210d;
    queue.context[0x2d6] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "control-point counts");
    queue.context[0x2d6] = 0xc300;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "NUM_PATCHES is zero");
    queue = makeState();
    for (const auto value : {0x2003u, 0x2018u, 0x20c0u, 0x80002000u}) {
        queue.context[0x2d5] = value;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "reserved");
    }
    for (const auto bit : {1u, 8u, 0x40u, 0x100u, 0x200u, 0x400u, 0x1000u, 0x4000u, 0x80000u, 0x200000u, 0x800000u, 0x1000000u}) {
        queue.context[0x2d5] = 0x2000u | bit;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported vertex");
    }
    queue.context[0x2d5] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "without PRIMGEN_EN");
    queue.context.erase(0x2d5);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "missing register");
    queue.context[0x2d5] = 0x2000;
    queue.context.erase(0x1b6);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "missing register");
}

void UnsupportedTessellationFieldTests() {
    auto queue = makeState();
    queue.context[0x2d5] = 0x0200210d;
    queue.userConfig[0x242] = 9;
    queue.userConfig[0x25b] = 0x1fe55;
    queue.context[0x286] = 0x42800000;
    queue.context[0x287] = 0x3f800000;
    for (const auto [config, parameters, fields] : {
             std::tuple{0xc355u, 0x40045u, "VGT_TF_PARAM=0x40045 (TYPE=1 TESS_TRIANGLE, PARTITIONING=1 PART_POW2, TOPOLOGY=2 OUTPUT_TRIANGLE_CW), VGT_LS_HS_CONFIG=0xc355 (NUM_PATCHES=85, HS_NUM_INPUT_CP=3, HS_NUM_OUTPUT_CP=3)"},
             std::tuple{0x1033fu, 0x40066u, "VGT_TF_PARAM=0x40066 (TYPE=2 TESS_QUAD, PARTITIONING=1 PART_POW2, TOPOLOGY=3 OUTPUT_TRIANGLE_CCW), VGT_LS_HS_CONFIG=0x1033f (NUM_PATCHES=63, HS_NUM_INPUT_CP=3, HS_NUM_OUTPUT_CP=4)"},
             std::tuple{0xc355u, 0x40048u, "VGT_TF_PARAM=0x40048 (TYPE=0 TESS_ISOLINE, PARTITIONING=2 PART_FRAC_ODD, TOPOLOGY=2 OUTPUT_TRIANGLE_CW)"},
             std::tuple{0xc355u, 0x40029u, "(TYPE=1 TESS_TRIANGLE, PARTITIONING=2 PART_FRAC_ODD, TOPOLOGY=1 OUTPUT_LINE)"},
             std::tuple{0xc355u, 0x4000du, "(TYPE=1 TESS_TRIANGLE, PARTITIONING=3 PART_FRAC_EVEN, TOPOLOGY=0 OUTPUT_POINT)"},
             std::tuple{0xc355u, 0x4004bu, "(TYPE=3, PARTITIONING=2 PART_FRAC_ODD, TOPOLOGY=2 OUTPUT_TRIANGLE_CW)"},
             std::tuple{0xc355u, 0x400f1u, "(TYPE=1 TESS_TRIANGLE, PARTITIONING=4, TOPOLOGY=7)"}}) {
        queue.context[0x2d6] = config;
        queue.context[0x2db] = parameters;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "VGT_SHADER_STAGES_EN=0x200210d: VGT_TF_PARAM=0x");
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, fields);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "): only triangle and quad domains with integer, fractional-even or fractional-odd partitioning and triangle output are supported by the reference path");
    }
}

void NggVertexWorkgroupTests() {
    auto queue = makeState();
    queue.context[0x2d5] = 0x12010;
    queue.userConfig[0x242] = 4;
    queue.userConfig[0x25b] = 0x10080;
    queue.context[0x1ff] = 0x80;
    queue.context[0x2ce] = 0;
    queue.context[0x29b] = 2;
    queue.context[0x2ab] = 1;
    queue.shader[0x8a] = 0x222c0009;
    queue.shader[0x8b] = 0x43001e;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    Require(AgcDriver::Graphics::DecodeState(queue).stages.path == AgcDriver::Graphics::ShaderPath::Vertex, "an NGG vertex program without a workgroup left the vertex path");
    auto state = AgcDriver::Graphics::DecodeState(queue, true);
    Require(state.stages.path == AgcDriver::Graphics::ShaderPath::Geometry && state.stages.mesh, "an NGG vertex program that needs a workgroup did not take the mesh path");
    expectFailure([&] { AgcDriver::Graphics::ValidateShaders({}, state, VkPhysicalDeviceSubgroupProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES}, false, false, false, false, false, false, false, false, false, false, false, false, false); }, "device does not support mesh shaders");
    auto mesh = *state.stages.mesh;
    Require(mesh.inputPrimitive == 4 && mesh.primitivesPerGroup == 42 && mesh.verticesPerGroup == 126 && mesh.maxVertices == 128 && mesh.maxPrimitives == 128 && mesh.threadsPerGroup == 128 && mesh.ldsSizeDwords == 1024 && mesh.esgsItemSize == 1, "the NGG vertex subgroup of a triangle list changed");
    queue.userConfig[0x242] = 6;
    mesh = *AgcDriver::Graphics::DecodeState(queue, true).stages.mesh;
    Require(mesh.primitivesPerGroup == 126 && mesh.verticesPerGroup == 128 && mesh.maxPrimitives == 128, "the NGG vertex subgroup of a triangle strip is not bounded by its vertex budget");
    queue.shader[0x8a] = 0x222c0005;
    queue.shader[0x8b] = 0x400030;
    Require(AgcDriver::Graphics::DecodeState(queue, true).stages.mesh.has_value(), "an NGG vertex program that loads only its vertex ID was rejected");
    queue.shader[0x8a] = 0x222c0009;
    queue.shader[0x8b] = 0x43001e;
    queue.context[0x1ff] = 2;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue, true); }, "invalid geometry subgroup output");
    queue.context[0x1ff] = 0x80;
    for (const auto stages : {0x02012010u, 0x00012030u, 0x00010010u}) {
        queue.context[0x2d5] = stages;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue, true); }, "requires PRIMGEN_EN without GS_EN, PRIMGEN_PASSTHRU_EN or tessellation");
    }
    queue.context[0x2d5] = 0x12010;
    queue.context[0x29b] = 1;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue, true); }, "VGT_GS_OUT_PRIM_TYPE other than a triangle strip, or a point list or line strip from a geometry program, is unsupported");
    queue.context[0x29b] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue, true); }, "VGT_GS_OUT_PRIM_TYPE other than a triangle strip, or a point list or line strip from a geometry program, is unsupported");
}

constexpr std::array<std::uint8_t, 8> IdentityExports{0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u, 0xe4u};

std::vector<spv::BuiltIn> pixelBuiltinsRead(std::uint32_t ena, std::uint32_t addr, std::uint32_t source) {
    auto queue = makeState();
    queue.context[0x1b3] = ena;
    queue.context[0x1b4] = addr;
    const auto pixel = AgcDriver::Graphics::DecodePixelStageInfo(queue.context, IdentityExports);
    const std::array<std::uint32_t, 3> code{0xf800180fu, source * 0x01010101u, 0xbf810000u};
    ShaderRecompiler::RecompileRequest request{};
    request.shader = {ShaderRecompiler::ShaderStage::Fragment, 0x30000u, code, 0, {}};
    request.context.waveSize = 64;
    request.context.pixel = pixel;
    request.target.vulkanVersion = 0x00401000u;
    request.target.spirvVersion = 0x00010300u;
    request.target.subgroupSize = 64;
    request.target.fragmentShaderBarycentricEnabled = true;
    request.layout.pushConstantSizeBytes = 128;
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    const auto& words = result.spirv.Words();
    std::map<std::uint32_t, spv::BuiltIn> builtins;
    std::vector<spv::BuiltIn> read;
    for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
        const auto op = static_cast<spv::Op>(words[at] & 0xffffu);
        if (op == spv::OpDecorate && words[at + 2] == spv::DecorationBuiltIn) builtins[words[at + 1]] = static_cast<spv::BuiltIn>(words[at + 3]);
        if (op != spv::OpLoad && op != spv::OpAccessChain && op != spv::OpInBoundsAccessChain) continue;
        const auto found = builtins.find(words[at + 3]);
        if (found != builtins.end() && std::find(read.begin(), read.end(), found->second) == read.end()) read.push_back(found->second);
    }
    return read;
}

std::size_t sampleMaskInputs(std::uint32_t inputs, std::uint16_t quadMask) {
    auto queue = makeState();
    queue.context[0x1b3] = inputs;
    queue.context[0x1b4] = inputs;
    auto pixel = AgcDriver::Graphics::DecodePixelStageInfo(queue.context, IdentityExports);
    pixel.quadSampleMask[0] = quadMask;
    const auto source = ShaderRecompiler::PixelInputVgpr(inputs, ShaderRecompiler::PixelInput::SampleCoverage);
    const std::array<std::uint32_t, 3> code{0xf800180fu, source * 0x01010101u, 0xbf810000u};
    ShaderRecompiler::RecompileRequest request{};
    request.shader = {ShaderRecompiler::ShaderStage::Fragment, 0x30000u, code, 0, {}};
    request.context.waveSize = 64;
    request.context.pixel = pixel;
    request.target.vulkanVersion = 0x00401000u;
    request.target.spirvVersion = 0x00010300u;
    request.target.subgroupSize = 64;
    request.target.fragmentShaderBarycentricEnabled = true;
    request.layout.pushConstantSizeBytes = 128;
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    const auto& words = result.spirv.Words();
    std::map<std::uint32_t, std::uint32_t> storage;
    std::vector<std::uint32_t> sampleMasks;
    for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
        const auto op = static_cast<spv::Op>(words[at] & 0xffffu);
        if (op == spv::OpVariable) storage[words[at + 2]] = words[at + 3];
        if (op == spv::OpDecorate && words[at + 2] == spv::DecorationBuiltIn && words[at + 3] == spv::BuiltInSampleMask) sampleMasks.push_back(words[at + 1]);
    }
    return static_cast<std::size_t>(std::ranges::count_if(sampleMasks, [&](std::uint32_t id) { return storage[id] == spv::StorageClassInput; }));
}

bool readsBuiltin(const std::vector<spv::BuiltIn>& read, spv::BuiltIn builtin) {
    return std::find(read.begin(), read.end(), builtin) != read.end();
}

std::vector<std::uint32_t> pixelNoPerspectiveLocations(bool barycentricEnabled, std::uint32_t inputs = 0x22u) {
    auto queue = makeState();
    queue.context[0x1b3] = inputs;
    queue.context[0x1b4] = inputs;
    queue.context[0x1b6] = 2u;
    queue.context[0x191] = 0u;
    queue.context[0x192] = 1u;
    const auto pixel = AgcDriver::Graphics::DecodePixelStageInfo(queue.context, IdentityExports);
    const std::array<std::uint32_t, 8> code{0xc8100000u, 0xc8110001u, 0xc8140402u, 0xc8150403u, 0xf800180fu, 0x05040504u, 0xbf810000u, 0xbf810000u};
    ShaderRecompiler::RecompileRequest request{};
    request.shader = {ShaderRecompiler::ShaderStage::Fragment, 0x30000u, code, 0, {}};
    request.context.waveSize = 64;
    request.context.pixel = pixel;
    request.target.vulkanVersion = 0x00401000u;
    request.target.spirvVersion = 0x00010300u;
    request.target.subgroupSize = 64;
    request.target.fragmentShaderBarycentricEnabled = barycentricEnabled;
    request.layout.pushConstantSizeBytes = 128;
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    const auto& words = result.spirv.Words();
    std::map<std::uint32_t, std::uint32_t> locations;
    std::vector<std::uint32_t> noPerspective;
    std::uint32_t perVertex = 0;
    bool perspectiveBarycentrics = false;
    bool linearBarycentrics = false;
    for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
        if (static_cast<spv::Op>(words[at] & 0xffffu) != spv::OpDecorate) continue;
        if (words[at + 2] == spv::DecorationLocation) locations[words[at + 1]] = words[at + 3];
        if (words[at + 2] == spv::DecorationNoPerspective) noPerspective.push_back(words[at + 1]);
        if (words[at + 2] == spv::DecorationPerVertexKHR) perVertex++;
        if (words[at + 2] == spv::DecorationBuiltIn) {
            perspectiveBarycentrics |= words[at + 3] == spv::BuiltInBaryCoordKHR;
            linearBarycentrics |= words[at + 3] == spv::BuiltInBaryCoordNoPerspKHR;
        }
    }
    if (barycentricEnabled) Require(perVertex == 2u && perspectiveBarycentrics && linearBarycentrics, "explicit interpolation must preserve both parameter vertices and both barycentric inputs");
    if (!barycentricEnabled) Require(!result.barycentricEmulation.active, "v_interp_p1/p2 of their own I/J pairs did not keep host interpolation");
    std::vector<std::uint32_t> result2;
    for (const auto id : noPerspective) result2.push_back(locations.count(id) != 0 ? locations.at(id) : 0xffffffffu);
    return result2;
}

void ComputeScratchTests() {
    auto queue = makeState();
    auto& shader = queue.shader;
    shader[0x207] = 64u;
    shader[0x208] = 1u;
    shader[0x209] = 1u;
    shader[0x213] = 0x1u;
    std::vector<std::byte> header(sizeof(Shader));
    Shader agc{};
    agc.scratch_size_dw_per_thread = 24;
    std::memcpy(header.data(), &agc, sizeof(Shader));
    const auto compute = AgcDriver::Graphics::DecodeComputeStageInfo(shader, header);
    Require(compute.scratchDwords == 24u, "SCRATCH_EN did not take the AGC header's per-thread scratch size");
    agc.scratch_size_dw_per_thread = 0;
    std::memcpy(header.data(), &agc, sizeof(Shader));
    expectFailure([&] { static_cast<void>(AgcDriver::Graphics::DecodeComputeStageInfo(shader, header)); }, "zero scratch size");
    shader[0x213] = 0u;
    Require(AgcDriver::Graphics::DecodeComputeStageInfo(shader, {}).scratchDwords == 0u, "a dispatch without SCRATCH_EN got scratch");
    shader[0x207] = 0x00100040u;
    Require(AgcDriver::Graphics::DecodeComputeStageInfo(shader, {}).numThreads[0] == 64u, "the partial group size leaked into the group size");
    shader[0x207] = 64u;
    ShaderRecompiler::RecompileRequest request{};
    const std::array<std::uint32_t, 1> code{0xbf810000u};
    request.shader = {ShaderRecompiler::ShaderStage::Compute, 0x30000u, code, 0, {}};
    request.context.waveSize = 64;
    request.context.compute = compute;
    const ShaderRecompiler::RequestSerializer serializer;
    const auto back = serializer.Deserialize(serializer.Serialize(request));
    Require(back.request.context.compute->scratchDwords == 24u, "the compute scratch size did not survive serialization");
}

void shaderUserDataTailPaddingTests() {
    constexpr std::size_t userDataOffset = 288;
    constexpr auto userDataBytes = offsetof(ShaderUserData, sharp_resource_count) + sizeof(ShaderUserData{}.sharp_resource_count);
    static_assert(userDataBytes == 54);
    std::vector<std::byte> header(userDataOffset + userDataBytes);
    Shader shader{};
    shader.user_data = reinterpret_cast<ShaderUserData*>(header.data() + userDataOffset);
    std::memcpy(header.data(), &shader, sizeof(shader));
    const auto headerAddress = reinterpret_cast<std::uintptr_t>(header.data());
    const auto info = AgcDriver::Graphics::DecodeVertexStageInfo(header, headerAddress, {}, nullptr, true);
    Require(!info.fetchEmbedded, "a ShaderUserData block without trailing struct padding was rejected");
}

void ColorPipeBankXorTests() {
    constexpr std::uint32_t side = 128;
    constexpr std::size_t blockBytes = 65536;
    static std::vector<std::byte> storage(2 * blockBytes + side * side * 4);
    const auto block = (reinterpret_cast<std::uintptr_t>(storage.data()) + blockBytes - 1) / blockBytes * blockBytes;
    auto queue = makeState();
    queue.context[0x3b8] |= static_cast<std::uint32_t>(AgcDriver::Graphics::ColorTileMode::RenderTarget) << 14u;
    queue.context[0x3b0] = ((side - 1u) << 14u) | (side - 1u);
    for (const auto offset : {0xdu, 0x82u, 0x91u, 0x95u}) queue.context[offset] = (side << 16u) | side;
    for (const std::uint32_t pipeBankXor : {0u, 0x5600u, 0xff00u}) {
        const auto address = block + pipeBankXor;
        queue.context[0x318] = static_cast<std::uint32_t>(address >> 8u);
        queue.context[0x390] = static_cast<std::uint32_t>(address >> 40u);
        const auto color = AgcDriver::Graphics::DecodeState(queue).color;
        Require(color.address == block && color.surfaceAddress == block && color.pipeBankXor == pipeBankXor, "a SW_64KB_R_X color base did not split into its block base and pipe/bank XOR " + std::to_string(pipeBankXor));
    }
    queue.context[0x3b8] = 0x9000000u | (0x16u << 14u) | (0x14u << 19u);
    for (const std::uint32_t pipeBankXor : {0u, 0xa00u, 0xf00u}) {
        const auto address = block + 0x3000u + pipeBankXor;
        queue.context[0x318] = static_cast<std::uint32_t>(address >> 8u);
        queue.context[0x390] = static_cast<std::uint32_t>(address >> 40u);
        const auto color = AgcDriver::Graphics::DecodeState(queue).color;
        Require(color.tileMode == AgcDriver::Graphics::ColorTileMode::D4KBX && color.address == block + 0x3000u && color.pipeBankXor == pipeBankXor, "a SW_4KB_D_X color base did not split into its 4 KiB block base and pipe/bank XOR " + std::to_string(pipeBankXor));
    }
    queue.context[0x3b8] = 0x0dc68000u;
    for (const std::uint32_t pipeBankXor : {0u, 0x4600u, 0xcf00u}) {
        const auto address = block + pipeBankXor;
        queue.context[0x318] = static_cast<std::uint32_t>(address >> 8u);
        queue.context[0x390] = static_cast<std::uint32_t>(address >> 40u);
        const auto color = AgcDriver::Graphics::DecodeState(queue).color;
        Require(color.tileMode == AgcDriver::Graphics::ColorTileMode::D64KBX && color.address == block && color.surfaceAddress == block && color.pipeBankXor == pipeBankXor, "a SW_64KB_D_X color base did not split its block address and XOR");
    }
    queue.context[0x3b8] = 0x4dc60000u;
    for (const std::uint32_t pipeBankXor : {0u, 0x4600u, 0xff00u}) {
        const auto address = block + pipeBankXor;
        queue.context[0x318] = static_cast<std::uint32_t>(address >> 8u);
        queue.context[0x390] = static_cast<std::uint32_t>(address >> 40u);
        const auto color = AgcDriver::Graphics::DecodeState(queue).color;
        Require(color.tileMode == AgcDriver::Graphics::ColorTileMode::Z64KBX && color.address == block && color.surfaceAddress == block && color.pipeBankXor == pipeBankXor, "a SW_64KB_Z_X color base did not split its block address and XOR");
    }
    queue.context[0x3b8] = 0x0dc68000u;
    for (const std::uint32_t columnBits : {0x1000u, 0x2000u, 0x3000u}) {
        queue.context[0x318] = static_cast<std::uint32_t>((block + columnBits) >> 8u);
        queue.context[0x390] = static_cast<std::uint32_t>((block + columnBits) >> 40u);
        expectFailure([&queue] { AgcDriver::Graphics::DecodeColorBuffer(queue.context, 0); }, "unsupported display color pipe/bank XOR column bits");
    }
    static std::vector<std::byte> mipStorage(16u * blockBytes);
    const auto mipBase = (reinterpret_cast<std::uintptr_t>(mipStorage.data()) + blockBytes - 1u) / blockBytes * blockBytes;
    const auto mips = AgcDriver::Graphics::ComputeElementMipLayout(AgcDriver::Graphics::TextureTileMode::kD64KBX, 4, 256, 128, 5);
    const auto sliceBytes = AgcDriver::Graphics::ComputeSurfaceSize(mips, 1);
    queue.context[0x318] = static_cast<std::uint32_t>((mipBase + 0x0200u) >> 8u);
    queue.context[0x390] = static_cast<std::uint32_t>((mipBase + 0x0200u) >> 40u);
    queue.context[0x3b0] = (4u << 28u) | (255u << 14u) | 127u;
    queue.context[0x3b8] |= 1u;
    queue.context[0x31b] = (2u << 26u) | 1u | (1u << 13u);
    const auto mipColor = AgcDriver::Graphics::DecodeColorBuffer(queue.context, 0);
    Require(mipColor.mip == 2 && mipColor.mipCount == 5 && mipColor.extent.width == 64 && mipColor.extent.height == 32 && mipColor.surfaceAddress == mipBase + sliceBytes && mipColor.address == mipBase + sliceBytes + mips[2].tiledOffset && mipColor.mipTail == mips[2].tail && mipColor.pipeBankXor == 0x0200u, "a display XOR color view lost its array slice, mip placement or XOR");
}

void HiddenUserWordVertexTablesTests() {
    constexpr std::size_t userDataOffset = 288;
    constexpr std::size_t directOffset = 352;
    constexpr std::size_t semanticOffset = 384;
    static std::vector<std::byte> header(semanticOffset + sizeof(ShaderSemantic));
    static std::array<std::uint32_t, 4> bufferTable{0x1000u, 0x20000u, 64u, 0x3000u};
    static std::array<std::uint32_t, 1> attribTable{(0x2cu << 5u)};
    std::array<std::uint16_t, ShaderRegs::AGC_DIRECT_RESOURCE_TYPE_COUNT> direct{};
    direct.fill(ShaderRegs::AGC_ILLEGAL_DIRECT_OFFSET);
    direct[static_cast<std::size_t>(ShaderRegs::AgcDirectResourceType::PtrVertexBufferTable)] = 4;
    direct[static_cast<std::size_t>(ShaderRegs::AgcDirectResourceType::PtrVertexAttribDescTable)] = 6;
    std::memcpy(header.data() + directOffset, direct.data(), sizeof(direct));
    ShaderSemantic semantic{};
    semantic.size_in_elements = 1;
    std::memcpy(header.data() + semanticOffset, &semantic, sizeof(semantic));
    ShaderUserData userData{};
    userData.direct_resource_offset = reinterpret_cast<std::uint16_t*>(header.data() + directOffset);
    userData.direct_resource_count = ShaderRegs::AGC_DIRECT_RESOURCE_TYPE_COUNT;
    std::memcpy(header.data() + userDataOffset, &userData, sizeof(userData));
    Shader shader{};
    shader.user_data = reinterpret_cast<ShaderUserData*>(header.data() + userDataOffset);
    shader.input_semantics = reinterpret_cast<ShaderSemantic*>(header.data() + semanticOffset);
    shader.num_input_semantics = 1;
    std::memcpy(header.data(), &shader, sizeof(shader));
    const auto headerAddress = reinterpret_cast<std::uintptr_t>(header.data());
    const auto low = [](const void* pointer) { return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer)); };
    const auto high = [](const void* pointer) { return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer) >> 32u); };
    const std::array<std::uint32_t, 8> registers{0x31016facu, 0x7efff390u, 0x100002u, 0x49u, low(bufferTable.data()), high(bufferTable.data()), low(attribTable.data()), high(attribTable.data())};
    std::vector<std::uint32_t> merged(8, 0u);
    merged.insert(merged.end(), registers.begin(), registers.end());
    const auto info = AgcDriver::Graphics::DecodeVertexStageInfo(header, headerAddress, merged, nullptr, false, 8);
    Require(info.fetchEmbedded && info.fetchBufferReg == 4u && info.fetchAttribReg == 6u && info.resourcesNum == 1 && info.resources[0].fields[0] == 0x1000u && info.resources[0].fields[2] == 64u, "a merged program's vertex tables were not read past its eight hidden user words");
    expectFailure([&] { static_cast<void>(AgcDriver::Graphics::DecodeVertexStageInfo(header, headerAddress, merged, nullptr, false, 0)); }, "null vertex attribute table");
    const auto plain = AgcDriver::Graphics::DecodeVertexStageInfo(header, headerAddress, registers, nullptr, false, 0);
    Require(plain.fetchEmbedded && plain.fetchBufferReg == 4u && plain.resourcesNum == 1 && plain.resources[0].fields[0] == 0x1000u, "a vertex program's tables moved");
}

void HeaderlessVertexStageTests() {
    const std::array<std::uint32_t, 4> userData{1, 2, 3, 4};
    const auto info = AgcDriver::Graphics::DecodeVertexStageInfo({}, 0, userData);
    Require(!info.fetchEmbedded && info.resourcesNum == 0, "a vertex program without an AGC header received an embedded fetch table");
    const std::array<std::byte, 8> truncated{};
    expectFailure([&] { static_cast<void>(AgcDriver::Graphics::DecodeVertexStageInfo(truncated, 0x1000, userData)); }, "smaller than the fixed AGC header");
}

void UnexportedPixelInputTests() {
    ShaderRecompiler::ShaderPixelStageInfo pixel{};
    pixel.interpolatorCount = 5;
    pixel.interpolatorSettings[0] = 0x000u;
    pixel.interpolatorSettings[1] = 0x401u;
    pixel.interpolatorSettings[2] = 0x302u;
    pixel.interpolatorSettings[3] = 0x123u;
    pixel.interpolatorSettings[4] = 0x680004u;
    pixel.interpolatorSettings[5] = 0x005u;
    const std::array<std::uint32_t, 1> exported{0u};
    const auto adjusted = AgcDriver::Graphics::DefaultUnexportedInputs(pixel, exported);
    Require(adjusted.interpolatorSettings[0] == 0x000u, "an input of an exported parameter was changed");
    Require(adjusted.interpolatorSettings[1] == 0x021u, "a flat input of an unexported parameter did not read zero");
    Require(adjusted.interpolatorSettings[2] == 0x022u, "an input of an unexported parameter did not read zero");
    Require(adjusted.interpolatorSettings[3] == 0x123u, "an input that already reads its default value was changed");
    Require(adjusted.interpolatorSettings[4] == 0x180024u, "an FP16 input of an unexported parameter did not read zero in both halves");
    Require(adjusted.interpolatorSettings[5] == 0x005u, "a setting past SPI_PS_IN_CONTROL's input count was changed");
    const std::array<std::uint32_t, 5> all{0u, 1u, 2u, 4u, 5u};
    const auto unchanged = AgcDriver::Graphics::DefaultUnexportedInputs(pixel, all);
    Require(std::ranges::equal(unchanged.interpolatorSettings, pixel.interpolatorSettings), "inputs of exported parameters were changed");
}

void PixelInputLayoutTests() {
    using ShaderRecompiler::PixelInput;
    using ShaderRecompiler::PixelInputVgpr;
    auto queue = makeState();
    const auto decode = [&](std::uint32_t ena, std::uint32_t addr) {
        queue.context[0x1b3] = ena;
        queue.context[0x1b4] = addr;
        return AgcDriver::Graphics::DecodePixelStageInfo(queue.context, IdentityExports);
    };
    auto pixel = decode(0x326u, 0x326u);
    Require(pixel.inputAddr == 0x326u && pixel.hasPerspectiveCenterVgpr && pixel.perspectiveCentroid && pixel.noPerspective && !pixel.linearCentroid && pixel.posX && pixel.posY && !pixel.posZ, "the centroid input flags were not decoded");
    Require(PixelInputVgpr(pixel.inputAddr, PixelInput::PerspectiveCenter) == 0u && PixelInputVgpr(pixel.inputAddr, PixelInput::PerspectiveCentroid) == 2u && PixelInputVgpr(pixel.inputAddr, PixelInput::LinearCenter) == 4u && PixelInputVgpr(pixel.inputAddr, PixelInput::PositionX) == 6u && PixelInputVgpr(pixel.inputAddr, PixelInput::PositionY) == 7u, "the centroid layout moved the inputs");
    pixel = decode(0x1146u, 0x1146u);
    Require(pixel.linearCentroid && !pixel.noPerspective && PixelInputVgpr(pixel.inputAddr, PixelInput::LinearCentroid) == 4u && PixelInputVgpr(pixel.inputAddr, PixelInput::PositionX) == 6u && PixelInputVgpr(pixel.inputAddr, PixelInput::FrontFace) == 7u, "the linear centroid layout moved the inputs");
    Require(!pixel.frontFaceAllBits, "FRONT_FACE_ALL_BITS was decoded from a clear SPI_BARYC_CNTL");
    queue.context[0x1b8] = 1u << 24u;
    Require(decode(0x1146u, 0x1146u).frontFaceAllBits, "SPI_BARYC_CNTL FRONT_FACE_ALL_BITS was not decoded");
    Require(!decode(0x326u, 0x326u).frontFaceAllBits, "FRONT_FACE_ALL_BITS was decoded for a pixel program without the front-face input");
    using ShaderRecompiler::PixelPositionLocation;
    Require(decode(0x326u, 0x326u).positionLocation == PixelPositionLocation::Center, "POS_FLOAT_LOCATION 0 was not decoded as the pixel center");
    queue.context[0x1b8] = 0x10000u;
    Require(decode(0x326u, 0x326u).positionLocation == PixelPositionLocation::Centroid, "POS_FLOAT_LOCATION 1 was not decoded as the centroid");
    queue.context[0x1b8] = 0x20000u;
    Require(decode(0x326u, 0x326u).positionLocation == PixelPositionLocation::Sample, "POS_FLOAT_LOCATION 2 was not decoded as the iterated sample");
    queue.context[0x1b8] = 0x120000u;
    Require(decode(0x326u, 0x326u).positionLocation == PixelPositionLocation::UpperLeft, "POS_FLOAT_ULC did not override POS_FLOAT_LOCATION");
    queue.context[0x1b8] = 0x30000u;
    expectFailure([&] { static_cast<void>(decode(0x326u, 0x326u)); }, "POS_FLOAT_LOCATION uses the reserved value 3");
    Require(decode(0x1002u, 0x1002u).positionLocation == PixelPositionLocation::Center, "POS_FLOAT_LOCATION was decoded for a pixel program without positions");
    queue.context.erase(0x1b8);
    Require(decode(0x326u, 0x326u).positionLocation == PixelPositionLocation::Center, "positions without SPI_BARYC_CNTL were not at the pixel center");
    queue.context[0x1b8] = 0u;
    pixel = decode(0x506u, 0x7afu);
    Require(pixel.inputAddr == 0x7afu && !pixel.posY && pixel.posZ && PixelInputVgpr(pixel.inputAddr, PixelInput::PerspectiveCentroid) == 4u && PixelInputVgpr(pixel.inputAddr, PixelInput::PositionX) == 12u && PixelInputVgpr(pixel.inputAddr, PixelInput::PositionZ) == 14u, "ADDR-only inputs did not reserve their VGPRs");
    expectFailure([&] { static_cast<void>(decode(0xau, 0xau)); }, "unsupported SPI_PS_INPUT_ENA/ADDR");
    pixel = decode(0x4002u, 0x4002u);
    Require(pixel.inputAddr == 0x4002u && PixelInputVgpr(pixel.inputAddr, PixelInput::SampleCoverage) == 2u, "SAMPLE_COVERAGE did not take its VGPR after the perspective center");
    Require(readsBuiltin(pixelBuiltinsRead(0x4002u, 0x4002u, 2u), spv::BuiltInSampleMask), "SAMPLE_COVERAGE is not read from gl_SampleMaskIn");
    Require(sampleMaskInputs(0x4002u, 0xffffu) == 1u && sampleMaskInputs(0x4002u, 0x0003u) == 1u, "SAMPLE_COVERAGE beside a partial quad sample mask declared gl_SampleMaskIn more than once");
    pixel = decode(0x80u, 0x80u);
    Require(pixel.inputAddr == 0x80u && PixelInputVgpr(pixel.inputAddr, PixelInput::LineStipple) == 0u, "a pixel shader with only LINE_STIPPLE_TEX enabled was rejected");
    pixel = decode(0x8082u, 0x8082u);
    Require(PixelInputVgpr(pixel.inputAddr, PixelInput::LineStipple) == 2u && PixelInputVgpr(pixel.inputAddr, PixelInput::PositionFixedPoint) == 3u, "LINE_STIPPLE_TEX and POS_FIXED_PT did not take their VGPRs after the perspective center");
    Require(readsBuiltin(pixelBuiltinsRead(0x8082u, 0x8082u, 3u), spv::BuiltInFragCoord), "POS_FIXED_PT is not read from the fragment coordinate");
    Require(!readsBuiltin(pixelBuiltinsRead(0x82u, 0x82u, 2u), spv::BuiltInFragCoord), "LINE_STIPPLE_TEX read the fragment coordinate");
    pixel = decode(0x546u, 0x7c7u);
    {
        ShaderRecompiler::RecompileRequest request{};
        const std::array<std::uint32_t, 1> code{0xbf810000u};
        request.shader = {ShaderRecompiler::ShaderStage::Fragment, 0x30000u, code, 0, {}};
        request.context.waveSize = 64;
        request.context.pixel = pixel;
        const ShaderRecompiler::RequestSerializer serializer;
        const auto back = serializer.Deserialize(serializer.Serialize(request));
        const auto& p = *back.request.context.pixel;
        Require(p.inputAddr == 0x7c7u && p.hasPerspectiveCenterVgpr && p.perspectiveCentroid && p.linearCentroid && !p.noPerspective && p.posX && !p.posY && p.posZ, "the pixel input layout did not survive serialization");
    }
    for (const auto source : {0u, 2u, 3u}) {
        const auto read = pixelBuiltinsRead(0x106u, 0x106u, source);
        Require(readsBuiltin(read, spv::BuiltInBaryCoordKHR) && !readsBuiltin(read, spv::BuiltInFragCoord), "a centroid-layout I/J VGPR does not hold the barycentrics: v" + std::to_string(source));
    }
    const auto noPerspective = pixelNoPerspectiveLocations(false);
    Require(noPerspective.size() == 1 && noPerspective[0] == 1u, "only the parameter interpolated through the linear pair must be NoPerspective");
    Require(pixelNoPerspectiveLocations(true).empty(), "explicit interpolation must not interpolate parameter arrays a second time");
    for (const auto inputs : {0x11u, 0x12u, 0x21u}) {
        const auto sampleNoPerspective = pixelNoPerspectiveLocations(false, inputs);
        Require(sampleNoPerspective.size() == 1 && sampleNoPerspective[0] == 1u, "a parameter interpolated through a sample I/J pair lost its interpolation mode: inputs " + std::to_string(inputs));
        Require(pixelNoPerspectiveLocations(true, inputs).empty(), "explicit interpolation through a sample I/J pair interpolated parameter arrays a second time: inputs " + std::to_string(inputs));
    }
    for (const auto source : {0u, 1u}) {
        const auto read = pixelBuiltinsRead(0x3u, 0x3u, source);
        Require(readsBuiltin(read, spv::BuiltInBaryCoordKHR) && !readsBuiltin(read, spv::BuiltInFragCoord), "PERSP_SAMPLE does not hold the perspective barycentrics: v" + std::to_string(source));
    }
    for (const auto source : {2u, 3u}) {
        const auto read = pixelBuiltinsRead(0x12u, 0x12u, source);
        Require(readsBuiltin(read, spv::BuiltInBaryCoordNoPerspKHR) && !readsBuiltin(read, spv::BuiltInBaryCoordKHR), "LINEAR_SAMPLE does not hold the linear barycentrics: v" + std::to_string(source));
    }
    auto read = pixelBuiltinsRead(0x106u, 0x106u, 4u);
    Require(readsBuiltin(read, spv::BuiltInFragCoord) && !readsBuiltin(read, spv::BuiltInBaryCoordKHR), "POS_X is not in v4 after the center and centroid pairs");
    read = pixelBuiltinsRead(0x326u, 0x326u, 5u);
    Require(readsBuiltin(read, spv::BuiltInBaryCoordNoPerspKHR) && !readsBuiltin(read, spv::BuiltInFragCoord), "LINEAR_CENTER's J is not v5");
    read = pixelBuiltinsRead(0x326u, 0x326u, 6u);
    Require(readsBuiltin(read, spv::BuiltInFragCoord) && !readsBuiltin(read, spv::BuiltInBaryCoordKHR) && !readsBuiltin(read, spv::BuiltInBaryCoordNoPerspKHR), "POS_X is not in v6 after three I/J pairs");
    read = pixelBuiltinsRead(0x102u, 0x106u, 4u);
    Require(readsBuiltin(read, spv::BuiltInFragCoord), "an ADDR-only centroid pair did not reserve v2/v3");
    read = pixelBuiltinsRead(0x102u, 0x106u, 2u);
    Require(!readsBuiltin(read, spv::BuiltInFragCoord) && !readsBuiltin(read, spv::BuiltInBaryCoordKHR), "an ADDR-only centroid pair was loaded");
}

void BlendBypassTests() {
    auto queue = makeState();
    for (std::uint32_t constant = 0x105; constant < 0x109; ++constant) queue.context[constant] = 0;
    queue.context[0x1e0] = (1u << 30u) | 4u | (5u << 8u);
    Require(AgcDriver::Graphics::DecodeState(queue).blends.at(0).blendEnable, "enabled blending was not decoded");
    queue.context[0x31c] |= 0x10000u;
    const auto bypassed = AgcDriver::Graphics::DecodeState(queue);
    Require(!bypassed.blends.at(0).blendEnable && bypassed.blends.at(0).colorWriteMask != 0, "a target with BLEND_BYPASS still blends");
}

void UnclampedNormalizedColorTests() {
    using AgcDriver::Graphics::DecodeState;
    constexpr std::uint32_t blending = (1u << 30u) | 4u | (5u << 8u);
    const auto target = [](std::uint32_t info) {
        auto queue = makeState();
        for (std::uint32_t constant = 0x105; constant < 0x109; ++constant) queue.context[constant] = 0;
        queue.context[0x31c] = info;
        queue.context[0x1b3] = 2;
        queue.context[0x1b4] = 2;
        return queue;
    };
    auto queue = target(0x0000000cu);
    auto state = DecodeState(queue);
    Require(state.colors.size() == 1 && state.colors[0].format == VK_FORMAT_R8G8_UNORM && !state.blends.at(0).blendEnable && state.blends.at(0).colorWriteMask != 0, "an 8_8 UNORM target with BLEND_CLAMP clear (CB_COLOR_INFO 0x0000000c, PPSA26344) was refused without blending");
    const auto rejection = AgcDriver::Graphics::DrawRejection(queue, false);
    Require(rejection.empty(), "the precheck refused an unclamped UNORM target that does not blend: " + rejection);
    Require(AgcDriver::Graphics::DecodeColorBuffer(queue.context, 0).format == VK_FORMAT_R8G8_UNORM, "an unclamped UNORM target was refused outside a draw");
    auto eliminate = target(0x0000000cu);
    eliminate.context[0x0] = 0;
    eliminate.context[0x202] = 0xcc0020;
    const auto pass = AgcDriver::Graphics::DecodeColorMetadataPass(eliminate);
    Require(pass.has_value() && pass->mode == AgcDriver::Graphics::ColorMetadataPass::Mode::EliminateFastClear && pass->targets.size() == 1 && pass->targets[0].format == VK_FORMAT_R8G8_UNORM, "a fast-clear eliminate over an unclamped UNORM target was refused");
    queue.context[0x1e0] = blending;
    expectFailure([&] { DecodeState(queue); }, "blending unclamped normalized color is not implemented (CB_COLOR_INFO 0x0000000c, CB_BLEND0_CONTROL 0x40000504)");
    queue.context[0x31c] |= 0x10000u;
    state = DecodeState(queue);
    Require(!state.blends.at(0).blendEnable && state.blends.at(0).colorWriteMask != 0, "an unclamped UNORM target with BLEND_BYPASS was refused or blended");
    queue = target(0x0000010cu);
    Require(DecodeState(queue).colors.at(0).format == VK_FORMAT_R8G8_SNORM, "an unclamped SNORM target that does not blend was refused");
    queue.context[0x1e0] = blending;
    expectFailure([&] { DecodeState(queue); }, "blending unclamped normalized color is not implemented");
    queue = target(0x00000604u);
    expectFailure([&] { DecodeState(queue); }, "unclamped normalized color is unsupported");
    queue = target(0x0000800cu);
    queue.context[0x1e0] = blending;
    Require(DecodeState(queue).blends.at(0).blendEnable, "a clamped UNORM target stopped blending");
    queue = target(0x00000708u);
    queue.context[0x1e0] = blending;
    state = DecodeState(queue);
    Require(state.colors.at(0).format == VK_FORMAT_R16_SFLOAT && state.blends.at(0).blendEnable, "a 16 float target, which keeps BLEND_CLAMP clear, stopped blending");
}

void OpaqueDestinationAlphaTests() {
    auto queue = makeState();
    for (std::uint32_t constant = 0x105; constant < 0x109; ++constant) queue.context[constant] = 0;
    queue.context[0x1e0] = (1u << 30u) | (1u << 29u) | 6u | (7u << 8u) | (8u << 16u) | (10u << 24u);
    auto blend = AgcDriver::Graphics::DecodeState(queue).blend;
    Require(blend.srcColorBlendFactor == VK_BLEND_FACTOR_DST_ALPHA && blend.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, "color blend factors changed");
    Require(blend.srcAlphaBlendFactor == VK_BLEND_FACTOR_DST_COLOR && blend.dstAlphaBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA_SATURATE, "alpha blend factors changed");
    queue.context[0x31d] = 0x20000u;
    blend = AgcDriver::Graphics::DecodeState(queue).blend;
    Require(blend.srcColorBlendFactor == VK_BLEND_FACTOR_ONE && blend.dstColorBlendFactor == VK_BLEND_FACTOR_ZERO, "an opaque destination left a destination-alpha color factor");
    Require(blend.srcAlphaBlendFactor == VK_BLEND_FACTOR_ONE && blend.dstAlphaBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA_SATURATE, "an opaque destination left a destination alpha factor");
    queue.context[0x1e0] = (1u << 30u) | 10u | (9u << 8u);
    blend = AgcDriver::Graphics::DecodeState(queue).blend;
    Require(blend.srcColorBlendFactor == VK_BLEND_FACTOR_ZERO && blend.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, "alpha saturation with an opaque destination");
    Require(blend.dstAlphaBlendFactor == VK_BLEND_FACTOR_ZERO, "an opaque destination left a destination-color alpha factor");
    queue.context[0x31d] = 0x40000u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "destination alpha override");
}

void DisabledColorTests() {
    auto queue = makeState();
    queue.context[0x8e] = 0;
    for (const auto offset : {0x31cu, 0x31bu, 0x31du, 0x3b0u, 0x3b8u, 0x390u, 0x318u, 0x1e0u}) queue.context.erase(offset);
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.hasColorTarget && state.color.address == 0 && state.color.bytes == 0, "disabled color writes accessed a color surface");
    Require(state.renderExtent.width == 64 && state.renderExtent.height == 4, "attachment-free framebuffer lost screen scissor extent");
    queue.context[0xd] = 0;
    const auto emptyScreen = AgcDriver::Graphics::DecodeState(queue);
    Require(emptyScreen.renderExtent.width == 1 && emptyScreen.renderExtent.height == 1, "an empty screen scissor without targets did not get a one-pixel framebuffer");
    Require(emptyScreen.scissor.extent.width == 0 && emptyScreen.scissor.extent.height == 0, "an empty screen scissor without targets rasterized pixels");
    queue.context[0xd] = 64u;
    const auto emptyRows = AgcDriver::Graphics::DecodeState(queue);
    Require(emptyRows.renderExtent.width == 64 && emptyRows.scissor.extent.height == 0, "a screen scissor without rows rasterized pixels");
    queue = makeState();
    queue.context[0x8e] = 3;
    const auto partial = AgcDriver::Graphics::DecodeState(queue);
    Require(partial.hasColorTarget && partial.blend.colorWriteMask == 3, "partial color write mask changed");
    queue.context.erase(0x31c);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "missing register");
    queue = makeState();
    queue.context[0x31c] = 0;
    for (const auto offset : {0x31bu, 0x31du, 0x3b0u, 0x3b8u, 0x390u, 0x318u, 0x1e0u}) queue.context.erase(offset);
    const auto disabled = AgcDriver::Graphics::DecodeState(queue);
    Require(!disabled.hasColorTarget && disabled.colors.empty() && disabled.blends.empty(), "COLOR_INVALID retained an attachment despite the disabled buffer");
    Require(disabled.renderExtent.width == 64 && disabled.renderExtent.height == 4, "COLOR_INVALID lost the attachment-free render extent");
    queue.shader[0x008] = 0;
    queue.shader[0x009] = 0;
    Require(AgcDriver::Graphics::NullPixelProgramRejection(queue).empty(), "COLOR_INVALID rejected a draw without a pixel shader");
    queue.context[0x200] = 0x36;
    queue.context[0x000] = 0;
    queue.context[0x002] = 0;
    queue.context[0x010] = 0x80000181;
    queue.context[0x011] = 0x20000180;
    queue.context[0x012] = 0x100;
    queue.context[0x014] = 0x100;
    queue.context[0x007] = 0x003f003f;
    queue.context[0x00a] = 0;
    queue.context[0x00b] = std::bit_cast<std::uint32_t>(1.0f);
    const auto depthOnly = AgcDriver::Graphics::DecodeState(queue);
    Require(!depthOnly.hasColorTarget && depthOnly.depth && depthOnly.depthTest && depthOnly.depthWrite && depthOnly.renderExtent.height == 64, "COLOR_INVALID discarded a depth-only draw");
    Require(depthOnly.depth->firstSlice == 0, "a depth view of slice 0 decoded another slice");
    queue.context[0x002] = 3;
    const auto thirdSlice = AgcDriver::Graphics::DecodeState(queue);
    Require(thirdSlice.depth && thirdSlice.depth->firstSlice == 3 && thirdSlice.depth->address == depthOnly.depth->address + 3u * AgcDriver::Graphics::DepthSliceBytes(depthOnly.depth->extent, 4u), "a depth view of slice 3 lost its slice or its Z offset");
    queue.context[0x002] = 1u << 26u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "on a surface without mips");
    queue.context[0x002] = 0;
    {
        using AgcDriver::Graphics::ComputeElementMipLayout;
        using AgcDriver::Graphics::TextureTileMode;
        auto mipped = queue;
        mipped.context[0x007] = 0x03ff03ff;
        mipped.context[0x010] = 0x80000181u | (2u << 16u);
        const auto mips = ComputeElementMipLayout(TextureTileMode::kZ64KBX, 2u, 1024u, 1024u, 3u);
        Require(!mips.at(1).tail && mips.at(1).width == 512u && mips.at(1).height == 512u, "the 1024x1024 Z_X layout does not give a 512x512 second mip outside the tail");
        const auto first = AgcDriver::Graphics::DecodeState(mipped);
        Require(first.depth && first.depth->address == depthOnly.depth->address + mips.at(0).tiledOffset && first.depth->extent.width == 1024u && first.depth->extent.height == 1024u, "view mip 0 of a mip chain is not its first level");
        mipped.context[0x002] = 1u << 26u;
        const auto second = AgcDriver::Graphics::DecodeState(mipped);
        Require(second.depth && second.depth->address == depthOnly.depth->address + mips.at(1).tiledOffset, "view mip 1 is not at the second level's offset in the Z_X mip chain");
        Require(second.depth->extent.width == 512u && second.depth->extent.height == 512u, "view mip 1 does not render at the second level's extent");
        mipped.context[0x002] = (1u << 26u) | 1u | (1u << 13u);
        const auto sliced = AgcDriver::Graphics::DecodeState(mipped);
        Require(sliced.depth && sliced.depth->address == second.depth->address + AgcDriver::Graphics::ComputeSurfaceSize(mips, 1u), "slice 1 of a mip chain does not step by the whole chain");
        mipped.context[0x002] = 3u << 26u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(mipped); }, "is beyond the surface's mips");
        mipped.context[0x002] = 2u << 26u;
        mipped.context[0x007] = 0x003f003f;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(mipped); }, "into the packed mip tail is unsupported");
        mipped.context[0x007] = 0x03ff03ff;
        mipped.context[0x002] = 1u << 26u;
        mipped.context[0x010] |= 1u << 29u;
        mipped.context[0x005] = 0x300;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(mipped); }, "HTILE for the depth view mip");
        mipped.context[0x010] = (0x80000181u | (2u << 16u)) & ~(31u << 4u);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(mipped); }, "uses a swizzle other than 64 KiB Z_X");
    }
    for (const auto colorControl : {0x0u, 0xcc0000u}) {
        queue = makeState();
        queue.context[0x202] = colorControl;
        const auto unwritten = AgcDriver::Graphics::DecodeState(queue);
        Require(!unwritten.hasColorTarget && unwritten.colors.empty() && unwritten.color.address == 0, "CB_COLOR_CONTROL mode disable kept a color attachment");
        Require(unwritten.renderExtent.width == 64 && unwritten.renderExtent.height == 4, "CB_COLOR_CONTROL mode disable lost the screen scissor extent");
    }
}

void TuningFieldTests() {
    auto queue = makeState();
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    const auto baseline = AgcDriver::Graphics::DecodeState(queue);
    queue.context[0x292] = 0x22;
    auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "ALTERNATE_RBS_PER_TILE was rejected");
    Require(state.scissor.offset.x == baseline.scissor.offset.x && state.scissor.extent.width == baseline.scissor.extent.width && state.scissor.extent.height == baseline.scissor.extent.height, "ALTERNATE_RBS_PER_TILE changed the scissor");
    queue.context[0x292] = 0x26;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "scan conversion mode");
    queue.context[0x292] = 2;
    queue.context[0x202] = 0xcc0011;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "DISABLE_DUAL_QUAD was rejected");
    Require(state.hasColorTarget && state.blend.colorWriteMask == baseline.blend.colorWriteMask, "DISABLE_DUAL_QUAD changed color output");
    queue.context[0x202] = 0xcc0013;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "source or brush form");
    queue.context[0x202] = 0x5b0010;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "source or brush form");
    queue.context[0x202] = 0x5a0010;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "an XOR ROP was rejected");
    Require(state.hasColorTarget && state.colorRop == 0x5a && AgcDriver::Graphics::LogicOpFromRop(state.colorRop) == VK_LOGIC_OP_XOR, "an XOR ROP did not decode");
    queue.context[0x202] = 0xcc0010;
    Require(AgcDriver::Graphics::DecodeState(queue).colorRop == AgcDriver::Graphics::CopyRop, "the copy ROP did not decode as the copy operation");
    const std::array<std::pair<std::uint8_t, VkLogicOp>, 14> ropOperations{{
        {0x0f, VK_LOGIC_OP_COPY_INVERTED}, {0xa0, VK_LOGIC_OP_AND}, {0xf0, VK_LOGIC_OP_COPY}, {0xfa, VK_LOGIC_OP_OR},
        {0x00, VK_LOGIC_OP_CLEAR}, {0x11, VK_LOGIC_OP_NOR}, {0x33, VK_LOGIC_OP_COPY_INVERTED}, {0x55, VK_LOGIC_OP_INVERT}, {0x66, VK_LOGIC_OP_XOR},
        {0x88, VK_LOGIC_OP_AND}, {0xaa, VK_LOGIC_OP_NO_OP}, {0xcc, VK_LOGIC_OP_COPY}, {0xee, VK_LOGIC_OP_OR}, {0xff, VK_LOGIC_OP_SET}}};
    for (const auto& [rop, operation] : ropOperations) Require(AgcDriver::Graphics::LogicOpFromRop(rop) == operation, "a ROP3 code mapped to the wrong logic operation");
    std::uint32_t documentedRops = 0;
    for (std::uint32_t rop = 0; rop < 256u; ++rop) {
        try {
            static_cast<void>(AgcDriver::Graphics::LogicOpFromRop(static_cast<std::uint8_t>(rop)));
            ++documentedRops;
        } catch (const std::runtime_error&) {
        }
    }
    Require(documentedRops == 28u, "a ROP3 code outside the source and brush forms was accepted, or one inside them was refused");
    auto blendedLogic = queue;
    blendedLogic.context[0x202] = 0x5a0010;
    blendedLogic.context[0x1e0] = 0x40000101u;
    for (std::uint32_t index = 0; index < 4u; ++index) blendedLogic.context[0x105 + index] = 0u;
    blendedLogic.context[0x31c] |= 0x8000u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(blendedLogic); }, "logic operation together with blending");
    for (const auto groups : {1u, 2u, 15u}) {
        queue.context[0x2d5] = 0x2000u | (groups << 15u);
        state = AgcDriver::Graphics::DecodeState(queue);
        Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "MAX_PRIMGRP_IN_WAVE was rejected");
        Require(state.stages.path == AgcDriver::Graphics::ShaderPath::Vertex && state.stages.vertexWaveSize == 64u, "MAX_PRIMGRP_IN_WAVE changed vertex routing");
    }
}

void ShadersWriteMemoryTests() {
    const auto binding = [](ShaderRecompiler::DescriptorKind kind, ShaderRecompiler::DescriptorRole role, std::vector<bool> written) {
        ShaderRecompiler::DescriptorBinding result{};
        result.kind = kind;
        result.role = role;
        result.count = 1;
        if (role == ShaderRecompiler::DescriptorRole::GuestImages) result.imageWritten = std::move(written);
        else result.bufferWritten = std::move(written);
        return result;
    };
    ShaderRecompiler::RecompileResult reads;
    reads.bindings.push_back(binding(ShaderRecompiler::DescriptorKind::StorageBuffer, ShaderRecompiler::DescriptorRole::GuestBuffers, {false}));
    reads.bindings.push_back(binding(ShaderRecompiler::DescriptorKind::StorageImage, ShaderRecompiler::DescriptorRole::GuestImages, {false}));
    ShaderRecompiler::RecompileResult stores;
    stores.bindings.push_back(binding(ShaderRecompiler::DescriptorKind::StorageBuffer, ShaderRecompiler::DescriptorRole::GuestBuffers, {true}));
    using Stage = ShaderRecompiler::ShaderStage;
    const auto writes = [](std::initializer_list<AgcDriver::Graphics::CompiledShader> shaders) { return AgcDriver::Graphics::ShadersWriteMemory(std::vector<AgcDriver::Graphics::CompiledShader>(shaders)); };
    Require(!writes({{Stage::Vertex, &reads, 0}, {Stage::Fragment, &reads, 0}}), "shaders that only read memory were reported as writing it");
    Require(writes({{Stage::Vertex, &reads, 0}, {Stage::Fragment, &stores, 0}}), "a pixel shader that stores to a buffer was not reported");
    Require(writes({{Stage::Vertex, &stores, 0}, {Stage::Fragment, &reads, 0}}), "a vertex shader that stores to a buffer was not reported");
    Require(writes({{Stage::Geometry, &stores, 0}, {Stage::Fragment, &reads, 0}}), "a geometry shader that stores to a buffer was not reported");
}

void ReversedComponentOrderTests() {
    {
        auto queue = makeState();
        queue.context[0x31c] = (queue.context[0x31c] & ~((0x1fu << 2u) | (7u << 8u) | (3u << 11u))) | (3u << 2u) | (1u << 11u);
        const auto state = AgcDriver::Graphics::DecodeState(queue);
        Require(state.colors.size() == 1 && state.colors[0].format == VK_FORMAT_R8G8_UNORM && state.colors[0].componentMapping == 0xecu, "an 8_8 target with SWAP_ALT did not store the x and w exports in its two channels");
        queue.context[0x1e0] = 0x40010001u;
        for (const auto offset : {0x105u, 0x106u, 0x107u, 0x108u}) queue.context[offset] = 0;
        const auto blended = AgcDriver::Graphics::DecodeState(queue);
        Require(blended.blends.size() == 1 && blended.blends[0].blendEnable, "blending into an 8_8 SWAP_ALT target with one equation for color and alpha was rejected");
        Require(!AgcDriver::Graphics::SplitsAlternateAlpha(blended), "one blend equation for color and alpha split an 8_8 SWAP_ALT draw");
        queue.context[0x1e0] = 0x60000001u;
        Require(AgcDriver::Graphics::SplitsAlternateAlpha(AgcDriver::Graphics::DecodeState(queue)), "a separate alpha blend into an 8_8 SWAP_ALT target did not split its second channel");
        queue.context[0x1e0] = (1u << 30u) | 13u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "constant color factor");
        queue.context[0x1e0] = (1u << 30u) | (1u << 29u) | 1u | (1u << 16u) | (14u << 24u);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "constant color factor");
        queue.context[0x1e0] = (1u << 30u) | (2u << 5u) | 13u;
        Require(AgcDriver::Graphics::DecodeState(queue).blends[0].colorBlendOp == VK_BLEND_OP_MIN, "a constant color factor of a MIN blend into an 8_8 SWAP_ALT target was rejected");
    }
    for (const auto& [swap, mapping] : {std::pair{2u, 0x1bu}, std::pair{3u, 0x93u}}) {
        auto queue = makeState();
        queue.context[0x31c] = (queue.context[0x31c] & ~(3u << 11u)) | (swap << 11u);
        const auto state = AgcDriver::Graphics::DecodeState(queue);
        Require(state.colors.size() == 1 && state.colors[0].format == VK_FORMAT_R8G8B8A8_UNORM && state.colors[0].componentMapping == mapping, "an 8_8_8_8 target with a reversed component order did not map exports onto RGBA8");
        Require(AgcDriver::Graphics::ExportMappings(state)[0] == mapping && state.blends[0].colorWriteMask == 0xfu, "a reversed 8_8_8_8 target did not write all four channels through its export mapping");
        queue.context[0x1e0] = 0x40010001u;
        for (const auto offset : {0x105u, 0x106u, 0x107u, 0x108u}) queue.context[offset] = 0;
        const auto blended = AgcDriver::Graphics::DecodeState(queue);
        Require(blended.blends[0].blendEnable && blended.blends[0].srcColorBlendFactor == VK_BLEND_FACTOR_ONE && blended.blends[0].dstColorBlendFactor == VK_BLEND_FACTOR_ZERO && blended.blends[0].srcAlphaBlendFactor == VK_BLEND_FACTOR_ONE && blended.blends[0].dstAlphaBlendFactor == VK_BLEND_FACTOR_ZERO && blended.reversedBlend == 0u && !blended.dualSourceBlend, "a ONE + ZERO blend into a reversed 8_8_8_8 target did not stay a plain Vulkan blend");
        queue.context[0x1e0] = 0;
        queue.context[0x31c] = (queue.context[0x31c] & ~((0x1fu << 2u) | (7u << 8u))) | (12u << 2u) | (7u << 8u);
        const auto wide = AgcDriver::Graphics::DecodeState(queue);
        Require(wide.colors.size() == 1 && wide.colors[0].format == VK_FORMAT_R16G16B16A16_SFLOAT && wide.colors[0].componentMapping == mapping && wide.blends[0].colorWriteMask == 0xfu, "a 16_16_16_16 float target with a reversed component order did not map exports onto RGBA16");
    }
}

AgcDriver::QueueState reversedBlendQueue(std::uint32_t format, std::uint32_t number, std::uint32_t swap, std::uint32_t control) {
    auto queue = makeState();
    queue.context[0x31c] = (queue.context[0x31c] & ~0x1f7cu) | (format << 2u) | (number << 8u) | (swap << 11u);
    const auto address = reinterpret_cast<std::uintptr_t>(sliceMemory.data());
    queue.context[0x318] = static_cast<std::uint32_t>(address >> 8u);
    queue.context[0x390] = static_cast<std::uint32_t>(address >> 40u);
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    queue.context[0x1e0] = control;
    for (std::uint32_t i = 0; i < 4; ++i) queue.context[0x105 + i] = 0;
    return queue;
}

void ReversedComponentOrderBlendTests() {
    using AgcDriver::Graphics::DecodeState;
    struct Target {
        std::uint32_t format;
        std::uint32_t number;
        VkFormat vkFormat;
    };
    struct Case {
        std::uint32_t control;
        VkBlendFactor source;
        VkBlendFactor destination;
        VkBlendFactor sourceAlpha;
        VkBlendFactor destinationAlpha;
        VkBlendOp operation;
        std::uint32_t code;
        bool opaqueDestination;
    };
    const std::array<Target, 3> targets{{
        {10u, 0u, VK_FORMAT_R8G8B8A8_UNORM}, {12u, 7u, VK_FORMAT_R16G16B16A16_SFLOAT}, {12u, 0u, VK_FORMAT_R16G16B16A16_UNORM}
    }};
    const std::array<Case, 10> cases{{
        {0x65010504u, VK_BLEND_FACTOR_SRC1_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, VK_BLEND_FACTOR_SRC1_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, VK_BLEND_OP_ADD, 0x570001u, false},
        {0x65810584u, VK_BLEND_FACTOR_SRC1_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, VK_BLEND_FACTOR_SRC1_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, VK_BLEND_OP_REVERSE_SUBTRACT, 0x570001u, false},
        {0x40000501u, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, VK_BLEND_OP_ADD, 0x540001u, false},
        {0x61000104u, VK_BLEND_FACTOR_SRC1_COLOR, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_SRC1_ALPHA, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, 0x560001u, false},
        {0x61020302u, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, 0x020001u, false},
        {0x61010504u, VK_BLEND_FACTOR_SRC1_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR, VK_BLEND_FACTOR_SRC1_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, VK_BLEND_OP_ADD, 0x560301u, false},
        {0x60060008u, VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0u, false},
        {0x40000101u, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, 0u, false},
        {0x40000141u, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_MIN, 0u, false},
        {0x40000706u, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0u, true},
    }};
    const std::array<std::pair<std::uint32_t, const char*>, 10> refused{{
        {0x40000706u, "with a factor of the destination alpha in the color equation"},
        {0x4000000au, "with a factor of the destination alpha in the color equation"},
        {0x61000008u, "with factors the host blend cannot form from copies of the export is not implemented"},
        {0x61000501u, "with factors the host blend cannot form from copies of the export is not implemented"},
        {0x65050302u, "with factors the host blend cannot form from copies of the export is not implemented"},
        {0x4000000du, "with a constant blend factor"},
        {0x40000013u, "with a constant blend factor"},
        {0x4000000fu, "with a second-source factor is not implemented, as dual-source blending into a component-swapped color target is unsupported"},
        {0x65240504u, "with different color and alpha operations"},
        {0x40000544u, "with a MIN or MAX operation whose factors are not ONE"},
    }};
    for (const auto& [swap, mapping] : {std::pair{2u, std::uint8_t{0x1bu}}, std::pair{3u, std::uint8_t{0x93u}}}) {
        for (const auto& target : targets) {
            for (const auto& test : cases) {
                auto queue = reversedBlendQueue(target.format, target.number, swap, test.control);
                if (test.opaqueDestination) queue.context[0x31d] = 0x20000u;
                const auto state = DecodeState(queue);
                const auto code = test.code;
                char what[128];
                std::snprintf(what, sizeof(what), "CB_BLEND0_CONTROL 0x%08x into color format %u number type %u with component mapping 0x%02x", test.control, target.format, target.number, mapping);
                Require(state.colors.size() == 1 && state.colors[0].format == target.vkFormat && state.colors[0].componentMapping == mapping, std::string(what) + ": the target decoded differently");
                const auto& blend = state.blends.at(0);
                Require(blend.blendEnable && blend.srcColorBlendFactor == test.source && blend.dstColorBlendFactor == test.destination && blend.srcAlphaBlendFactor == test.sourceAlpha && blend.dstAlphaBlendFactor == test.destinationAlpha && blend.colorBlendOp == test.operation && blend.alphaBlendOp == test.operation && blend.colorWriteMask == 0xfu, std::string(what) + ": wrong Vulkan blend state");
                Require(state.reversedBlend == code && !state.dualSourceBlend, std::string(what) + ": wrong pixel shader blend code " + std::to_string(state.reversedBlend));
                Require(AgcDriver::Graphics::DecodePixelStageInfo(queue.context, AgcDriver::Graphics::ExportMappings(state)).targetExportMapping[0] == mapping, std::string(what) + ": the export mapping changed");
                const auto direct = AgcDriver::Graphics::TranslateReversedBlend(test.control, mapping, target.vkFormat, test.opaqueDestination);
                Require(direct.shaderCode == code && direct.blend.srcColorBlendFactor == blend.srcColorBlendFactor && direct.blend.dstColorBlendFactor == blend.dstColorBlendFactor && direct.blend.srcAlphaBlendFactor == blend.srcAlphaBlendFactor && direct.blend.dstAlphaBlendFactor == blend.dstAlphaBlendFactor && direct.blend.colorBlendOp == blend.colorBlendOp && direct.blend.alphaBlendOp == blend.alphaBlendOp, std::string(what) + ": TranslateReversedBlend disagrees with DecodeState");
            }
        }
        for (const auto& [control, reason] : refused) {
            const auto queue = reversedBlendQueue(10u, 0u, swap, control);
            expectFailure([&] { DecodeState(queue); }, std::string("blending into a color target with a reversed component order ") + reason);
        }
        char registers[96];
        std::snprintf(registers, sizeof(registers), "(CB_BLEND_CONTROL 0x65010504, component mapping 0x%02x, VkFormat %d)", mapping, static_cast<int>(VK_FORMAT_R8G8B8A8_SRGB));
        expectFailure([&] { DecodeState(reversedBlendQueue(10u, 6u, swap, 0x65010504u)); }, std::string("reversed component order is not implemented for sRGB targets ") + registers);
        expectFailure([&] { DecodeState(reversedBlendQueue(10u, 1u, swap, 0x65010504u)); }, "reversed component order is not implemented for signed normalized targets");
        expectFailure([&] { DecodeState(reversedBlendQueue(12u, 1u, swap, 0x65010504u)); }, "reversed component order is not implemented for signed normalized targets");
        expectFailure([&] { DecodeState(reversedBlendQueue(12u, 4u, swap, 0x65010504u)); }, "reversed component order is not implemented for integer targets");
        auto masked = reversedBlendQueue(10u, 0u, swap, 0x65010504u);
        masked.context[0x8e] = 7u;
        Require(DecodeState(masked).blends.at(0).colorWriteMask == 0xeu, "a reversed target's blend did not keep the export-order write mask of its first three exports");
        auto twoTargets = reversedBlendQueue(10u, 0u, swap, 0x65010504u);
        for (const auto offset : {0x318u, 0x31bu, 0x31cu, 0x31du}) twoTargets.context[offset + 0xfu] = twoTargets.context.at(offset);
        for (const auto offset : {0x390u, 0x3b0u, 0x3b8u}) twoTargets.context[offset + 1u] = twoTargets.context.at(offset);
        twoTargets.context[0x31c + 0xfu] &= ~(3u << 11u);
        twoTargets.context[0x1c5] = 0x99u;
        twoTargets.context[0x8e] = 0xffu;
        twoTargets.context[0x8f] = 0xffu;
        twoTargets.context[0x1e1] = 0;
        expectFailure([&] { DecodeState(twoTargets); }, "reversed component order through a second source color needs that target to be the shader's only color export");
        twoTargets.context[0x1e0] = 0x40000101u;
        const auto two = DecodeState(twoTargets);
        Require(two.colors.size() == 2 && two.reversedBlend == 0u && two.blends.at(0).srcColorBlendFactor == VK_BLEND_FACTOR_ONE && two.blends.at(0).dstColorBlendFactor == VK_BLEND_FACTOR_ONE, "a reversed target blended by Vulkan alone was refused next to a second target");
        twoTargets.context[0x31c + 0xfu] |= swap << 11u;
        twoTargets.context[0x1e0] = 0x65010504u;
        twoTargets.context[0x1e1] = 0x40000101u;
        expectFailure([&] { DecodeState(twoTargets); }, "reversed component order through a second source color needs that target to be the shader's only color export");
    }
}

void ReversedBlendRoutingTests() {
    namespace Code = ShaderRecompiler::ReversedBlendCode;
    using Term = std::pair<bool, std::uint32_t>;
    constexpr Term zero{false, 0u};
    constexpr Term one{true, 0u};
    const auto flip = [](Term term) { return Term{!term.first, term.second}; };
    const auto guest = [&](std::uint32_t factor, std::uint32_t component) -> std::optional<Term> {
        const std::uint32_t exported = 10u + component;
        const std::uint32_t stored = 20u + component;
        switch (factor) {
            case 0: return zero;
            case 1: return one;
            case 2: return Term{false, exported};
            case 3: return Term{true, exported};
            case 4: return Term{false, 10u};
            case 5: return Term{true, 10u};
            case 6: return Term{false, 20u};
            case 7: return Term{true, 20u};
            case 8: return Term{false, stored};
            case 9: return Term{true, stored};
            default: return component == 0u ? std::optional<Term>{one} : std::nullopt;
        }
    };
    const auto product = [&](Term first, Term second) {
        if (first == zero || second == zero) return std::vector<Term>{zero};
        std::vector<Term> factors;
        for (const auto term : {first, second}) {
            if (term != one) factors.push_back(term);
        }
        std::sort(factors.begin(), factors.end());
        return factors;
    };
    std::uint32_t routed = 0;
    std::uint32_t native = 0;
    for (std::uint32_t factors = 0; factors < 11u * 11u * 11u * 11u; ++factors) {
        const std::array<std::uint32_t, 4> guestFactors{factors % 11u, factors / 11u % 11u, factors / 121u % 11u, factors / 1331u};
        const auto control = guestFactors[0] | (guestFactors[1] << 8u) | (guestFactors[2] << 16u) | (guestFactors[3] << 24u) | 0x20000000u;
        std::optional<AgcDriver::Graphics::ReversedBlend> translated;
        try {
            translated = AgcDriver::Graphics::TranslateReversedBlend(control, 0x1bu, VK_FORMAT_R8G8B8A8_UNORM, false);
        } catch (const std::runtime_error& error) {
            Require(std::string(error.what()).find("reversed component order with") != std::string::npos, std::string("an unexpected reversed-order blend refusal: ") + error.what());
            continue;
        }
        const auto code = translated->shaderCode;
        Require(code == 0u || (code & Code::Enable) != 0u, "a routed reversed-order blend code lacks its enable bit");
        const auto route = [&](std::uint32_t shift, std::uint32_t component) {
            switch ((code >> (shift + Code::RouteBits * component)) & Code::RouteMask) {
                case Code::Own: return Term{false, 10u + component};
                case Code::Component0: return Term{false, 10u};
                case Code::Zero: return zero;
                default: return one;
            }
        };
        const auto host = [&](VkBlendFactor factor, std::uint32_t component) -> std::optional<Term> {
            switch (factor) {
                case VK_BLEND_FACTOR_ZERO: return zero;
                case VK_BLEND_FACTOR_ONE: return one;
                case VK_BLEND_FACTOR_SRC_COLOR: return route(Code::ColorShift, component);
                case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return flip(route(Code::ColorShift, component));
                case VK_BLEND_FACTOR_SRC_ALPHA: return route(Code::ColorShift, 3u);
                case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return flip(route(Code::ColorShift, 3u));
                case VK_BLEND_FACTOR_DST_COLOR: return Term{false, 20u + component};
                case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return Term{true, 20u + component};
                case VK_BLEND_FACTOR_DST_ALPHA: return Term{false, 23u};
                case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return Term{true, 23u};
                case VK_BLEND_FACTOR_SRC1_COLOR: return route(Code::SecondShift, component);
                case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR: return flip(route(Code::SecondShift, component));
                case VK_BLEND_FACTOR_SRC1_ALPHA: return route(Code::SecondShift, 3u);
                case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA: return flip(route(Code::SecondShift, 3u));
                default: return std::nullopt;
            }
        };
        const auto& blend = translated->blend;
        const auto secondColor = [](VkBlendFactor factor) { return factor == VK_BLEND_FACTOR_SRC1_COLOR || factor == VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR || factor == VK_BLEND_FACTOR_SRC1_ALPHA || factor == VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA; };
        Require(code != 0u || !(secondColor(blend.srcColorBlendFactor) || secondColor(blend.dstColorBlendFactor) || secondColor(blend.srcAlphaBlendFactor) || secondColor(blend.dstAlphaBlendFactor)), "a reversed-order blend without a second color reads one");
        for (std::uint32_t component = 0; component < 4; ++component) {
            const auto wantedSource = guest(guestFactors[component == 0u ? 2u : 0u], component);
            const auto wantedDestination = guest(guestFactors[component == 0u ? 3u : 1u], component);
            const auto source = host(component == 3u ? blend.srcAlphaBlendFactor : blend.srcColorBlendFactor, component);
            const auto destination = host(component == 3u ? blend.dstAlphaBlendFactor : blend.dstColorBlendFactor, component);
            Require(wantedSource && wantedDestination && source && destination, "a reversed-order blend was accepted with a factor the routing check cannot express");
            char what[160];
            std::snprintf(what, sizeof(what), "CB_BLEND_CONTROL 0x%08x, code 0x%06x, component %u", control, code, component);
            Require(product(route(Code::ColorShift, component), *source) == product(Term{false, 10u + component}, *wantedSource), std::string(what) + ": the routed source term differs from the guest's");
            Require(*destination == *wantedDestination, std::string(what) + ": the routed destination factor differs from the guest's");
        }
        ++(code == 0u ? native : routed);
    }
    Require(native == 121u && routed == 932u, "the reversed-order blend routing accepts " + std::to_string(native) + " native and " + std::to_string(routed) + " routed factor combinations, not 121 and 932");
}

void Color565TargetTests() {
    auto queue = makeState();
    const auto info = [&](std::uint32_t number, std::uint32_t swap) {
        queue.context[0x31c] = (queue.context[0x31c] & ~((0x1fu << 2u) | (7u << 8u) | (3u << 11u))) | (16u << 2u) | (number << 8u) | (swap << 11u);
    };
    info(0, 2);
    const auto reversed = AgcDriver::Graphics::DecodeState(queue);
    Require(reversed.colors.size() == 1 && reversed.colors[0].format == VK_FORMAT_R5G6B5_UNORM_PACK16 && reversed.colors[0].elementBytes == 2 && reversed.colors[0].componentMapping == 0xe4u, "a 5_6_5 target with the reversed component order did not render through R5G6B5 with X in its high bits");
    Require(reversed.blends[0].colorWriteMask == 0xfu, "a 5_6_5 target with the reversed component order did not write all its channels");
    queue.context[0x8e] = 0x1u;
    Require(AgcDriver::Graphics::DecodeState(queue).blends[0].colorWriteMask == VK_COLOR_COMPONENT_R_BIT, "a 5_6_5 target with the reversed component order writing only X did not write the R5G6B5 channel in its high bits");
    info(0, 0);
    const auto standard = AgcDriver::Graphics::DecodeState(queue);
    Require(standard.colors[0].format == VK_FORMAT_R5G6B5_UNORM_PACK16 && standard.colors[0].componentMapping == 0xc6u, "a 5_6_5 target with the standard component order did not render through R5G6B5 with X and Z exchanged");
    Require(AgcDriver::Graphics::ExportMappings(standard)[0] == 0xc6u, "a 5_6_5 target with the standard component order did not export through the X and Z exchange");
    Require(standard.blends[0].colorWriteMask == VK_COLOR_COMPONENT_B_BIT, "a 5_6_5 target with the standard component order writing only X did not write the R5G6B5 channel in its low bits");
    queue.context[0x8e] = 0xfu;
    info(0, 2);
    for (std::uint32_t i = 0; i < 4; ++i) queue.context[0x105 + i] = 0;
    queue.context[0x1e0] = 0x40000504u;
    const auto blended = AgcDriver::Graphics::DecodeState(queue);
    Require(blended.blends[0].blendEnable && blended.blends[0].srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA && blended.blends[0].dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, "alpha blending into a 5_6_5 target was not decoded");
    queue.context[0x1e0] = 0;
    for (const auto& [number, swap] : {std::pair{1u, 2u}, std::pair{4u, 0u}, std::pair{0u, 1u}, std::pair{0u, 3u}}) {
        info(number, swap);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported color format 16 number type " + std::to_string(number) + " component swap " + std::to_string(swap));
    }
}

void Color1555TargetTests() {
    auto queue = makeState();
    queue.context[0x31c] = (queue.context[0x31c] & ~((0x1fu << 2u) | (7u << 8u) | (3u << 11u))) | (17u << 2u);
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.colors.size() == 1 && state.colors[0].format == VK_FORMAT_A1R5G5B5_UNORM_PACK16 && state.colors[0].elementBytes == 2 && state.colors[0].componentMapping == 0xc6u, "a 1_5_5_5 target did not render through A1R5G5B5 with X and Z exchanged");
    Require(AgcDriver::Graphics::ExportMappings(state)[0] == 0xc6u && state.blends[0].colorWriteMask == 0xfu, "a 1_5_5_5 target did not write all four channels through its export mapping");
    queue.context[0x8e] = 0x1u;
    Require(AgcDriver::Graphics::DecodeState(queue).blends[0].colorWriteMask == VK_COLOR_COMPONENT_B_BIT, "a 1_5_5_5 target writing only X did not write the A1R5G5B5 channel in its low bits");
    queue.context[0x8e] = 0xfu;
    for (std::uint32_t i = 0; i < 4; ++i) queue.context[0x105 + i] = 0;
    queue.context[0x1e0] = 0x40000504u;
    const auto blended = AgcDriver::Graphics::DecodeState(queue);
    Require(blended.blends[0].blendEnable && blended.blends[0].srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA && blended.blends[0].dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, "alpha blending into a 1_5_5_5 target was not decoded");
    queue.context[0x1e0] = 0;
    for (const auto& [number, swap] : {std::pair{1u, 0u}, std::pair{4u, 0u}, std::pair{0u, 1u}}) {
        queue.context[0x31c] = (queue.context[0x31c] & ~((7u << 8u) | (3u << 11u))) | (number << 8u) | (swap << 11u);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "unsupported color format 17 number type " + std::to_string(number) + " component swap " + std::to_string(swap));
    }
}

void CompactedExportTests() {
    alignas(256) static std::array<std::byte, 1024> slotFourMemory{};
    const auto slotFour = reinterpret_cast<std::uintptr_t>(slotFourMemory.data());
    auto queue = makeState();
    for (const auto offset : {0x31bu, 0x31cu, 0x31du}) queue.context[offset + 4u * 0xfu] = queue.context.at(offset);
    for (const auto offset : {0x3b0u, 0x3b8u}) queue.context[offset + 4u] = queue.context.at(offset);
    queue.context[0x318 + 4u * 0xfu] = static_cast<std::uint32_t>(slotFour >> 8u);
    queue.context[0x390 + 4u] = static_cast<std::uint32_t>(slotFour >> 40u);
    queue.context[0x1e4] = 0x40010001u;
    for (std::uint32_t i = 0; i < 4; ++i) queue.context[0x105 + i] = 0;
    queue.context[0x8e] = 0x3000fu;
    queue.context[0x8f] = 0xf000fu;
    queue.context[0x1c5] = 0x44u;
    auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.colors.size() == 2 && state.blends.size() == 2, "two compacted exports did not give two attachments");
    Require(state.colors[0].slot == 0 && state.colors[0].exportIndex == 0 && state.colors[0].address == reinterpret_cast<std::uintptr_t>(colorMemory.data()), "export 0 did not reach MRT slot 0");
    Require(state.colors[1].slot == 4 && state.colors[1].exportIndex == 1 && state.colors[1].address == slotFour, "export 1 did not reach MRT slot 4, the second slot CB_SHADER_MASK enables");
    Require(!state.blends[0].blendEnable && state.blends[1].blendEnable && state.blends[1].colorWriteMask == (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT), "export 1 did not take MRT slot 4's blend control and target mask");
    Require(AgcDriver::Graphics::ExportMappings(state)[1] == state.colors[1].componentMapping, "export 1 did not take MRT slot 4's component mapping");
    auto disabledFirst = queue;
    disabledFirst.context[0x31c] = 0;
    const auto withHole = AgcDriver::Graphics::DecodeState(disabledFirst);
    Require(withHole.colors.size() == 1 && withHole.colors[0].slot == 4 && withHole.colors[0].exportIndex == 1 && withHole.blends.size() == 2 && withHole.blends[0].colorWriteMask == 0, "COLOR_INVALID shifted a later MRT export");
    queue.context[0x1c5] = 0x90009u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "color export format 0");
    queue.context[0x8e] = 0xf000fu;
    queue.context[0x8f] = 0xf0f0fu;
    queue.context[0x1c5] = 0x999u;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.colors.size() == 2 && state.blends.size() == 3, "a write-masked export between written ones changed the attachment count");
    Require(state.colors[0].exportIndex == 0 && state.colors[1].slot == 4 && state.colors[1].exportIndex == 2, "export 2 did not reach MRT slot 4, the third slot CB_SHADER_MASK enables");
    Require(state.blends[1].colorWriteMask == 0 && !state.blends[1].blendEnable && state.blends[2].blendEnable, "the write-masked export 1 was not left unused");
}

void DepthStencilTests() {
    auto queue = makeState();
    queue.context[0x000] = 0;
    queue.context[0x002] = 0;
    queue.context[0x007] = (1u << 16u) | 3u;
    queue.context[0x00a] = 7;
    queue.context[0x00b] = 0;
    queue.context[0x010] = 0x22900983;
    queue.context[0x011] = 0x20000181;
    for (const auto offset : {0x012u, 0x014u}) queue.context[offset] = 0x100;
    for (const auto offset : {0x013u, 0x015u}) queue.context[offset] = 0x200;
    queue.context[0x005] = 0x300;
    queue.context[0x01e] = 0x1;
    {
        AgcDriver::QueueState unwritten;
        unwritten.shader = queue.shader;
        unwritten.userConfig = queue.userConfig;
        queue.context[0x200] = 0x00700791;
        for (int reset = 0; reset < 2; ++reset) {
            queue.context.ForEach([&](std::uint32_t offset, std::uint32_t value) { unwritten.context[offset] = value; });
            const auto state = AgcDriver::Graphics::DecodeState(unwritten);
            Require(state.stencilTest && state.depth, "unwritten stencil registers rejected a stencil draw");
            for (const auto face : {state.stencilFront, state.stencilBack}) {
                Require(face.failOp == VK_STENCIL_OP_KEEP && face.passOp == VK_STENCIL_OP_KEEP && face.depthFailOp == VK_STENCIL_OP_KEEP && face.compareOp == VK_COMPARE_OP_ALWAYS, "unwritten stencil operations changed");
                Require(face.compareMask == 0 && face.writeMask == 0 && face.reference == 0, "unwritten stencil reference or masks changed");
            }
            Require(AgcDriver::Graphics::DrawRejection(unwritten, false).empty(), "precheck rejected unwritten stencil registers");
            unwritten.context[0x10b] = 0x00070070;
            unwritten.context[0x10c] = unwritten.context[0x10d] = 0x01ffff09;
            const auto explicitState = AgcDriver::Graphics::DecodeState(unwritten);
            for (const auto face : {explicitState.stencilFront, explicitState.stencilBack}) Require(face.passOp == VK_STENCIL_OP_INVERT && face.compareMask == 0xff && face.writeMask == 0xff && face.reference == 9, "explicit stencil state was replaced by defaults");
            unwritten.ClearContext();
            for (const auto offset : {0x10bu, 0x10cu, 0x10du}) Require(unwritten.context.at(offset) == 0, "context clear did not reset stencil state");
        }
    }
    queue.context[0x10b] = 0x00050050;
    queue.context[0x10c] = 0x01ffff01;
    queue.context[0x10d] = 0x01000001;
    queue.context[0x200] = 0x00700711;
    std::vector<AgcDriver::Graphics::RegisterRead> log;
    auto state = [&] {
        const AgcDriver::Graphics::RegisterReadLogScope logScope(&log);
        return AgcDriver::Graphics::DecodeState(queue);
    }();
    for (const auto read : log) Require(AgcDriver::Graphics::DrawKeyCovers(read), "DrawKeyRegisters lacks a depth register the decoder reads: " + std::to_string(read.offset));
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    const auto rejection = AgcDriver::Graphics::DrawRejection(queue, false);
    Require(rejection.empty(), "precheck rejected a stencil draw with a surface: " + rejection);
    Require(state.depth && state.depth->address == 0x10000 && state.depth->stencilAddress == 0x20000 && state.depth->format == VK_FORMAT_D32_SFLOAT_S8_UINT && state.depth->clearStencil == 7 && state.depth->htileAddress == 0x10000030000ull && !state.depth->htileStencil, "depth surface decode changed");
    {
        auto split = queue;
        split.context[0x014] = 0;
        for (const auto control : {0x00700711u, 0x00700715u, 0x00700713u}) {
            split.context[0x200] = control;
            const auto splitState = AgcDriver::Graphics::DecodeState(split);
            Require(splitState.depth && splitState.depth->address == 0x10000 && !splitState.depthWrite, "a depth plane the draw does not write rejected another write address");
            Require(AgcDriver::Graphics::DrawRejection(split, false).empty(), "precheck rejected a depth plane the draw does not write at another write address");
        }
        split.context[0x200] = 0x00700717u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(split); }, "depth read and written at different addresses");
        split.context[0x200] = 0x00700711u;
        for (const auto control : {0x01u, 0x20u, 0x40u, 0x60u}) {
            split.context[0x000] = control;
            expectFailure([&] { AgcDriver::Graphics::DecodeState(split); }, "depth read and written at different addresses");
        }
    }
    for (const auto clear : {0u, 1u, 2u, 3u}) {
        auto retained = queue;
        retained.context[0x8e] = retained.context[0x8f] = retained.context[0x203] = 0;
        retained.context[0x000] = clear;
        retained.context[0x200] = 0x00700777u;
        auto unbound = retained;
        unbound.shader[0x008] = unbound.shader[0x009] = 0;
        const auto expected = AgcDriver::Graphics::DecodeState(unbound);
        for (const auto format : {1u, 2u, 3u, 9u}) {
            retained.context[0x1c4] = format;
            Require(AgcDriver::Graphics::PixelProgramSkipped(retained), "a retained format ran the pixel program during a depth/stencil pass");
            const auto actual = AgcDriver::Graphics::DecodeState(retained);
            Require(actual.depth && expected.depth && actual.depth->address == expected.depth->address && actual.depth->stencilAddress == expected.depth->stencilAddress && actual.depth->format == expected.depth->format && actual.depth->clearDepth == expected.depth->clearDepth && actual.depth->clearStencil == expected.depth->clearStencil, "null substitution changed the depth/stencil surface");
            Require(actual.depthTest == expected.depthTest && actual.depthWrite == expected.depthWrite && actual.depthCompare == expected.depthCompare && actual.depthBoundsTest == expected.depthBoundsTest && actual.stencilTest == expected.stencilTest, "null substitution changed fixed-function depth/stencil tests");
            Require(std::memcmp(&actual.stencilFront, &expected.stencilFront, sizeof(VkStencilOpState)) == 0 && std::memcmp(&actual.stencilBack, &expected.stencilBack, sizeof(VkStencilOpState)) == 0, "null substitution changed stencil operations");
            Require(actual.viewport.minDepth == expected.viewport.minDepth && actual.viewport.maxDepth == expected.viewport.maxDepth && actual.renderExtent.width == expected.renderExtent.width && actual.renderExtent.height == expected.renderExtent.height, "null substitution changed depth coverage");
            Require(AgcDriver::Graphics::DrawRejection(retained, true).empty(), "a retained-format depth/stencil pass was rejected");
        }
    }
    {
        queue.context[0x011] &= ~(1u << 29u);
        const auto stencilTiled = AgcDriver::Graphics::DecodeState(queue);
        Require(stencilTiled.depth && stencilTiled.depth->htileStencil, "HTILE holds the stencil state without TILE_STENCIL_DISABLE");
        queue.context[0x011] |= 1u << 29u;
        queue.context[0x010] &= ~(1u << 29u);
        const auto untiled = AgcDriver::Graphics::DecodeState(queue);
        Require(untiled.depth && untiled.depth->htileAddress == 0, "HTILE must be ignored without TILE_SURFACE_ENABLE");
        queue.context[0x010] |= 1u << 29u;
        using AgcDriver::Graphics::HtileFillClears;
        using AgcDriver::Graphics::HtileFillCovers;
        constexpr VkImageAspectFlags both = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        Require(HtileFillClears(0x0003fff0u, false) == VK_IMAGE_ASPECT_DEPTH_BIT, "a depth-only HTILE fill with ZMask 0 clears the depth");
        Require(HtileFillClears(0x000000f0u, true) == both, "a depth and stencil fast clear (ZMask 0, SMem 0) clears both aspects");
        Require(HtileFillClears(0x000003f0u, true) == VK_IMAGE_ASPECT_DEPTH_BIT, "SMem 3 leaves the stencil uncleared");
        Require(HtileFillClears(0xfffff0ffu, true) == VK_IMAGE_ASPECT_STENCIL_BIT, "ZMask 0xf with SMem 0 clears only the stencil");
        Require(HtileFillClears(0xffffffffu, true) == 0 && HtileFillClears(0xffffffffu, false) == 0, "an expanded fill clears nothing");
        const VkExtent2D extent{16, 8};
        Require(HtileFillCovers(0x10000, extent, 0x10000, 0x8000) && HtileFillCovers(0x10000, extent, 0xfff0, 0x8010), "a fill over the whole 32 KiB metadata block covers the surface");
        Require(!HtileFillCovers(0x10000, extent, 0x10000, 0x7ffc) && !HtileFillCovers(0x10000, extent, 0x10004, 0x8000) && !HtileFillCovers(0, extent, 0, 0x10000), "a partial fill, one starting past the HTILE base, or no HTILE covers nothing");
        // Pipe-aligned SW_64KB_Z_X places a 16x16 surface's HTILE words across its metadata block (a valid word at offset
        // 260, AMD PAL), so a fill of the words a linear layout would hold covers nothing.
        Require(!HtileFillCovers(0x10000, {16, 16}, 0x10000, 16) && !HtileFillCovers(0x10000, {16, 16}, 0x10000, 264), "a fill of the linear HTILE size of a 16x16 surface misses its pipe-aligned words");
        Require(AgcDriver::Graphics::HtileMetadataBytes({1024, 512}) == 0x8000u && AgcDriver::Graphics::HtileMetadataBytes({1025, 512}) == 0x10000u && AgcDriver::Graphics::HtileMetadataBytes({3840, 2160}) == 20u * 0x8000u, "the HTILE metadata is 32 KiB per 1024x512 block");
    }
    Require(state.renderExtent.width == 4 && state.renderExtent.height == 2, "render extent ignores the depth surface");
    Require(!state.depthTest && !state.depthWrite && state.stencilTest, "depth/stencil enables changed");
    queue.context.erase(0x005);
    Require(AgcDriver::Graphics::DecodeState(queue).depth->htileAddress == 0, "an HTILE surface without DB_HTILE_DATA_BASE decoded an HTILE address");
    queue.context[0x005] = 0x300;
    queue.context[0x01e] = 0x01;
    log.clear();
    {
        const AgcDriver::Graphics::RegisterReadLogScope logScope(&log);
        state = AgcDriver::Graphics::DecodeState(queue);
    }
    for (const auto read : log) Require(AgcDriver::Graphics::DrawKeyCovers(read), "DrawKeyRegisters lacks an HTILE register the decoder reads: " + std::to_string(read.offset));
    Require(state.depth->htileAddress == (0x30000ull | (1ull << 40u)), "the HTILE address was not decoded from DB_HTILE_DATA_BASE and its high byte");
    {
        const auto view = queue.context[0x002];
        queue.context[0x002] = 2u | (2u << 13u);
        const auto slice = AgcDriver::Graphics::DecodeState(queue);
        Require(slice.depth->htileAddress == (0x30000ull | (1ull << 40u)) + 2u * AgcDriver::Graphics::HtileMetadataBytes(slice.depth->extent), "slice 2 of a depth array must use the third slice of its HTILE");
        queue.context[0x002] = view;
    }
    queue.context[0x010] &= ~0x20000000u;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.depth->htileAddress == 0, "a surface without TILE_SURFACE_ENABLE decoded an HTILE address");
    queue.context[0x010] |= 0x20000000u;
    const auto& front = state.stencilFront;
    Require(front.compareOp == VK_COMPARE_OP_ALWAYS && front.passOp == VK_STENCIL_OP_INCREMENT_AND_CLAMP && front.failOp == VK_STENCIL_OP_KEEP && front.reference == 1 && front.writeMask == 0xff, "stencil mask pass changed");
    Require(std::memcmp(&state.stencilBack, &front, sizeof(front)) == 0, "back faces without BACKFACE_ENABLE must use the front state");
    queue.context[0x10b] = 0;
    queue.context[0x10c] = 0x01ffff02;
    queue.context[0x200] = 0x00200211;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilFront.compareOp == VK_COMPARE_OP_EQUAL && state.stencilFront.reference == 2 && state.stencilFront.passOp == VK_STENCIL_OP_KEEP, "stencil content pass changed");
    queue.context[0x10b] = 0x00030030;
    queue.context[0x10c] = 0x01ffff00;
    queue.context[0x200] = 0x00700771;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilFront.passOp == VK_STENCIL_OP_REPLACE && state.stencilFront.reference == 0 && std::memcmp(&state.stencilBack, &state.stencilFront, sizeof(state.stencilFront)) == 0, "stencil clear pass changed");
    queue.context[0x200] = 0x007007f1;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilBack.compareOp == VK_COMPARE_OP_ALWAYS && state.stencilBack.passOp == VK_STENCIL_OP_KEEP && state.stencilBack.writeMask == 0 && state.stencilBack.reference == 1, "back-face stencil state changed");
    queue.context[0x10b] = 0x40;
    queue.context[0x10c] = 0x05ffff02;
    queue.context[0x200] = 0x00200211;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilFront.passOp == VK_STENCIL_OP_INVERT && state.stencilFront.failOp == VK_STENCIL_OP_KEEP && state.stencilFront.depthFailOp == VK_STENCIL_OP_KEEP && state.stencilFront.compareOp == VK_COMPARE_OP_EQUAL, "an equal test replaced by another value is not drawn as an inversion");
    Require(state.stencilFront.reference == 2 && state.stencilFront.compareMask == 0xff && state.stencilFront.writeMask == 7, "an inverted replacement does not keep the test value and write only the bits that differ from it");
    queue.context[0x10b] = 0x440;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilFront.passOp == VK_STENCIL_OP_INVERT && state.stencilFront.depthFailOp == VK_STENCIL_OP_INVERT && state.stencilFront.writeMask == 7, "a replacement on pass and depth fail is not drawn as one inversion");
    queue.context[0x10b] = 0x4;
    queue.context[0x200] = 0x00200511;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilFront.failOp == VK_STENCIL_OP_INVERT && state.stencilFront.passOp == VK_STENCIL_OP_KEEP && state.stencilFront.writeMask == 7, "a not-equal test that fails is not replaced as an inversion");
    queue.context[0x10c] = 0x40ffffff;
    queue.context[0x10b] = 0x40;
    queue.context[0x200] = 0x00200311;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilFront.passOp == VK_STENCIL_OP_INVERT && state.stencilFront.writeMask == 0xbf && state.stencilFront.reference == 0xff, "a less-or-equal test against 0xff, which passes only on 0xff, is not replaced as an inversion");
    queue.context[0x10b] = 0x4;
    queue.context[0x200] = 0x00200411;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilFront.failOp == VK_STENCIL_OP_INVERT && state.stencilFront.writeMask == 0xbf, "a greater test against 0xff, which fails only on 0xff, is not replaced as an inversion");
    queue.context[0x10c] = 0x40ffff7f;
    queue.context[0x10b] = 0x40;
    queue.context[0x200] = 0x00200311;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "stencil replacement");
    queue.context[0x10c] = 0x05ffff02;
    queue.context[0x200] = 0x00200511;
    queue.context[0x10b] = 0x40;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "stencil replacement");
    queue.context[0x200] = 0x00200111;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "stencil replacement");
    queue.context[0x200] = 0x00200211;
    queue.context[0x10c] = 0x05ff0f02;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "stencil replacement");
    queue.context[0x10c] = 0x05ffff02;
    queue.context[0x10b] = 0x41;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "beside another stencil operation");
    queue.context[0x10b] = 0x50;
    queue.context[0x200] = 0x00700711;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "stencil add/subtract");
    queue.context[0x10b] = 0;
    queue.context[0x000] = 1;
    queue.context[0x00b] = std::bit_cast<std::uint32_t>(0.25f);
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.depth && state.depthTest && state.depthWrite && state.depthCompare == VK_COMPARE_OP_ALWAYS && !state.depthBoundsTest && !state.depthBias && state.viewport.minDepth == 0.25f && state.viewport.maxDepth == 0.25f, "a DEPTH_CLEAR_ENABLE draw does not store DB_DEPTH_CLEAR everywhere it covers");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "precheck rejected a depth clear");
    queue.context[0x002] = 0x01000000;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "writable depth plane");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).find("writable depth plane") != std::string::npos, "precheck accepted read-only depth clear");
    queue.context[0x002] = 0;
    queue.context[0x00b] = 0;
    queue.context[0x000] = 0x22;
    state = AgcDriver::Graphics::DecodeState(queue);
    const auto clears = [](const VkStencilOpState& face) {
        return face.compareOp == VK_COMPARE_OP_ALWAYS && face.passOp == VK_STENCIL_OP_REPLACE && face.failOp == VK_STENCIL_OP_REPLACE && face.depthFailOp == VK_STENCIL_OP_REPLACE && face.writeMask == 0xff && face.reference == 7;
    };
    Require(state.stencilTest && clears(state.stencilFront) && clears(state.stencilBack), "a STENCIL_CLEAR_ENABLE draw does not store DB_STENCIL_CLEAR");
    queue.context[0x200] = 0;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.stencilTest && clears(state.stencilFront), "a STENCIL_CLEAR_ENABLE draw without a stencil test does not store DB_STENCIL_CLEAR");
    queue.context[0x002] = 0x02000000;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "writable stencil plane");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).find("writable stencil plane") != std::string::npos, "precheck accepted read-only stencil clear");
    queue.context[0x002] = 0;
    for (const auto offset : {0x31bu, 0x31cu, 0x31du}) queue.context[offset + 0xfu] = queue.context.at(offset);
    for (const auto offset : {0x3b0u, 0x3b8u}) queue.context[offset + 1u] = queue.context.at(offset);
    const auto secondColor = reinterpret_cast<std::uintptr_t>(sliceMemory.data());
    queue.context[0x327] = static_cast<std::uint32_t>(secondColor >> 8u);
    queue.context[0x391] = static_cast<std::uint32_t>(secondColor >> 40u);
    queue.context[0x1e1] = 0;
    queue.context[0x8e] = 0xf3;
    queue.context[0x8f] = 0xff;
    queue.context[0x1c5] = 0x99;
    queue.context[0x90] = 0x80000001;
    queue.context[0x91] = 0x00020003;
    queue.context[0x10b] = 0x00050050;
    queue.context[0x10c] = 0x05ffff02;
    queue.context[0x10d] = 0x090000ff;
    for (const auto control : {0u, 1u, 0x81u}) {
        queue.context[0x200] = control;
        state = AgcDriver::Graphics::DecodeState(queue);
        Require(state.stencilTest && clears(state.stencilFront) && clears(state.stencilBack), "stencil clear used the overridden stencil operations");
        Require(state.depth && !state.depthTest && !state.depthWrite, "stencil clear changed depth state");
        Require(state.colors.size() == 2 && state.blends.size() == 2 && state.colors[1].address == secondColor && state.blends[0].colorWriteMask == 3 && state.blends[1].colorWriteMask == 0xf, "stencil clear lost MRT color writes");
        Require(state.scissor.offset.x == 1 && state.scissor.offset.y == 0 && state.scissor.extent.width == 2 && state.scissor.extent.height == 2, "stencil clear changed scissor coverage");
        Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "precheck rejected combined color/stencil clear");
    }
    queue.context[0x002] = 0x01000000;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.depthWrite && clears(state.stencilFront), "read-only depth prevented stencil clear");
    queue.context[0x002] = 0;
    queue.context[0x000] = 0x23;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.depthWrite && state.depthCompare == VK_COMPARE_OP_ALWAYS && clears(state.stencilFront), "a combined depth and stencil clear draw does not clear both planes");
    for (const auto select : {0xc0u, 0xf00u}) {
        queue.context[0x000] = 0x23u | select;
        state = AgcDriver::Graphics::DecodeState(queue);
        Require(state.depthWrite && state.depthCompare == VK_COMPARE_OP_ALWAYS && clears(state.stencilFront), "a copy sample selection without a copy changed a clear draw");
    }
    for (const auto control : {4u, 8u, 0x84u, 0x108u}) {
        queue.context[0x000] = 0x22u | control;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "DB_RENDER_CONTROL");
    }
    queue.context[0x000] = 0x23;
    queue.context[0x011] = 0;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.depthWrite && state.depthCompare == VK_COMPARE_OP_ALWAYS && !state.stencilTest && state.colors.size() == 2, "a depth and stencil clear without a stencil plane does not clear depth alone");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "precheck rejected a stencil clear without a stencil plane");
    queue.context[0x000] = 0x22;
    queue.context[0x010] = 0;
    state = AgcDriver::Graphics::DecodeState(queue);
    Require(!state.depth && !state.depthTest && !state.depthWrite && !state.stencilTest && state.colors.size() == 2, "clear flags without depth/stencil planes changed color-only rendering");
    Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "precheck rejected clear flags without depth/stencil planes");
    queue = makeState();
    queue.context[0x010] = 0x80000180;
    queue.context[0x011] = 0x20000180;
    queue.context[0x200] = 0x776;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    const auto colorOnly = AgcDriver::Graphics::DecodeState(queue);
    for (const auto control : {1u, 2u, 3u}) {
        queue.context[0x000] = control;
        state = AgcDriver::Graphics::DecodeState(queue);
        Require(!state.depth && !state.depthTest && !state.depthWrite && !state.stencilTest, "stale clear flags enabled absent depth/stencil planes");
        Require(state.color.address == colorOnly.color.address && state.blends[0].colorWriteMask == colorOnly.blends[0].colorWriteMask && state.viewport.minDepth == colorOnly.viewport.minDepth && state.viewport.maxDepth == colorOnly.viewport.maxDepth, "stale clear flags changed color writes or viewport depth");
        Require(AgcDriver::Graphics::DrawRejection(queue, false).empty(), "precheck rejected a color-only draw with stale clear flags: " + AgcDriver::Graphics::DrawRejection(queue, false));
    }
    queue = makeState();
    queue.context[0x31b] = 1u << 26u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "mip exceeds");
    queue.context[0x31b] = 0;
    const auto plain = AgcDriver::Graphics::DecodeState(queue);
    queue.context[0x31b] = 35u << 13u;
    const auto plainRange = AgcDriver::Graphics::DecodeState(queue);
    Require(plainRange.color.address == plain.color.address && plainRange.color.bytes == plain.color.bytes && plainRange.color.arrayLastSlice == plain.color.arrayLastSlice, "a color view of slices 0 to 35 of a target with no array slices did not render into its only slice");
    queue.context[0x31b] = 1u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "ends before its first slice");
    queue.context[0x31b] = 1u << 13u;
    const auto firstSlice = AgcDriver::Graphics::DecodeState(queue);
    Require(firstSlice.color.address == reinterpret_cast<std::uintptr_t>(colorMemory.data()), "a color view of several slices did not retain its first slice");
    queue.context[0x207] = 1u << 18u;
    const auto layeredSingle = AgcDriver::Graphics::DecodeState(queue);
    Require(layeredSingle.color.address == firstSlice.color.address && layeredSingle.color.arrayAddress == firstSlice.color.address && layeredSingle.color.arrayLastSlice == 1u && layeredSingle.color.layers == 2u && layeredSingle.renderLayers == 2u, "a layered draw into slices 0 to 1 of a surface of one slice did not render them as a two-slice array");
    queue.context[0x207] = 0;
    const auto sliced = reinterpret_cast<std::uintptr_t>(sliceMemory.data());
    queue.context[0x318] = static_cast<std::uint32_t>(sliced >> 8u);
    queue.context[0x390] = static_cast<std::uint32_t>(sliced >> 40u);
    queue.context[0x31b] = 1u | (1u << 13u);
    const auto slice = AgcDriver::Graphics::DecodeState(queue);
    Require(slice.color.address == sliced + 1024u && slice.color.bytes == 1024u, "a color view of one slice did not move the target by one slice");
    queue.context[0x3b8] = 0x09000001;
    const auto arraySlice = AgcDriver::Graphics::DecodeState(queue);
    Require(arraySlice.color.address == sliced + 1024u && arraySlice.color.bytes == 1024u && arraySlice.color.depth == 1u, "a color view of one slice of a 2D array target did not move the target by one slice");
    queue.context[0x31b] = 1u << 13u;
    const auto range = AgcDriver::Graphics::DecodeState(queue);
    Require(range.color.address == sliced && range.color.bytes == 1024u, "a color view of slices 0 to 1 of a 2D array target did not render into its first slice");
    queue.context[0x207] = 1u << 18u;
    const auto layeredArray = AgcDriver::Graphics::DecodeState(queue);
    Require(layeredArray.color.address == sliced && layeredArray.color.arraySlice == 0u && layeredArray.color.layers == 2u && layeredArray.renderLayers == 2u && layeredArray.layerExports != 0u, "a layeredArray draw into slices 0 to 1 of a 2D array target did not render two layers");
    queue.context[0x207] = 0;
    queue.context[0x31b] = 2u | (2u << 13u);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "beyond the array surface");
    queue.context[0x31b] = 1u << 14u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "beyond the array surface");
    queue.context[0x3b8] = 0x0a000003;
    queue.context[0x31b] = 2u | (2u << 13u);
    const auto volume = AgcDriver::Graphics::DecodeState(queue);
    Require(volume.color.address == sliced && volume.color.depth == 4u && volume.color.depthSlice == 2u, "a color view of one 3D depth slice did not keep the surface address and select the slice");
    Require(volume.color.layers == 1 && volume.renderLayers == 1 && volume.layerExports == 0, "a color view of one 3D depth slice became layered");
    {
        auto mipmapped = queue;
        mipmapped.context[0x3b0] |= 2u << 28u;
        mipmapped.context[0x31b] = (1u << 26u) | (1u << 13u) | 1u;
        expectFailure([&] { AgcDriver::Graphics::DecodeColorBuffer(mipmapped.context, 0); }, "slot 0, 64x4x4, viewMip 1, maxMip 2, slices 1..1, CB_COLOR_INFO 0x00028028, ATTRIB2 0x200fc003, ATTRIB3 0x0a000003, VIEW 0x04002001");
    }
    queue.context[0x31b] = 4u | (4u << 13u);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "outside the 3D surface");
    queue.context[0x31b] = 2u | (1u << 13u);
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "ends before its first slice");
    queue.context[0x31b] = 1u | (3u << 13u);
    queue.context[0x207] = (1u << 18u) | (1u << 21u) | (1u << 24u);
    const auto layered = AgcDriver::Graphics::DecodeState(queue);
    Require(layered.color.depthSlice == 1u && layered.color.layers == 3u && layered.renderLayers == 3u && layered.layerExports == (1u << 18u), "a color view of three 3D depth slices did not render three layers from the vertex layer export");
    {
        auto pointSize = queue;
        pointSize.context[0x1b3] = pointSize.context[0x1b4] = 2;
        Require(AgcDriver::Graphics::DrawRejection(pointSize, false).empty(), "layered volume test state failed the draw precheck");
        pointSize.context[0x207] |= 1u << 16u;
        const auto rejection = AgcDriver::Graphics::DrawRejection(pointSize, false);
        Require(rejection.empty(), "inactive point size rejected a layered volume draw: " + rejection);
        const auto decoded = AgcDriver::Graphics::DecodeState(pointSize);
        Require(decoded.color.depthSlice == layered.color.depthSlice && decoded.color.layers == layered.color.layers && decoded.renderLayers == layered.renderLayers && decoded.layerExports == layered.layerExports, "inactive point size changed layered volume routing");
        Require(decoded.paClVsOutCntl == pointSize.context[0x207], "inactive point size discarded the layered export state");
    }
    queue.context[0x31b] = 1u | (8u << 13u);
    Require(AgcDriver::Graphics::DecodeState(queue).renderLayers == 3u, "a color view past the 3D surface was not clamped to its last slice");
    queue.context[0x207] = 0;
    queue.context[0x31b] = 0;
    queue.context[0x31c] |= 0x10000000;
    const auto volumeKeys = reinterpret_cast<std::uintptr_t>(volumeKeyMemory.data());
    queue.context[0x325] = static_cast<std::uint32_t>(volumeKeys >> 8u);
    queue.context[0x3a8] = static_cast<std::uint32_t>(volumeKeys >> 40u);
    Require(AgcDriver::Graphics::DecodeState(queue).color.dccAddress == volumeKeys, "a DCC 3D color target lost its keys");
}

void OneDimensionalColorTests() {
    auto queue = makeState();
    queue.context[0x3b8] = 0x08000000;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, "taller than one row");
    queue.context[0x3b0] = 63u << 14u;
    const auto line = AgcDriver::Graphics::DecodeState(queue);
    Require(line.color.extent.width == 64u && line.color.extent.height == 1u && line.color.address == reinterpret_cast<std::uintptr_t>(colorMemory.data()) && line.color.depth == 1u, "a 1D color target did not decode as one row");
}

void multisampledSingleClearTests() {
    static constexpr std::array<std::uint32_t, 22> kernel{0xbe800303u, 0xbe830382u, 0xf40c0401u, 0xfa000000u, 0xd7460000u, 0x04010609u, 0xd7460001u, 0x0405060au, 0x7e060208u, 0x7e0c020bu, 0x7e0e0280u, 0xd5690004u, 0x02000100u, 0x7e000205u, 0xd5690005u, 0x02000901u, 0xd7000001u, 0x02000c80u, 0xbf8cc07fu, 0xf0201f38u, 0x00040004u, 0xbf810000u};
    const std::array<std::uint32_t, 9> userData{0u, 0u, 0x0f7f0008u, 8u, 8u, 0xff00ffccu, 0u, 0u, 0u};
    const std::array<std::uint32_t, 3> group{8, 8, 1};
    const auto match = [&](std::span<const std::uint32_t> code, std::span<const std::uint32_t> words, const std::array<std::uint32_t, 3>& threads) {
        return AgcDriver::Graphics::MatchMultisampledSingleClear(code, words, threads);
    };
    const auto clear = match(kernel, userData, group);
    Require(clear.has_value() && clear->descriptor == 0x20f7f0008ull && clear->color == std::array<std::uint32_t, 4>{0xff00ffccu, 0u, 0u, 0u}, "RADV's multisampled comp-to-single clear was not recognized");
    Require(!match(kernel, userData, {64, 1, 1}), "the comp-to-single clear matched with another group size");
    Require(!match(kernel, std::span(userData).first(8), group), "the comp-to-single clear matched without its clear color");
    auto unsized = userData;
    unsized[3] = 0;
    Require(!match(kernel, unsized, group), "a comp-to-single clear with no block size was recognized");
    auto other = kernel;
    other[17] ^= 1u;
    Require(!match(other, userData, group), "a different kernel was taken for RADV's comp-to-single clear");
    Require(!match(std::span(kernel).first(21), userData, group), "a truncated kernel was taken for RADV's comp-to-single clear");
}

void htileDepthClearTests() {
    static constexpr std::array<std::uint32_t, 17> kernel{0xd7460000u, 0x04010c06u, 0x34000084u, 0xdc388000u, 0x04020000u, 0xbf8c3f70u, 0xd7710008u, 0x00120805u, 0xd7710009u, 0x00120a05u, 0xd771000au, 0x00120c05u, 0xd771000bu, 0x00120e05u, 0xdc788000u, 0x00020800u, 0xbf810000u};
    const std::array<std::uint32_t, 3> wave{64, 1, 1};
    const auto address = [&](std::span<const std::uint32_t> code, std::array<std::uint32_t, 6> userData, std::array<std::uint32_t, 3> threads) {
        return AgcDriver::Graphics::HtileDepthClearAddress(code, userData, threads);
    };
    Require(address(kernel, {0, 0, 0x0dc50000u, 0x40u, 0u, 0x00000ff0u}, wave) == 0x400dc50000ull, "RADV's HTILE depth fast clear (ZMASK to 0) was not recognized");
    Require(address(kernel, {0, 0, 0x0dc50000u, 0x40u, 0xfffff00fu, 0x00000ff0u}, wave) == 0, "an HTILE initialization to expanded (ZMASK 0xf) was taken for a clear");
    Require(address(kernel, {0, 0, 0x0dc50000u, 0x40u, 0x000003f0u, 0xfffffc0fu}, wave) == 0, "a stencil-only HTILE update that keeps ZMASK was taken for a depth clear");
    Require(address(kernel, {0, 0, 0x0dc50000u, 0x40u, 0u, 0x00000ff0u}, {32, 1, 1}) == 0, "the HTILE kernel matched with another group size");
    auto other = kernel;
    other[6] ^= 1u;
    Require(address(other, {0, 0, 0x0dc50000u, 0x40u, 0u, 0x00000ff0u}, wave) == 0, "a different kernel was taken for RADV's HTILE update");
    Require(address(std::span(kernel).first(16), {0, 0, 0x0dc50000u, 0x40u, 0u, 0x00000ff0u}, wave) == 0, "a truncated kernel was taken for RADV's HTILE update");
}

void memoryValueFillTests() {
    static constexpr std::array<std::uint32_t, 9> kernel{0xd7460000u, 0x04010c08u, 0xf4201a82u, 0xfa000000u, 0xbf8cc07fu, 0x7e02026au, 0xe0102000u, 0x80000100u, 0xbf810000u};
    const std::array<std::uint32_t, 3> wave{64, 1, 1};
    const std::array<std::uint32_t, 8> userData{0x50b00000u, 0x00040020u, 0x20000u, 0x00014204u, 0x34be64e0u, 0x00100020u, 1u, 0x0004dfacu};
    const auto match = [&](std::span<const std::uint32_t> code, std::array<std::uint32_t, 8> words, std::array<std::uint32_t, 3> threads, std::array<std::uint32_t, 3> groups) {
        return AgcDriver::Graphics::MatchMemoryValueFill(code, words, threads, groups);
    };
    const auto fill = match(kernel, userData, wave, {2048, 1, 1});
    Require(fill.has_value() && fill->address == 0x2050b00000ull && fill->bytes == 0x80000u && fill->valueAddress == 0x2034be64e0ull, "Kena's HTILE fill from a value in memory was not recognized");
    const auto partial = match(kernel, userData, wave, {1024, 1, 1});
    Require(partial.has_value() && partial->bytes == 0x40000u, "a fill of half the groups did not cover half the buffer");
    auto records = userData;
    records[2] = 0x100u;
    const auto clipped = match(kernel, records, wave, {2048, 1, 1});
    Require(clipped.has_value() && clipped->bytes == 0x400u, "a fill past NUM_RECORDS was not clipped to it");
    Require(!match(kernel, userData, {32, 1, 1}, {2048, 1, 1}), "the memory value fill matched with another group size");
    Require(!match(kernel, userData, wave, {2048, 2, 1}), "a two-dimensional dispatch was taken for a linear fill");
    auto strided = userData;
    strided[1] = 0x00080020u;
    Require(!match(kernel, strided, wave, {2048, 1, 1}), "a fill with an 8-byte stride was taken for a dword fill");
    auto indexed = userData;
    indexed[3] |= 1u << 23u;
    Require(!match(kernel, indexed, wave, {2048, 1, 1}), "a buffer with ADD_TID_ENABLE was taken for a plain fill");
    auto nowhere = userData;
    nowhere[4] = 0;
    nowhere[5] = 0;
    Require(!match(kernel, nowhere, wave, {2048, 1, 1}), "a fill from a null value pointer was recognized");
    auto empty = userData;
    empty[6] = 0;
    Require(!match(kernel, empty, wave, {2048, 1, 1}), "a fill whose value buffer has no records was taken for a fill from memory");
    auto raw = userData;
    raw[5] = 0x20u;
    raw[6] = 4u;
    Require(match(kernel, raw, wave, {2048, 1, 1}).has_value(), "a fill from a 4-byte raw value buffer was not recognized");
    raw[6] = 3u;
    Require(!match(kernel, raw, wave, {2048, 1, 1}), "a fill from a value buffer shorter than a dword was taken for a fill from memory");
    for (const std::uint32_t format : {20u, 21u, 22u}) {
        auto typed = userData;
        typed[3] = (typed[3] & ~(0x7fu << 12u)) | (format << 12u);
        Require(match(kernel, typed, wave, {2048, 1, 1}).has_value(), "a fill through a 32-bit buffer format was not recognized");
    }
    for (const std::uint32_t format : {0x27u, 0x0du, 0x4bu}) {
        auto converted = userData;
        converted[3] = (converted[3] & ~(0x7fu << 12u)) | (format << 12u);
        Require(!match(kernel, converted, wave, {2048, 1, 1}), "a fill through a converting buffer format was taken for a raw dword fill");
    }
    auto other = kernel;
    other[6] ^= 1u;
    Require(!match(other, userData, wave, {2048, 1, 1}), "a different kernel was taken for the memory value fill");
    Require(!match(std::span(kernel).first(8), userData, wave, {2048, 1, 1}), "a truncated kernel was taken for the memory value fill");
}

void depthMaintenanceTests() {
    for (const auto mode : {0x4u, 0x8u, 0x84u, 0x108u, 0xf88u, 0x1000u, 0x4000u}) {
        for (const auto clear : {0u, 1u, 2u, 3u}) {
            auto queue = makeState();
            queue.context[0x000] = mode | clear;
            queue.context[0x200] = 0;
            queue.context[0x8e] = 0;
            queue.context[0x8f] = 0;
            queue.shader.erase(0x8);
            const auto reason = AgcDriver::Graphics::DepthMaintenanceRejection(queue);
            Require(reason.find("DB_RENDER_CONTROL") != std::string::npos, "depth maintenance passed without depth tests or color writes");
            Require(AgcDriver::Graphics::DrawRejection(queue, false) == reason, "draw precheck did not reject depth maintenance first");
            expectFailure([&] { AgcDriver::Graphics::DecodeState(queue); }, reason);
        }
    }
    for (const auto control : {0u, 1u, 2u, 3u, 0x20u, 0x40u, 0x80u, 0xc0u, 0x100u, 0xf00u, 0x2000u, 0x2063u, 0x2fe3u}) {
        auto queue = makeState();
        queue.context[0x000] = control;
        Require(AgcDriver::Graphics::DepthMaintenanceRejection(queue).empty(), "ordinary depth controls were mistaken for maintenance");
    }
    for (const auto clear : {0u, 1u, 2u, 3u}) {
        auto queue = makeState();
        queue.context[0x000] = 0x10u | clear;
        queue.context[0x200] = 0;
        queue.context[0x8e] = 0;
        queue.context[0x8f] = 0;
        Require(AgcDriver::Graphics::DepthMaintenanceRejection(queue).empty(), "a resummarize draw without depth, stencil or color work was rejected");
        queue.context[0x200] = 2;
        Require(AgcDriver::Graphics::DepthMaintenanceRejection(queue).find("DB_RENDER_CONTROL") != std::string::npos, "a resummarize draw with a depth test passed");
        queue.context[0x200] = 0;
        queue.context[0x8e] = 0xf;
        queue.context[0x8f] = 0xf;
        Require(AgcDriver::Graphics::DepthMaintenanceRejection(queue).find("DB_RENDER_CONTROL") != std::string::npos, "a resummarize draw with color writes passed");
    }
    auto absent = makeState();
    absent.context.erase(0x000);
    Require(AgcDriver::Graphics::DepthMaintenanceRejection(absent).empty(), "an absent depth control produced a maintenance verdict");
    auto readOnly = makeState();
    readOnly.context[0x000] = 0x10;
    readOnly.context[0x200] = 0x007007f3;
    readOnly.context[0x010] = 0xa0000183;
    readOnly.context[0x011] = 0x20000180;
    readOnly.context[0x8e] = 0xffffffff;
    readOnly.context[0x8f] = 0;
    readOnly.shader.erase(0x8);
    readOnly.shader.erase(0x9);
    Require(AgcDriver::Graphics::DepthMaintenanceRejection(readOnly).empty(), "a resummarize draw with pass-through read-only depth and no stencil plane was rejected");
    for (const auto [offset, value] : {std::pair{0x200u, 0x007007f7u}, {0x200u, 0x007007fbu}, {0x200u, 0x007007e3u}, {0x011u, 0x20000181u}, {0x8fu, 0xfu}, {0x000u, 0x11u}, {0x010u, 0xa0000182u}}) {
        auto changed = readOnly;
        changed.context[offset] = value;
        Require(!AgcDriver::Graphics::DepthMaintenanceRejection(changed).empty(), "a resummarize draw with surface work or unsupported depth passed");
    }
    auto pixel = readOnly;
    pixel.shader[0x8] = 1;
    pixel.shader[0x9] = 0;
    pixel.context[0x1c4] = 0;
    pixel.context[0x203] = 0x40;
    Require(!AgcDriver::Graphics::DepthMaintenanceRejection(pixel).empty(), "a resummarize draw with a running pixel stage passed");
    for (const auto offset : {0x200u, 0x010u, 0x011u, 0x8eu, 0x8fu}) {
        auto missing = readOnly;
        missing.context.erase(offset);
        Require(!AgcDriver::Graphics::DepthMaintenanceRejection(missing).empty(), "a resummarize draw without a required register passed");
    }
}

// SPI_SHADER_Z_FORMAT (0x1c4) and the export enables of DB_SHADER_CONTROL (0x203): Z export needs a
// format with a depth channel (1, 2, 3 or 32_ABGR 9), the sample mask needs 32_ABGR, and the
// formats the export path does not lay out are refused. A missing 0x1c4 gives no verdict here.
void ZExportTests() {
    constexpr std::uint32_t zExportEnable = 0x1u;
    constexpr std::uint32_t maskExportEnable = 0x100u;
    const auto withExports = [](std::uint32_t format, std::uint32_t exports) {
        auto queue = makeState();
        queue.context[0x1b3] = 2;
        queue.context[0x1b4] = 2;
        queue.context[0x1c4] = format;
        queue.context[0x203] = 0x800u | exports;
        return queue;
    };
    auto unorm = queue;
    unorm.context[0x31c] = (unorm.context.at(0x31c) & ~0x1f7cu) | (4u << 2u) | (7u << 8u);
    Require(AgcDriver::Graphics::DecodeState(unorm).color.packing == ColorExportPacking::None, "a 32-bit float target got the unsigned integer export packing");
}

void unorm10_11_11TargetTests() {
    using ShaderRecompiler::ColorExportPacking;
    auto queue = makeState();
    queue.context[0x31c] = (queue.context.at(0x31c) & ~0x7cu) | (6u << 2u);
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.color.format == VK_FORMAT_R32_UINT && state.color.elementBytes == 4 && state.color.bytes == colorMemory.size() && state.color.packing == ColorExportPacking::Unorm10_11_11, "a 10_11_11 unorm target did not decode as a packed R32_UINT attachment");
    const auto packings = AgcDriver::Graphics::ExportPackings(state);
    Require(packings[0] == ColorExportPacking::Unorm10_11_11 && std::all_of(packings.begin() + 1, packings.end(), [](ColorExportPacking packing) { return packing == ColorExportPacking::None; }), "export 0 did not carry the 10_11_11 unorm packing alone");
    auto changed = queue;
    changed.context[0x31c] |= 7u << 8u;
    const auto floating = AgcDriver::Graphics::DecodeState(changed);
    Require(floating.color.format == VK_FORMAT_B10G11R11_UFLOAT_PACK32 && floating.color.packing == ColorExportPacking::None, "a 10_11_11 float target changed");
    changed = queue;
    changed.context[0x31c] |= 1u << 8u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "unsupported color format 6 number type 1");
    changed = queue;
    changed.context[0x1e0] = 0x40010001u;
    for (std::uint32_t i = 0; i < 4; ++i) changed.context[0x105 + i] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "blending into a 10_11_11 unorm color target");
    changed = queue;
    changed.context[0x8e] = 3;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "partial writes of a 10_11_11 unorm color target");
    changed.context[0x8e] = 7;
    Require(AgcDriver::Graphics::DecodeState(changed).color.packing == ColorExportPacking::Unorm10_11_11, "an RGB write of a 10_11_11 unorm target was refused");
    for (const auto format : {5u, 6u}) {
        changed = queue;
        changed.context[0x1c5] = format;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "color export format " + std::to_string(format) + " into a 10_11_11 unorm target");
    }
    changed = queue;
    changed.context[0x1c5] = 4;
    Require(AgcDriver::Graphics::DecodeState(changed).color.packing == ColorExportPacking::Unorm10_11_11, "an FP16_ABGR export into a 10_11_11 unorm target was refused");
    changed = queue;
    changed.context[0x31c] |= 0x10000000u;
    changed.context[0x325] = 0x1234;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "DCC-compressed 10_11_11 unorm color targets");
    changed = queue;
    changed.context[0x31c] |= 0x40000u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "truncating (ROUND_MODE) 10_11_11 unorm color targets");

    constexpr std::array<std::uint32_t, 3> code{0xf800180fu, 0x07060504u, 0xbf810000u};
    const auto request = packedExportRequest(queue, state, code);
    auto plain = request;
    plain.context.pixel->targetExportPacking.fill(ColorExportPacking::None);
    const ShaderRecompiler::RequestSerializer serializer;
    Require(serializer.Deserialize(serializer.Serialize(request)).request.context.pixel->targetExportPacking == request.context.pixel->targetExportPacking, "the 10_11_11 unorm packing was lost in serialization");
    std::vector<std::uint64_t> key;
    std::vector<std::uint64_t> plainKey;
    ShaderRecompiler::BuildPreparedShaderKey(request, key);
    ShaderRecompiler::BuildPreparedShaderKey(plain, plainKey);
    Require(key == plainKey, "the export packing entered the static shader ABI");
    const auto prepared = ShaderRecompiler::PrepareShader(plain);
    Require(ShaderRecompiler::MatchesPreparedShader(request, *prepared), "a packed draw did not match the registered pixel shader");
    const auto packed = ShaderRecompiler::Recompile(request);
    const auto unpacked = ShaderRecompiler::Recompile(plain);
    Require(packed.variantId == unpacked.variantId && packed.variantId == ShaderRecompiler::GetPreparedArtifact(*prepared).variantId, "the packed draw did not reuse the registered artifact");
    Require(packed.PipelineVariantId() != unpacked.PipelineVariantId() && !(packed.spirv == unpacked.spirv), "the export packing did not specialize the module");
    Require(ShaderRecompiler::Recompile(request).PipelineVariantId() == packed.PipelineVariantId(), "the packed specialization was not reused");
    ShaderRecompiler::RecompileResult vertex;
    vertex.spirv = makeModule({});
    const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    const auto validate = [&](const ShaderRecompiler::RecompileResult& pixel, const AgcDriver::Graphics::State& target) {
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &pixel, 0}}};
        return AgcDriver::Graphics::ValidateShaders(shaders, target, subgroup, false);
    };
    Require(validate(packed, state) == std::set<std::uint32_t>{0u}, "the packed export did not reach attachment 0");
    expectFailure([&] { validate(unpacked, state); }, "uint4 words to its packed color attachments");
    auto plainState = state;
    plainState.colors[0].packing = ColorExportPacking::None;
    expectFailure([&] { validate(packed, plainState); }, "float4 colors");
    auto targetless = plainState;
    targetless.colors.clear();
    targetless.blends.clear();
    Require(validate(packed, targetless) == std::set<std::uint32_t>{0u}, "a uint4 export from a draw with no color target was refused");
    auto gap = plainState;
    gap.blends.insert(gap.blends.begin(), gap.blends.front());
    gap.colors[0].exportIndex = 1;
    Require(validate(packed, gap) == std::set<std::uint32_t>{0u}, "a uint4 export into the unused slot below a color target was refused");
    auto uintExport = request;
    uintExport.context.pixel->targetOutputMode[0] = 7;
    expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(uintExport)); }, "fragment output 0 has no color export packing path");
}

void unorm4_4_4_4TargetTests() {
    using ShaderRecompiler::ColorExportPacking;
    auto queue = makeState();
    queue.context[0x31c] = (queue.context.at(0x31c) & ~((0x1fu << 2u) | (7u << 8u) | (3u << 11u))) | (19u << 2u) | (2u << 11u);
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.color.format == VK_FORMAT_R16_UINT && state.color.elementBytes == 2 && state.color.componentMapping == 0x1bu && state.color.packing == ColorExportPacking::Unorm4_4_4_4, "a 4_4_4_4 unorm target did not decode to an R16_UINT attachment with reversed exports");
    const auto packings = AgcDriver::Graphics::ExportPackings(state);
    Require(packings[0] == ColorExportPacking::Unorm4_4_4_4 && std::all_of(packings.begin() + 1, packings.end(), [](ColorExportPacking packing) { return packing == ColorExportPacking::None; }), "export 0 did not carry the 4_4_4_4 unorm packing");
    Require(state.color.partialWriteMask == 0u && state.blends[0].colorWriteMask == 0xfu && AgcDriver::Graphics::ExportTargetMerge(state).mask == 0u, "a full write of a 4_4_4_4 unorm target merged into the target");
    auto changed = queue;
    for (const auto& [targetMask, wordMask] : {std::pair{1u, 0xf000u}, std::pair{7u, 0xfff0u}, std::pair{8u, 0x000fu}, std::pair{0xau, 0x0f0fu}}) {
        changed.context[0x8e] = targetMask;
        Require(AgcDriver::Graphics::DrawRejection(changed, false).empty(), "the draw precheck refused CB_TARGET_MASK " + std::to_string(targetMask) + " of a 4_4_4_4 unorm target");
        const auto partial = AgcDriver::Graphics::DecodeState(changed);
        Require(partial.color.partialWriteMask == wordMask && partial.colors.front().partialWriteMask == wordMask && partial.blends[0].colorWriteMask == 0u && partial.blend.colorWriteMask == 0u, "CB_TARGET_MASK " + std::to_string(targetMask) + " of a 4_4_4_4 unorm target did not merge its nibbles behind a closed write mask");
        const auto merge = AgcDriver::Graphics::ExportTargetMerge(partial);
        const auto expected = AgcDriver::Graphics::SurfaceForTarget(partial.color);
        const auto surface = AgcDriver::Graphics::DecodeTextureResource(merge.descriptor);
        Require(merge.target == 0u && merge.mask == wordMask && merge.samples == 1u && !merge.depthStencilWrites && surface.baseAddress == expected.baseAddress && surface.pipeBankXor == expected.pipeBankXor && surface.width == expected.width && surface.height == expected.height && surface.format == expected.format && surface.tileMode == expected.tileMode && surface.dimension == AgcDriver::Graphics::TextureDimension::k2D && surface.mipCount == 1u && surface.samples == 1u, "the merge descriptor of CB_TARGET_MASK " + std::to_string(targetMask) + " does not decode to the target's surface");
    }
    {
        auto depthQueue = queue;
        depthQueue.context[0x8e] = 1;
        depthQueue.context[0x000] = 0;
        depthQueue.context[0x002] = 0;
        depthQueue.context[0x007] = (1u << 16u) | 3u;
        depthQueue.context[0x00a] = 7;
        depthQueue.context[0x00b] = 0;
        depthQueue.context[0x010] = 0x22900983;
        depthQueue.context[0x011] = 0x20000181;
        for (const auto offset : {0x012u, 0x014u}) depthQueue.context[offset] = 0x100;
        for (const auto offset : {0x013u, 0x015u}) depthQueue.context[offset] = 0x200;
        depthQueue.context[0x005] = 0x300;
        depthQueue.context[0x01e] = 0x1;
        depthQueue.context[0x008] = std::bit_cast<std::uint32_t>(0.25f);
        depthQueue.context[0x009] = std::bit_cast<std::uint32_t>(0.75f);
        struct DepthStencilCase {
            std::uint32_t control;
            std::uint32_t ops;
            std::uint32_t masks;
            bool writes;
        };
        for (const auto& [control, ops, masks, writes] : {DepthStencilCase{0x00700710u, 0x00050050u, 0x01ffff01u, false}, DepthStencilCase{0x00700712u, 0x00050050u, 0x01ffff01u, false}, DepthStencilCase{0x00700714u, 0x00050050u, 0x01ffff01u, false}, DepthStencilCase{0x00700716u, 0x00050050u, 0x01ffff01u, true}, DepthStencilCase{0x00700711u, 0x00050050u, 0x01ffff01u, true}, DepthStencilCase{0x00700711u, 0u, 0x01ffff01u, false}, DepthStencilCase{0x00700711u, 0x00050050u, 0x0100ff01u, false}, DepthStencilCase{0x00700713u, 0u, 0x01ffff01u, false}, DepthStencilCase{0x00700718u, 0x00050050u, 0x01ffff01u, false}}) {
            depthQueue.context[0x200] = control;
            depthQueue.context[0x10b] = ops;
            depthQueue.context[0x10c] = depthQueue.context[0x10d] = masks;
            const auto what = "DB_DEPTH_CONTROL " + std::to_string(control) + " with stencil operations " + std::to_string(ops) + " and masks " + std::to_string(masks);
            Require(AgcDriver::Graphics::DrawRejection(depthQueue, false).empty(), "the draw precheck refused a partial 4_4_4_4 write under " + what);
            const auto partial = AgcDriver::Graphics::DecodeState(depthQueue);
            const auto merge = AgcDriver::Graphics::ExportTargetMerge(partial);
            Require(partial.depth.has_value() == ((control & 0xbu) != 0u) && partial.depthBoundsTest == ((control & 8u) != 0u) && merge.mask == 0xf000u && AgcDriver::Graphics::DepthStencilWrites(partial) == writes && merge.depthStencilWrites == writes && merge.depthStencilTests == ((control & 0xbu) != 0u), "a partial 4_4_4_4 write under " + what + (writes ? " did not report" : " reported") + " depth or stencil writes");
        }
        auto truncated = depthQueue;
        truncated.context[0x200] = 0x00700712u;
        truncated.context[0x010] = 0x22900987u;
        truncated.context[0x2f8] = 0x00100001u;
        truncated.context[0x292] = 3;
        for (std::uint32_t pixel = 0; pixel < 4; ++pixel) truncated.context[0x2fe + pixel * 4u] = 0xcc44;
        truncated.context[0x31d] = truncated.context.at(0x31d) & ~0x3f000u;
        truncated.context[0x8e] = 0xf;
        const auto full = AgcDriver::Graphics::DecodeState(truncated);
        Require(full.samples == 2 && full.truncatedCoverage && !full.mergedCoverage && full.color.samples == 1 && full.color.partialWriteMask == 0u, "a full write of a one-sample 4_4_4_4 unorm target beside a 2x depth target did not decode as truncated coverage");
        truncated.context[0x8e] = 1;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(truncated); }, "under merged or truncated coverage");
    }
    {
        auto smoothed = queue;
        smoothed.context[0x2f8] = 0x0020c002u;
        smoothed.context[0x292] = 3;
        smoothed.context[0x3b8] = 0x0dc00000u;
        for (std::uint32_t pixel = 0; pixel < 4; ++pixel) smoothed.context[0x2fe + pixel * 4u] = 0xcc44;
        smoothed.context[0x31d] = smoothed.context.at(0x31d) & ~0x3f000u;
        smoothed.context[0x201] = 0x02130000u;
        const auto merged = AgcDriver::Graphics::DecodeState(smoothed);
        Require(merged.samples == 4 && merged.mergedCoverage && merged.color.samples == 1 && merged.color.partialWriteMask == 0u, "a full write of a one-sample 4_4_4_4 unorm target under OVERRASTERIZATION_AMOUNT 2 did not decode as merged coverage");
        smoothed.context[0x8e] = 1;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(smoothed); }, "under merged or truncated coverage");
    }
    {
        changed.context[0x8e] = 1;
        const auto partial = AgcDriver::Graphics::DecodeState(changed);
        auto killQueue = changed;
        killQueue.context[0x203] = 0x40u;
        const auto killState = AgcDriver::Graphics::DecodeState(killQueue);
        killQueue.context[0x203] = 0x10u;
        const auto earlyState = AgcDriver::Graphics::DecodeState(killQueue);
        Require(killState.pixelKill && !killState.pixelEarlyZ && !earlyState.pixelKill && earlyState.pixelEarlyZ, "DB_SHADER_CONTROL.KILL_ENABLE of a 4_4_4_4 merge did not decode as a kill under late tests");
        constexpr std::array<std::uint32_t, 3> mergeCode{0xf800180fu, 0x07060504u, 0xbf810000u};
        auto merging = packedExportRequest(changed, partial, mergeCode);
        merging.context.pixel->targetMerge = AgcDriver::Graphics::ExportTargetMerge(partial);
        const std::array<std::uint32_t, 3> capabilities{spv::CapabilityShader, spv::CapabilityFragmentShaderPixelInterlockEXT, spv::CapabilityStorageImageReadWithoutFormat};
        merging.target.supportedCapabilities = capabilities;
        merging.target.registrationWordLimit = 1u << 20u;
        const ShaderRecompiler::RequestSerializer mergeSerializer;
        Require(mergeSerializer.Deserialize(mergeSerializer.Serialize(merging)).request.context.pixel->targetMerge == merging.context.pixel->targetMerge, "the 4_4_4_4 target merge was lost in serialization");
        auto unmerged = merging;
        unmerged.context.pixel->targetMerge = {};
        auto moved = merging;
        moved.context.pixel->targetMerge.descriptor[0] += 0x100u;
        std::vector<std::uint64_t> mergedKey, unmergedKey, movedKey;
        ShaderRecompiler::RecompileCacheKey::BuildInterface(merging, mergedKey);
        ShaderRecompiler::RecompileCacheKey::BuildInterface(unmerged, unmergedKey);
        ShaderRecompiler::RecompileCacheKey::BuildInterface(moved, movedKey);
        Require(mergedKey != unmergedKey && mergedKey == movedKey, "the static shader key does not separate merging from plain writes, or depends on the merged target's address");
        for (const bool registration : {false, true}) {
            merging.target.registrationModules = registration;
            const auto merged = ShaderRecompiler::Recompile(merging);
            const auto words = merged.spirv.Copy();
            std::size_t reads = 0, writes = 0;
            for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
                reads += static_cast<spv::Op>(words[at] & 0xffffu) == spv::OpImageRead ? 1u : 0u;
                writes += static_cast<spv::Op>(words[at] & 0xffffu) == spv::OpImageWrite ? 1u : 0u;
            }
            const auto what = std::string(registration ? "a registration" : "a specialized") + " 4_4_4_4 merge module";
            Require(merged.registrationModule == registration && reads == 1u && writes == 1u && interlockShape(words) == "capability extension ordered begin end " && declaresEarlyFragmentTests(merging), what + " did not read and write the target once inside a pixel interlock after early tests: " + interlockShape(words));
            Require(coherentImageRead(words), what + " does not read the target through a Coherent image variable");
            const auto bound = std::ranges::any_of(merged.bindings, [&](const ShaderRecompiler::DescriptorBinding& binding) {
                if (binding.role != ShaderRecompiler::DescriptorRole::GuestImages || binding.kind != ShaderRecompiler::DescriptorKind::StorageImage) return false;
                for (std::size_t offset = 0; offset + 8u <= binding.guestDescriptor.size(); offset += 8u) {
                    if (std::equal(merging.context.pixel->targetMerge.descriptor.begin(), merging.context.pixel->targetMerge.descriptor.end(), binding.guestDescriptor.begin() + static_cast<std::ptrdiff_t>(offset))) return true;
                }
                return false;
            });
            Require(bound, what + " does not bind the merged target as a storage image");
            ShaderRecompiler::RecompileResult vertex;
            vertex.spirv = makeModule({});
            const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
            const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &merged, 0}}};
            Require(AgcDriver::Graphics::ValidateShaders(shaders, partial, subgroup, false) == std::set<std::uint32_t>{0u}, what + " lost its export to attachment 0");
        }
        auto lacking = merging;
        const std::array<std::uint32_t, 2> noInterlock{spv::CapabilityShader, spv::CapabilityStorageImageReadWithoutFormat};
        lacking.target.supportedCapabilities = noInterlock;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(lacking)); }, "fragmentShaderPixelInterlock");
        auto unformatted = merging;
        const std::array<std::uint32_t, 2> noFormatlessReads{spv::CapabilityShader, spv::CapabilityFragmentShaderPixelInterlockEXT};
        unformatted.target.supportedCapabilities = noFormatlessReads;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(unformatted)); }, "storage image reads without a format");
        auto killing = merging;
        killing.context.pixel->pixelKillEnable = true;
        killing.context.pixel->earlyZ = false;
        for (const bool registration : {false, true}) {
            killing.target.registrationModules = registration;
            const auto words = ShaderRecompiler::Recompile(killing).spirv.Copy();
            std::size_t reads = 0, writes = 0;
            for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
                reads += static_cast<spv::Op>(words[at] & 0xffffu) == spv::OpImageRead ? 1u : 0u;
                writes += static_cast<spv::Op>(words[at] & 0xffffu) == spv::OpImageWrite ? 1u : 0u;
            }
            Require(reads == 1u && writes == 1u && interlockShape(words) == "capability extension ordered begin end " && declaresEarlyFragmentTests(killing), "a killing 4_4_4_4 merge module under late tests of a draw without depth or stencil writes did not merge inside a pixel interlock after early tests");
        }
        killing.context.pixel->targetMerge.depthStencilWrites = true;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(killing)); }, "kills pixels under late tests of a draw writing depth or stencil");
        auto earlyKilling = killing;
        earlyKilling.context.pixel->earlyZ = true;
        static_cast<void>(ShaderRecompiler::Recompile(earlyKilling));
        auto exporting = merging;
        exporting.context.pixel->depthExportEnable = true;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(exporting)); }, "exports depth");
        alignas(16) static std::array<std::uint32_t, 4> counted{};
        static const std::array<std::uint32_t, 4> counter{static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(counted.data())), static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(counted.data()) >> 32u) & 0xffffu, 16u, 0x31016facu};
        constexpr std::array<std::uint32_t, 6> countingCode{0x7e000281u, 0xe0c80000u, 0x80000000u, 0xf800180fu, 0x07060504u, 0xbf810000u};
        auto counting = packedExportRequest(changed, partial, countingCode);
        counting.context.userData = counter;
        counting.context.pixel->targetMerge = merging.context.pixel->targetMerge;
        counting.target = merging.target;
        counting.target.registrationModules = false;
        counting.target.bdaAbiVersion = ShaderRecompiler::BdaAbi::Version;
        static const std::array<std::uint32_t, 4> atomicCapabilities{spv::CapabilityShader, spv::CapabilityFragmentShaderPixelInterlockEXT, spv::CapabilityStorageImageReadWithoutFormat, spv::CapabilityInt64};
        counting.target.supportedCapabilities = atomicCapabilities;
        counting.context.pixel->earlyZ = false;
        static_cast<void>(ShaderRecompiler::Recompile(counting));
        counting.context.pixel->targetMerge.depthStencilTests = true;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(counting)); }, "writes guest memory (stores, atomics or GDS) under late tests");
        counting.context.pixel->earlyZ = true;
        const auto earlyWords = ShaderRecompiler::Recompile(counting).spirv.Copy();
        std::size_t atomics = 0;
        for (std::size_t at = 5; at < earlyWords.size() && (earlyWords[at] >> 16u) != 0; at += earlyWords[at] >> 16u) atomics += static_cast<spv::Op>(earlyWords[at] & 0xffffu) == spv::OpAtomicIAdd ? 1u : 0u;
        Require(atomics == 1u && interlockShape(earlyWords) == "capability extension ordered begin end " && declaresEarlyFragmentTests(counting), "a 4_4_4_4 merge whose shader adds to a counter under early tests did not keep its one atomic add beside the merge");
        constexpr std::array<std::uint32_t, 9> gdsReadCode{0xbefc03ffu, 0x00000100u, 0x7e000280u, 0xd8da0000u, 0x04000000u, 0xbf8c0000u, 0xf800180fu, 0x07060504u, 0xbf810000u};
        auto gdsReading = packedExportRequest(changed, partial, gdsReadCode);
        gdsReading.context.pixel->targetMerge = counting.context.pixel->targetMerge;
        gdsReading.context.pixel->earlyZ = false;
        gdsReading.target = counting.target;
        static_cast<void>(ShaderRecompiler::Recompile(gdsReading));
        constexpr std::array<std::uint32_t, 9> gdsAddCode{0xbefc03ffu, 0x00000100u, 0x7e000280u, 0xd8020000u, 0x00000000u, 0xbf8c0000u, 0xf800180fu, 0x07060504u, 0xbf810000u};
        auto gdsAdding = gdsReading;
        gdsAdding.shader.code = gdsAddCode;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(gdsAdding)); }, "writes guest memory (stores, atomics or GDS) under late tests");
        auto lateMerge = counting;
        lateMerge.context.pixel->earlyZ = false;
        lateMerge.context.pixel->targetMerge = {};
        static_cast<void>(ShaderRecompiler::Recompile(lateMerge));
    }
    changed = queue;
    changed.context[0x8e] = 1u;
    changed.context[0x31d] = (changed.context.at(0x31d) & ~0x3f000u) | (1u << 12u) | (1u << 15u);
    {
        const auto multisampled = AgcDriver::Graphics::DecodeState(changed);
        Require(multisampled.samples == 2u && multisampled.color.samples == 2u && multisampled.color.partialWriteMask == 0xf000u && multisampled.blends[0].colorWriteMask == 0u && multisampled.perSampleShading, "a partial write of a 2-sample 4_4_4_4 unorm target did not merge per sample behind a closed write mask");
        auto coverage = changed;
        coverage.context[0x1b3] = 0x4002u;
        coverage.context[0x1b4] = 0x4002u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(coverage); }, "SAMPLE_COVERAGE in a multisampled pixel shader that runs per sample");
        coverage.context[0x8e] = 0xfu;
        Require(!AgcDriver::Graphics::DecodeState(coverage).perSampleShading, "a full write of a 2-sample 4_4_4_4 unorm target with a SAMPLE_COVERAGE input was refused or ran per sample");
        struct InputCase {
            std::uint32_t inputs;
            std::uint32_t position;
            const char* what;
        };
        auto rasterized = changed;
        rasterized.context[0x2f8] = 0x00100001u;
        rasterized.context[0x292] = 3;
        rasterized.context[0x3b8] = 0x0dc00000u;
        for (std::uint32_t pixel = 0; pixel < 4; ++pixel) rasterized.context[0x2fe + pixel * 4u] = 0xcc44;
        const auto inputState = [](AgcDriver::QueueState queue, std::uint32_t inputs, std::uint32_t position) {
            queue.context[0x1b3] = inputs;
            queue.context[0x1b4] = inputs;
            queue.context[0x1b8] = position;
            return queue;
        };
        for (const auto& [inputs, position, what] : {InputCase{0x0004u, 0u, "PERSP_CENTROID"}, InputCase{0x0040u, 0u, "LINEAR_CENTROID"}, InputCase{0x0001u, 0u, "PERSP_SAMPLE"}, InputCase{0x0008u, 0u, "PERSP_PULL_MODEL"}, InputCase{0x2002u, 0u, "ANCILLARY"}, InputCase{0x0402u, 0u, "POS_Z_FLOAT"}, InputCase{0x0802u, 0u, "POS_W_FLOAT"}, InputCase{0x0302u, 0x20000u, "POS_X_FLOAT and POS_Y_FLOAT at a sample"}, InputCase{0x0102u, 0x10000u, "POS_X_FLOAT at the centroid"}}) {
            auto moved = inputState(rasterized, inputs, position);
            expectFailure([&] { AgcDriver::Graphics::DecodeState(moved); }, "cannot read the ancillary word or, unless every sample sits at the pixel centre");
            moved.context[0x293] = moved.context.at(0x293) | 0x10000u;
            Require(AgcDriver::Graphics::DecodeState(moved).perSampleShading, std::string("a partial write of a 2-sample 4_4_4_4 unorm target whose shader the guest runs per sample was refused for reading ") + what);
            if (inputs == 0x2002u) {
                expectFailure([&] { AgcDriver::Graphics::DecodeState(inputState(changed, inputs, position)); }, "cannot read the ancillary word");
                continue;
            }
            Require(AgcDriver::Graphics::DecodeState(inputState(changed, inputs, position)).perSampleShading, std::string("a partial write of a 2-sample 4_4_4_4 unorm target raised from a one-sample rasterizer, every sample at the pixel centre, was refused for reading ") + what);
        }
        for (const auto& [inputs, position, what] : {InputCase{0x0302u, 0u, "POS_X_FLOAT and POS_Y_FLOAT at the centre"}, InputCase{0x0302u, 0x100000u, "POS_X_FLOAT and POS_Y_FLOAT at the upper-left corner"}, InputCase{0x9022u, 0u, "PERSP_CENTER, LINEAR_CENTER, FRONT_FACE and POS_FIXED_PT"}}) {
            Require(AgcDriver::Graphics::DecodeState(inputState(rasterized, inputs, position)).perSampleShading, std::string("a partial write of a 2-sample 4_4_4_4 unorm target whose pixel-rate shader reads ") + what + " was refused");
        }
        const auto merge = AgcDriver::Graphics::ExportTargetMerge(multisampled);
        const auto surface = AgcDriver::Graphics::DecodeTextureResource(merge.descriptor);
        Require(merge.samples == 2u && merge.mask == 0xf000u && surface.dimension == AgcDriver::Graphics::TextureDimension::k2DMsaa && surface.samples == 2u && surface.baseAddress == multisampled.color.surfaceAddress && surface.width == multisampled.color.extent.width && surface.height == multisampled.color.extent.height && surface.format == AgcDriver::Graphics::TargetMergeSurface(multisampled.color).format, "the merge descriptor of a 2-sample 4_4_4_4 unorm target does not decode to its multisampled surface");
        auto unshaded = multisampled;
        unshaded.perSampleShading = false;
        expectFailure([&] { static_cast<void>(AgcDriver::Graphics::ExportTargetMerge(unshaded)); }, "needs per-sample shading");
        constexpr std::array<std::uint32_t, 3> mergeCode{0xf800180fu, 0x07060504u, 0xbf810000u};
        auto merging = packedExportRequest(changed, multisampled, mergeCode);
        merging.context.pixel->targetMerge = merge;
        const std::array<std::uint32_t, 5> capabilities{spv::CapabilityShader, spv::CapabilityFragmentShaderPixelInterlockEXT, spv::CapabilityStorageImageReadWithoutFormat, spv::CapabilityStorageImageMultisample, spv::CapabilityInterpolationFunction};
        merging.target.supportedCapabilities = capabilities;
        merging.target.fragmentShaderBarycentricEnabled = true;
        const auto words = ShaderRecompiler::Recompile(merging).spirv.Copy();
        std::size_t sampled = 0;
        bool sampleRate = false;
        for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
            const auto op = static_cast<spv::Op>(words[at] & 0xffffu);
            if (op == spv::OpCapability && words[at + 1] == spv::CapabilitySampleRateShading) sampleRate = true;
            if (op == spv::OpImageRead && (words[at] >> 16u) == 7u && words[at + 5] == spv::ImageOperandsSampleMask) ++sampled;
            if (op == spv::OpImageWrite && (words[at] >> 16u) == 6u && words[at + 4] == spv::ImageOperandsSampleMask) ++sampled;
        }
        Require(sampleRate && sampled == 2u && interlockShape(words) == "capability extension ordered begin end ", "a 2-sample 4_4_4_4 merge module does not read and write the shaded sample inside a pixel interlock");
        auto single = merging;
        const std::array<std::uint32_t, 4> noMultisampleStorage{spv::CapabilityShader, spv::CapabilityFragmentShaderPixelInterlockEXT, spv::CapabilityStorageImageReadWithoutFormat, spv::CapabilityInterpolationFunction};
        single.target.supportedCapabilities = noMultisampleStorage;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(single)); }, "storage image multisampling");
        auto centred = merging;
        centred.target.fragmentShaderBarycentricEnabled = false;
        centred.context.pixel->sampleLocations = AgcDriver::Graphics::PixelSampleLocations(multisampled, *centred.context.pixel);
        Require(std::ranges::all_of(centred.context.pixel->sampleLocations, [](std::uint8_t location) { return location == 0u; }), "a merge raised from a one-sample rasterizer did not keep every sample at the pixel centre");
        static_cast<void>(ShaderRecompiler::Recompile(centred));
        const auto rasterizedState = AgcDriver::Graphics::DecodeState(rasterized);
        auto offCentre = packedExportRequest(rasterized, rasterizedState, mergeCode);
        offCentre.context.pixel->targetMerge = AgcDriver::Graphics::ExportTargetMerge(rasterizedState);
        offCentre.context.pixel->iteratesSamples = rasterizedState.perSampleShading;
        offCentre.context.pixel->sampleLocations = AgcDriver::Graphics::PixelSampleLocations(rasterizedState, *offCentre.context.pixel);
        Require(std::ranges::any_of(offCentre.context.pixel->sampleLocations, [](std::uint8_t location) { return location != 0u; }), "a 2-sample rasterizer's merge did not pass its off-centre sample locations");
        offCentre.target.supportedCapabilities = capabilities;
        offCentre.target.fragmentShaderBarycentricEnabled = false;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(offCentre)); }, "its centre-located barycentrics would follow the sample");
        offCentre.target.fragmentShaderBarycentricEnabled = true;
        static_cast<void>(ShaderRecompiler::Recompile(offCentre));
        alignas(16) static std::array<std::uint32_t, 4> counted{};
        static const std::array<std::uint32_t, 4> counter{static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(counted.data())), static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(counted.data()) >> 32u) & 0xffffu, 16u, 0x31016facu};
        constexpr std::array<std::uint32_t, 6> countingCode{0x7e000281u, 0xe0c80000u, 0x80000000u, 0xf800180fu, 0x07060504u, 0xbf810000u};
        auto counting = packedExportRequest(changed, multisampled, countingCode);
        counting.context.userData = counter;
        counting.context.pixel->targetMerge = merge;
        counting.context.pixel->iteratesSamples = true;
        counting.context.pixel->earlyZ = true;
        counting.target = merging.target;
        counting.target.bdaAbiVersion = ShaderRecompiler::BdaAbi::Version;
        static const std::array<std::uint32_t, 6> atomicCapabilities{spv::CapabilityShader, spv::CapabilityFragmentShaderPixelInterlockEXT, spv::CapabilityStorageImageReadWithoutFormat, spv::CapabilityStorageImageMultisample, spv::CapabilityInterpolationFunction, spv::CapabilityInt64};
        counting.target.supportedCapabilities = atomicCapabilities;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(counting)); }, "each covered sample would repeat its writes");
        auto unmergedCounting = counting;
        unmergedCounting.context.pixel->targetMerge = {};
        static_cast<void>(ShaderRecompiler::Recompile(unmergedCounting));
    }
    for (const auto& [number, swap] : {std::pair{1u, 2u}, std::pair{4u, 2u}, std::pair{0u, 0u}, std::pair{0u, 1u}, std::pair{0u, 3u}}) {
        changed = queue;
        changed.context[0x31c] = (changed.context[0x31c] & ~((7u << 8u) | (3u << 11u))) | (number << 8u) | (swap << 11u);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "unsupported color format 19 number type " + std::to_string(number) + " component swap " + std::to_string(swap));
    }
    changed = queue;
    changed.context[0x31c] |= 0x10000000u;
    changed.context[0x325] = 0x1234;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "DCC-compressed 4_4_4_4 unorm color targets");
    changed = queue;
    changed.context[0x31c] |= 0x40000u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "truncating (ROUND_MODE) 4_4_4_4 unorm color targets");

    constexpr std::array<std::uint32_t, 3> code{0xf800180fu, 0x07060504u, 0xbf810000u};
    const auto request = packedExportRequest(queue, state, code);
    auto plain = request;
    plain.context.pixel->targetExportPacking.fill(ColorExportPacking::None);
    const ShaderRecompiler::RequestSerializer serializer;
    Require(serializer.Deserialize(serializer.Serialize(request)).request.context.pixel->targetExportPacking == request.context.pixel->targetExportPacking, "the 4_4_4_4 unorm packing was lost in serialization");
    const auto packed = ShaderRecompiler::Recompile(request);
    const auto unpacked = ShaderRecompiler::Recompile(plain);
    Require(packed.PipelineVariantId() != unpacked.PipelineVariantId() && !(packed.spirv == unpacked.spirv), "the 4_4_4_4 export packing did not specialize the module");
    ShaderRecompiler::RecompileResult vertex;
    vertex.spirv = makeModule({});
    const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &packed, 0}}};
    Require(AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false) == std::set<std::uint32_t>{0u}, "the 4_4_4_4 packed export did not reach attachment 0");
}

void float11_11_10TargetTests() {
    using ShaderRecompiler::ColorExportPacking;
    auto queue = makeState();
    queue.context[0x31c] = (queue.context.at(0x31c) & ~0x1f7cu) | (7u << 2u) | (7u << 8u) | (2u << 11u) | 0x40000u;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.color.format == VK_FORMAT_R32_UINT && state.color.elementBytes == 4 && state.color.bytes == colorMemory.size() && state.color.componentMapping == 0xc6u && state.color.packing == ColorExportPacking::Float11_11_10, "an 11_11_10 float target did not decode as a packed R32_UINT attachment with blue in the low bits");
    const auto packings = AgcDriver::Graphics::ExportPackings(state);
    Require(packings[0] == ColorExportPacking::Float11_11_10 && std::all_of(packings.begin() + 1, packings.end(), [](ColorExportPacking packing) { return packing == ColorExportPacking::None; }), "export 0 did not carry the 11_11_10 float packing alone");
    Require(AgcDriver::Graphics::ExportMappings(state)[0] == 0xc6u, "export 0 did not reverse its components into the 11_11_10 float target");
    auto changed = queue;
    changed.context[0x31c] &= ~0x40000u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "rounding (ROUND_MODE 0) 11_11_10 float color targets");
    changed = queue;
    changed.context[0x31c] &= ~0x1800u;
    const auto standard = AgcDriver::Graphics::DecodeState(changed);
    Require(standard.color.format == VK_FORMAT_R32_UINT && standard.color.componentMapping == 0xe4u && standard.color.packing == ColorExportPacking::Float11_11_10, "an 11_11_10 float target with the standard component order did not decode");
    Require(AgcDriver::Graphics::ExportMappings(standard)[0] == 0xe4u, "export 0 reordered its components into the standard-order 11_11_10 float target");
    for (const auto swap : {1u, 3u}) {
        changed = queue;
        changed.context[0x31c] = (changed.context[0x31c] & ~0x1800u) | (swap << 11u);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "unsupported color format 7 number type 7 component swap " + std::to_string(swap));
    }
    for (const auto number : {0u, 4u}) {
        changed = queue;
        changed.context[0x31c] = (changed.context[0x31c] & ~0x700u) | (number << 8u) | 0x8000u;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "unsupported color format 7 number type " + std::to_string(number) + " component swap 2");
    }
    changed = queue;
    changed.context[0x1e0] = 0x60010101u;
    for (std::uint32_t i = 0; i < 4; ++i) changed.context[0x105 + i] = 0;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "blending into an 11_11_10 float color target");
    changed = queue;
    changed.context[0x8e] = 6;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "partial writes of an 11_11_10 float color target");
    changed.context[0x8e] = 7;
    Require(AgcDriver::Graphics::DecodeState(changed).color.packing == ColorExportPacking::Float11_11_10, "an RGB write of an 11_11_10 float target was refused");
    for (const auto format : {5u, 6u}) {
        changed = queue;
        changed.context[0x1c5] = format;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "color export format " + std::to_string(format) + " into an 11_11_10 float target");
    }
    changed = queue;
    changed.context[0x1c5] = 4;
    Require(AgcDriver::Graphics::DecodeState(changed).color.packing == ColorExportPacking::Float11_11_10, "an FP16_ABGR export into an 11_11_10 float target was refused");
    changed = queue;
    changed.context[0x31c] |= 0x10000000u;
    changed.context[0x325] = 0x1234;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "DCC-compressed 11_11_10 float color targets");

    constexpr std::array<std::uint32_t, 3> code{0xf800180fu, 0x07060504u, 0xbf810000u};
    const auto request = packedExportRequest(queue, state, code);
    auto unorm = request;
    unorm.context.pixel->targetExportPacking[0] = ColorExportPacking::Unorm10_11_11;
    const ShaderRecompiler::RequestSerializer serializer;
    Require(serializer.Deserialize(serializer.Serialize(request)).request.context.pixel->targetExportPacking == request.context.pixel->targetExportPacking, "the 11_11_10 float packing was lost in serialization");
    const auto packed = ShaderRecompiler::Recompile(request);
    const auto unormPacked = ShaderRecompiler::Recompile(unorm);
    Require(packed.variantId == unormPacked.variantId && packed.PipelineVariantId() != unormPacked.PipelineVariantId(), "the 11_11_10 float packing did not key its own specialization of the shared artifact");
    ShaderRecompiler::RecompileResult vertex;
    vertex.spirv = makeModule({});
    const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &packed, 0}}};
    Require(AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false) == std::set<std::uint32_t>{0u}, "the packed float export did not reach attachment 0");
    auto invalid = request;
    invalid.context.pixel->targetExportPacking[0] = static_cast<ColorExportPacking>(5);
    expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(invalid)); }, "invalid prepared fragment export packing");
}

AgcDriver::QueueState dualSourceQueue() {
    auto queue = makeState();
    queue.context[0x8f] = 0xffu;
    queue.context[0x1c5] = 0x44u;
    queue.context[0x1e0] = 0x40010f01u;
    for (std::uint32_t i = 0; i < 4; ++i) queue.context[0x105 + i] = 0;
    queue.context[0x1b3] = 2;
    queue.context[0x1b4] = 2;
    return queue;
}

struct OutputDecorations {
    std::map<std::uint32_t, std::uint32_t> locations;
    std::map<std::uint32_t, std::uint32_t> indices;
};

OutputDecorations outputDecorations(std::span<const std::uint32_t> words) {
    OutputDecorations result;
    std::set<std::uint32_t> outputs;
    for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
        const auto op = static_cast<spv::Op>(words[at] & 0xffffu);
        if (op == spv::OpVariable && words[at + 3] == spv::StorageClassOutput) outputs.insert(words[at + 2]);
        if (op != spv::OpDecorate) continue;
        if (words[at + 2] == spv::DecorationLocation) result.locations[words[at + 1]] = words[at + 3];
        if (words[at + 2] == spv::DecorationIndex) result.indices[words[at + 1]] = words[at + 3];
    }
    std::erase_if(result.locations, [&](const auto& entry) { return !outputs.contains(entry.first); });
    return result;
}

void DualSourceBlendTests() {
    auto queue = dualSourceQueue();
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.dualSourceBlend && state.colors.size() == 1 && state.blends.size() == 1, "an ONE + SRC1_COLOR blend did not decode as dual-source blending into one target");
    Require(state.blends[0].srcColorBlendFactor == VK_BLEND_FACTOR_ONE && state.blends[0].dstColorBlendFactor == VK_BLEND_FACTOR_SRC1_COLOR && state.blends[0].srcAlphaBlendFactor == VK_BLEND_FACTOR_ONE && state.blends[0].dstAlphaBlendFactor == VK_BLEND_FACTOR_SRC1_COLOR, "BLEND_SRC1_COLOR did not become VK_BLEND_FACTOR_SRC1_COLOR");
    const std::array<std::pair<std::uint32_t, VkBlendFactor>, 4> factors{{{15u, VK_BLEND_FACTOR_SRC1_COLOR}, {16u, VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR}, {17u, VK_BLEND_FACTOR_SRC1_ALPHA}, {18u, VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA}}};
    for (const auto& [value, factor] : factors) {
        auto changed = queue;
        changed.context[0x1e0] = 0x60000000u | (value << 24u) | (1u << 16u) | (value << 8u) | value;
        const auto decoded = AgcDriver::Graphics::DecodeState(changed);
        Require(decoded.dualSourceBlend && decoded.blends[0].srcColorBlendFactor == factor && decoded.blends[0].dstColorBlendFactor == factor && decoded.blends[0].srcAlphaBlendFactor == VK_BLEND_FACTOR_ONE && decoded.blends[0].dstAlphaBlendFactor == factor, "blend factor " + std::to_string(value) + " decoded wrongly");
    }
    auto changed = queue;
    changed.context[0x1e0] = 0x00010f01u;
    Require(!AgcDriver::Graphics::DecodeState(changed).dualSourceBlend, "SRC1 factors of a disabled blend enabled dual-source blending");
    changed = queue;
    changed.context[0x1e0] = 0x40010001u;
    Require(!AgcDriver::Graphics::DecodeState(changed).dualSourceBlend, "a blend without SRC1 factors enabled dual-source blending");
    for (const auto operation : {2u, 3u}) {
        changed.context[0x1e0] = 0x40010f01u | (operation << 5u);
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "MIN or MAX operation");
    }
    changed.context[0x1e0] = 0x40010f01u | (4u << 5u);
    Require(AgcDriver::Graphics::DecodeState(changed).blends[0].colorBlendOp == VK_BLEND_OP_REVERSE_SUBTRACT, "a reverse-subtract dual-source blend was refused");
    changed = queue;
    changed.context[0x8f] = 0xfu;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "MRT slots 0 and 1");
    changed.context[0x8f] = 0xfffu;
    changed.context[0x1c5] = 0x444u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "MRT slots 0 and 1");
    changed.context[0x8f] = 0xf000fu;
    changed.context[0x1c5] = 0x44u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "MRT slots 0 and 1");
    alignas(256) static std::array<std::byte, 1024> slotOneMemory{};
    changed = queue;
    for (const auto offset : {0x31bu, 0x31cu, 0x31du}) changed.context[offset + 0xfu] = changed.context.at(offset);
    for (const auto offset : {0x3b0u, 0x3b8u}) changed.context[offset + 1u] = changed.context.at(offset);
    changed.context[0x318 + 0xfu] = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(slotOneMemory.data()) >> 8u);
    changed.context[0x390 + 1u] = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(slotOneMemory.data()) >> 40u);
    changed.context[0x1e1] = 0;
    changed.context[0x8e] = 0xffu;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "color writes to slot 0 alone");
    for (const auto format : {0x04u, 0x14u, 0x74u, 0x94u}) {
        changed = queue;
        changed.context[0x1c5] = format;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "MRT1 export in the MRT0 export format");
    }
    changed = queue;
    changed.context[0x1c5] = 0x99u;
    Require(AgcDriver::Graphics::DecodeState(changed).dualSourceBlend, "a 32_ABGR MRT0 and MRT1 export pair was refused");
    for (const auto mask : {0x7fu, 0xf7u}) {
        changed = queue;
        changed.context[0x8f] = mask;
        expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "MRT1 export with the MRT0 shader mask");
    }
    changed = queue;
    changed.context[0x31c] = 0x8804u;
    expectFailure([&] { AgcDriver::Graphics::DecodeState(changed); }, "component-swapped color target");

    constexpr std::array<std::uint32_t, 5> code{0xf800000fu, 0x07060504u, 0xf800181fu, 0x0b0a0908u, 0xbf810000u};
    ShaderRecompiler::RecompileRequest request{};
    request.shader = {ShaderRecompiler::ShaderStage::Fragment, 0x30000u, code, 0, {}};
    request.context.waveSize = 64;
    request.context.pixel = AgcDriver::Graphics::DecodePixelStageInfo(queue.context, AgcDriver::Graphics::ExportMappings(state));
    request.context.pixel->dualSourceBlend = state.dualSourceBlend;
    request.target.vulkanVersion = 0x00401000u;
    request.target.spirvVersion = 0x00010300u;
    request.target.subgroupSize = 64;
    request.layout.pushConstantSizeBytes = 128;
    request.useCache = true;
    auto plain = request;
    plain.context.pixel->dualSourceBlend = false;
    const ShaderRecompiler::RequestSerializer serializer;
    Require(serializer.Deserialize(serializer.Serialize(request)).request.context.pixel->dualSourceBlend, "dual-source blending was lost in serialization");
    std::vector<std::uint64_t> key;
    std::vector<std::uint64_t> plainKey;
    ShaderRecompiler::BuildPreparedShaderKey(request, key);
    ShaderRecompiler::BuildPreparedShaderKey(plain, plainKey);
    Require(key == plainKey, "dual-source blending entered the static shader ABI");
    const auto prepared = ShaderRecompiler::PrepareShader(plain);
    Require(ShaderRecompiler::MatchesPreparedShader(request, *prepared), "a dual-source draw did not match the registered pixel shader");
    const auto dual = ShaderRecompiler::Recompile(request);
    const auto single = ShaderRecompiler::Recompile(plain);
    Require(dual.variantId == single.variantId && dual.variantId == ShaderRecompiler::GetPreparedArtifact(*prepared).variantId, "the dual-source draw did not reuse the registered artifact");
    Require(dual.PipelineVariantId() != single.PipelineVariantId() && ShaderRecompiler::Recompile(request).PipelineVariantId() == dual.PipelineVariantId(), "dual-source blending did not key its own specialization");
    const auto dualOutputs = outputDecorations(dual.spirv.Words());
    const auto singleOutputs = outputDecorations(single.spirv.Words());
    Require(dualOutputs.locations.size() == 2 && std::all_of(dualOutputs.locations.begin(), dualOutputs.locations.end(), [](const auto& entry) { return entry.second == 0u; }) && dualOutputs.indices.size() == 1 && dualOutputs.indices.begin()->second == 1u, "the MRT1 export of a dual-source draw was not the location 0 index 1 output");
    std::set<std::uint32_t> singleLocations;
    for (const auto& entry : singleOutputs.locations) singleLocations.insert(entry.second);
    Require(singleOutputs.indices.empty() && singleLocations == std::set<std::uint32_t>{0u, 1u}, "MRT0 and MRT1 without dual-source blending did not keep locations 0 and 1");
    ShaderRecompiler::RecompileResult vertex;
    vertex.spirv = makeModule({});
    const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    const auto validate = [&](const ShaderRecompiler::RecompileResult& pixel, const AgcDriver::Graphics::State& target) {
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &pixel, 0}}};
        return AgcDriver::Graphics::ValidateShaders(shaders, target, subgroup, false);
    };
    Require(validate(dual, state) == std::set<std::uint32_t>{0u}, "the dual-source shader did not validate against its draw");
    expectFailure([&] { validate(single, state); }, "needs a float4 second color at location 0 index 1");
    auto singleState = state;
    singleState.dualSourceBlend = false;
    expectFailure([&] { validate(dual, singleState); }, "Index decoration");
}

void ReversedBlendShaderTests() {
    const auto queue = reversedBlendQueue(10u, 0u, 2u, 0x65010504u);
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    Require(state.reversedBlend == 0x570001u && !state.dualSourceBlend && state.blends.size() == 1, "the reversed-order source-alpha blend did not decode to a pixel shader blend code");
    constexpr std::array<std::uint32_t, 3> code{0xf800180fu, 0x07060504u, 0xbf810000u};
    auto request = packedExportRequest(queue, state, code);
    request.context.pixel->reversedBlend = state.reversedBlend;
    auto plain = request;
    plain.context.pixel->reversedBlend = 0u;
    const ShaderRecompiler::RequestSerializer serializer;
    Require(serializer.Deserialize(serializer.Serialize(request)).request.context.pixel->reversedBlend == 0x570001u, "the reversed-order blend code was lost in serialization");
    std::vector<std::uint64_t> key;
    std::vector<std::uint64_t> plainKey;
    ShaderRecompiler::BuildPreparedShaderKey(request, key);
    ShaderRecompiler::BuildPreparedShaderKey(plain, plainKey);
    Require(key == plainKey, "the reversed-order blend code entered the static shader ABI");
    const auto prepared = ShaderRecompiler::PrepareShader(plain);
    Require(ShaderRecompiler::MatchesPreparedShader(request, *prepared), "a reversed-order blended draw did not match the registered pixel shader");
    const auto reversed = ShaderRecompiler::Recompile(request);
    const auto single = ShaderRecompiler::Recompile(plain);
    Require(reversed.variantId == single.variantId && reversed.variantId == ShaderRecompiler::GetPreparedArtifact(*prepared).variantId, "the reversed-order blended draw did not reuse the registered artifact");
    Require(reversed.PipelineVariantId() != single.PipelineVariantId() && ShaderRecompiler::Recompile(request).PipelineVariantId() == reversed.PipelineVariantId(), "the reversed-order blend code did not key its own specialization");
    const auto arithmetic = [](std::span<const std::uint32_t> words) {
        std::size_t count = 0;
        for (std::size_t at = 5; at < words.size() && (words[at] >> 16u) != 0; at += words[at] >> 16u) {
            const auto op = static_cast<spv::Op>(words[at] & 0xffffu);
            if (op == spv::OpFAdd || op == spv::OpFSub || op == spv::OpFMul || op == spv::OpFDiv || op == spv::OpFNegate || op == spv::OpExtInst || op == spv::OpSelect) ++count;
        }
        return count;
    };
    for (const auto shaderCode : {0x570001u, 0x540001u, 0x560001u, 0x020001u, 0x560301u, 0xffff01u}) {
        auto changed = request;
        changed.context.pixel->reversedBlend = shaderCode;
        const auto specialized = ShaderRecompiler::Recompile(changed);
        const auto words = specialized.spirv.Words();
        const auto outputs = outputDecorations(words);
        Require(outputs.locations.size() == 2 && std::all_of(outputs.locations.begin(), outputs.locations.end(), [](const auto& entry) { return entry.second == 0u; }) && outputs.indices.size() == 1 && outputs.indices.begin()->second == 1u, "blend code " + std::to_string(shaderCode) + " did not add a second color at location 0 index 1");
        Require(arithmetic(words) == arithmetic(single.spirv.Words()), "blend code " + std::to_string(shaderCode) + " computed in the pixel shader instead of copying the export");
    }
    const auto singleOutputs = outputDecorations(single.spirv.Words());
    Require(singleOutputs.locations.size() == 1 && singleOutputs.locations.begin()->second == 0u && singleOutputs.indices.empty(), "a pixel shader without a reversed-order blend code changed its outputs");
    ShaderRecompiler::RecompileResult vertex;
    vertex.spirv = makeModule({});
    const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    const auto validate = [&](const ShaderRecompiler::RecompileResult& pixel, const AgcDriver::Graphics::State& target) {
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &pixel, 0}}};
        return AgcDriver::Graphics::ValidateShaders(shaders, target, subgroup, false);
    };
    Require(validate(reversed, state) == std::set<std::uint32_t>{0u}, "the reversed-order blended shader did not validate against its draw");
    expectFailure([&] { validate(single, state); }, "needs a float4 second color at location 0 index 1");
    auto plainState = state;
    plainState.reversedBlend = 0u;
    expectFailure([&] { validate(reversed, plainState); }, "Index decoration");
    for (const auto invalid : {0x2u, 0x80u, 0x100u, 0x10000u, 0x1000001u, 0x80000001u}) {
        auto changed = request;
        changed.context.pixel->reversedBlend = invalid;
        expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(changed)); }, "invalid prepared reversed-order blend");
    }
    const auto twoQueue = dualSourceQueue();
    const auto twoState = AgcDriver::Graphics::DecodeState(twoQueue);
    constexpr std::array<std::uint32_t, 5> twoExports{0xf800000fu, 0x07060504u, 0xf800181fu, 0x0b0a0908u, 0xbf810000u};
    auto both = packedExportRequest(twoQueue, twoState, twoExports);
    both.context.pixel->reversedBlend = 0x570001u;
    expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(both)); }, "reversed-order blending needs MRT0 to be the only color output");
    both.context.pixel->dualSourceBlend = true;
    expectFailure([&] { static_cast<void>(ShaderRecompiler::Recompile(both)); }, "reversed-order blending cannot be combined with dual-source blending");
}

void validationTests() {
    AgcDriver::Graphics::State state{};
    state.stages.path = AgcDriver::Graphics::ShaderPath::Vertex;
    ShaderRecompiler::RecompileResult fragment;
    fragment.spirv = makeModule({.fragment = true});
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.parameterOutput = true});
        ShaderRecompiler::RecompileResult pixel;
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &pixel, 0}}};
        const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        for (const auto noPerspective : {false, true}) {
            pixel.spirv = makeModule({.fragment = true, .barycentric = true, .barycentricNoPerspective = noPerspective, .perVertex = true});
            AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, true);
            expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "fragmentShaderBarycentric");
        }
        pixel.spirv = makeModule({.fragment = true, .sampleId = true, .layer = true, .fragDepth = true, .sampleMaskLength = 1});
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, true, true);
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, true); }, "unsupported device capability 2");
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, true, false); }, "unsupported device capability 35");
        pixel.spirv = makeModule({.fragment = true, .sampleMaskLength = 2});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "unsupported fragment built-in");
        pixel.spirv = makeModule({.fragment = true});
        vertex.spirv = makeModule({.parameterOutput = true, .clipDistanceLength = 4, .cullDistanceLength = 4});
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, true, true);
        state.paClVsOutCntl = 0x0040000fu;
        vertex.spirv = makeModule({.parameterOutput = true, .clipDistanceLength = 4});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, false, true); }, "PA_CL_VS_OUT_CNTL=0x0040000f");
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, true, false);
        vertex.spirv = makeModule({.parameterOutput = true, .cullDistanceLength = 4});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, true, false); }, "PA_CL_VS_OUT_CNTL=0x0040000f");
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, false, true);
        state.paClVsOutCntl = 0;
        vertex.spirv = makeModule({.parameterOutput = true, .clipDistanceLength = 9});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, true, true); }, "clip or cull distance");
        vertex.spirv = makeModule({.parameterOutput = true, .sampleId = true});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "unsupported device capability 35");
        vertex.spirv = makeModule({.parameterOutput = true});
        pixel.spirv = makeModule({.fragment = true, .barycentric = true, .barycentricComponents = 4});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, true); }, "invalid barycentric built-in");
        pixel.spirv = makeModule({.fragment = true, .barycentric = true, .perVertex = true, .perVertexLength = 2});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, true); }, "three vertices");
        pixel.spirv = makeModule({.fragment = true, .perVertex = true});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, true); }, "PerVertexKHR requires");
        vertex.spirv = makeModule({.barycentric = true});
        pixel.spirv = makeModule({.fragment = true});
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, true); }, "requires a fragment shader");
        const auto validateTerminate = [&](bool enabled) {
            return AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, false, false, false, false, false, enabled);
        };
        vertex.spirv = makeModule({});
        pixel.spirv = makeModule({.fragment = true, .terminateInvocation = true});
        validateTerminate(true);
        expectFailure([&] { validateTerminate(false); }, "shaderTerminateInvocation");
        vertex.spirv = makeModule({.terminateInvocation = true});
        pixel.spirv = makeModule({.fragment = true});
        expectFailure([&] { validateTerminate(true); }, "fragment shader");
        vertex.spirv = makeModule({});
        pixel.spirv = makeModule({.fragment = true, .terminateInvocation = true, .omitTerminateExtension = true});
        expectFailure([&] { validateTerminate(true); }, "OpTerminateInvocation requires SPV_KHR_terminate_invocation");
        expectFailure([&] { validateTerminate(false); }, "OpTerminateInvocation requires enabled shaderTerminateInvocation");
        vertex.spirv = makeModule({.terminateInvocation = true, .omitTerminateExtension = true});
        pixel.spirv = makeModule({.fragment = true});
        expectFailure([&] { validateTerminate(true); }, "OpTerminateInvocation requires enabled shaderTerminateInvocation in a fragment shader");
        vertex.spirv = makeModule({});
        pixel.spirv = ShaderRecompiler::SpecializeSpirv(makeModule({.fragment = true, .terminateInvocation = true}));
        std::size_t terminates = 0;
        std::size_t declarations = 0;
        for (std::size_t at = 5; at < pixel.spirv.size(); at += pixel.spirv[at] >> 16u) {
            const auto op = pixel.spirv[at] & 0xffffu;
            if (op == spv::OpTerminateInvocation) ++terminates;
            if (op == spv::OpExtension && std::string_view(reinterpret_cast<const char*>(&pixel.spirv[at + 1])) == "SPV_KHR_terminate_invocation") ++declarations;
        }
        Require(terminates == 1u && declarations == 1u, "specialization dropped OpTerminateInvocation or its extension declaration");
        validateTerminate(true);
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({});
        ShaderRecompiler::RecompileResult pixel;
        pixel.spirv = makeModule({.fragment = true, .secondTarget = true});
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &pixel, 0}}};
        const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        Require(state.colors.empty() && AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false) == std::set<std::uint32_t>{0u}, "an export past the attachments was not dropped");
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.vertexInput = true});
        ShaderRecompiler::VertexAttribute attribute{0, 4, {{0x1000, 32u << 16u, 3, 77u << 12u}}, 0};
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, 0}}};
        const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "missing attribute metadata");
        vertex.vertexAttributes.push_back(attribute);
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false);
        vertex.vertexAttributes[0].components = 2;
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "metadata disagrees");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 2, 1) == 80, "incorrect strided vertex range");
        Require(AgcDriver::Graphics::VertexBufferReadOffset(attribute, 2, 7) == 64, "vertex range includes records before the first vertex");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 3, 1) == 112, "an index past the records still spans its fetch");
        {
            auto wider = attribute;
            wider.resource.fields[2] = 9;
            auto instanced = attribute;
            instanced.fetchIndex = 1;
            instanced.resource.fields[2] = 1000;
            auto constant = attribute;
            constant.resource.fields[1] = 0;
            const std::array attributes{attribute, wider, instanced, constant};
            Require(AgcDriver::Graphics::ZeroFetchVertex(attributes) == 9u, "the first vertex reading zeros is not past the largest per-vertex record count");
            auto clamped = attribute;
            clamped.resource.fields[3] |= 2u << 28u;
            const std::array selective{attribute, clamped};
            Require(!AgcDriver::Graphics::ZeroFetchVertex(selective).has_value(), "an attribute with out-of-bounds select 2 was taken to read zeros past its records");
            std::array<std::uint32_t, 4> indices{0xffffffffu, 2u, 0x12345u, 0xffffffffu};
            Require(AgcDriver::Graphics::ClampDrawIndices(std::as_writable_bytes(std::span(indices)), 4, true, 9u) == 9u && indices == std::array<std::uint32_t, 4>{0xffffffffu, 2u, 9u, 0xffffffffu}, "clamping kept an index past the limit or moved a restart index");
            Require(AgcDriver::Graphics::ClampDrawIndices(std::as_writable_bytes(std::span(indices)), 4, false, 9u) == 9u && indices == std::array<std::uint32_t, 4>{9u, 2u, 9u, 9u}, "clamping without restart kept a 0xffffffff index");
            std::array<std::uint16_t, 2> narrow{0xffffu, 1u};
            Require(AgcDriver::Graphics::ClampDrawIndices(std::as_writable_bytes(std::span(narrow)), 2, false, 4u) == 4u && narrow[0] == 4u && narrow[1] == 1u, "clamping 16-bit indices failed");
        }
        attribute.fetchIndex = 1;
        Require(AgcDriver::Graphics::VertexBufferReadOffset(attribute, 2, 7) == 224, "instance range includes records before the first instance");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 100, 2) == 48, "instance attributes used the vertex index");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 100, 2, 1) == 80, "first instance was ignored");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 2, 2) == 112, "a first instance past the records still spans its fetch");
        expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 2, 0xffffffffu); }, "instance range overflow");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 4) == 112, "an instance range past the records still spans its fetch");
        attribute.fetchIndex = 0;
        attribute.resource.fields[3] = 0x1004dfacu;
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 3, 1) == 112, "oob_select 1 past the records still spans its fetch");
        attribute.resource.fields[3] = 0x2004dfacu;
        expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 3, 1); }, "record count");
        attribute.resource.fields[3] = 0x3004dfacu;
        expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 3, 1); }, "record count");
        attribute.resource.fields[3] = 77u << 12u;
        attribute.fetchIndex = 1;
        attribute.resource.fields[1] = 0;
        const auto zeroStride = [&](std::uint32_t select, std::uint32_t records) {
            attribute.resource.fields[3] = (77u << 12u) | (select << 28u);
            attribute.resource.fields[2] = records;
        };
        for (const auto records : {0u, 1u, 16u, 0xffffffffu}) {
            zeroStride(0, records);
            Require(AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 0 must read every zero-stride fetch as zero");
            Require(AgcDriver::Graphics::ZeroStrideAvailableBytes(attribute) == 0, "OOB_SELECT 0 zero-stride fetch has bytes to read");
            expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 1); }, "byte range");
            expectFailure([&] { AgcDriver::Graphics::VertexBufferExtent(attribute); }, "empty or oversized");
        }
        zeroStride(1, 16);
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 100, 2) == 16, "zero stride must repeat one value");
        Require(AgcDriver::Graphics::VertexBufferReadOffset(attribute, 100, 7) == 0, "zero-stride input skipped its repeated value");
        Require(AgcDriver::Graphics::VertexBufferExtent(attribute) == 16, "OOB_SELECT 1 extent must cover the whole fetch");
        zeroStride(1, 0);
        Require(AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 1 without records must read zero");
        zeroStride(1, 8);
        Require(!AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 1 counted NUM_RECORDS in bytes");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 8) == 16, "OOB_SELECT 1 must read the whole element for every index below NUM_RECORDS");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 1, 7) == 16, "OOB_SELECT 1 must read the whole element for the last index below NUM_RECORDS");
        expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 9); }, "record count");
        expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 1, 8); }, "record count");
        zeroStride(1, 1);
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 1) == 16, "OOB_SELECT 1 with one record must read the element at index 0");
        expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 2); }, "record count");
        attribute.fetchIndex = 0;
        zeroStride(1, 3);
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 2, 1) == 16, "OOB_SELECT 1 must read the element for index 2 of 3 records");
        expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 3, 1); }, "record count");
        attribute.fetchIndex = 1;
        zeroStride(2, 1);
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 100, 2) == 16, "OOB_SELECT 2 must read a zero-stride fetch whole while NUM_RECORDS is nonzero");
        Require(AgcDriver::Graphics::VertexBufferExtent(attribute) == 16, "OOB_SELECT 2 extent must cover the whole fetch");
        Require(!AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 2 with records was treated as out of range");
        zeroStride(2, 0);
        Require(AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 2 without records must read zero");
        zeroStride(3, 0);
        Require(AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 3 without records must read zero");
        for (const auto records : {1u, 8u, 15u}) {
            zeroStride(3, records);
            Require(AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 3 must read a fetch that only partly fits as zero");
            expectFailure([&] { AgcDriver::Graphics::VertexBufferReadSize(attribute, 0, 1); }, "byte range");
        }
        for (const auto records : {16u, 17u, 0xffffffffu}) {
            zeroStride(3, records);
            Require(!AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 3 with the whole fetch in range was treated as out of range");
            Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 100, 2) == 16, "OOB_SELECT 3 must read a fetch inside the range, whatever the index");
            Require(AgcDriver::Graphics::VertexBufferExtent(attribute) == 16, "OOB_SELECT 3 extent must cover only the fetch");
        }
        attribute.resource.fields[3] = (22u << 12u) | (3u << 28u);
        attribute.resource.fields[2] = 4;
        Require(!AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 3 compared NUM_RECORDS with the wrong fetch size");
        attribute.resource.fields[2] = 3;
        Require(AgcDriver::Graphics::VertexFetchOutOfRange(attribute), "OOB_SELECT 3 read a 4-byte fetch past 3 bytes");
        attribute.resource.fields[3] = (113u << 12u) | (3u << 28u);
        expectFailure([&] { AgcDriver::Graphics::VertexFetchOutOfRange(attribute); }, "unsupported vertex format");
        attribute.resource.fields[3] = 113u << 12u;
        expectFailure([&] { AgcDriver::Graphics::DecodeVertexFormat(attribute); }, "unsupported vertex format");
        attribute.resource.fields[3] = 50u << 12u;
        Require(AgcDriver::Graphics::DecodeVertexFormat(attribute).format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 && AgcDriver::Graphics::DecodeVertexFormat(attribute).bytes == 4, "2_10_10_10 vertex format was not decoded");
        attribute.resource.fields[3] = 55u << 12u;
        Require(AgcDriver::Graphics::DecodeVertexFormat(attribute).format == VK_FORMAT_A2B10G10R10_SINT_PACK32 && std::string_view(AgcDriver::Graphics::DecodeVertexFormat(attribute).scalar) == "i32", "2_10_10_10 sint vertex format was not decoded");
        attribute.resource.fields[3] = 36u << 12u;
        Require(AgcDriver::Graphics::DecodeVertexFormat(attribute).format == VK_FORMAT_B10G11R11_UFLOAT_PACK32, "10_11_11 float vertex format was not decoded");
        for (std::uint32_t format = 1; format < 128u; ++format) {
            attribute.resource.fields[3] = format << 12u;
            std::string_view scalar;
            try {
                scalar = AgcDriver::Graphics::DecodeVertexFormat(attribute).scalar;
            } catch (const std::exception&) {
                continue;
            }
            const auto numeric = ShaderRecompiler::VertexInputNumericClass(static_cast<ShaderRecompiler::IrBufferFormat>(format));
            Require((scalar == "u32") == (numeric == ShaderRecompiler::IrTextureNumericClass::Uint) && (scalar == "i32") == (numeric == ShaderRecompiler::IrTextureNumericClass::Sint), "vertex format " + std::to_string(format) + " declares a shader input of another numeric type than its attribute");
        }
        attribute.resource.fields[3] = 43u << 12u;
        expectFailure([&] { AgcDriver::Graphics::DecodeVertexFormat(attribute); }, "unsupported vertex format");
        attribute.resource.fields = {0x1000, 32u << 16u, 3, 77u << 12u};
        attribute.components = 2;
        AgcDriver::Graphics::Context context{};
        context.limits.maxVertexInputBindings = 16;
        context.limits.maxVertexInputAttributes = 16;
        context.limits.maxVertexInputBindingStride = 2048;
        context.formatProperties = [](VkPhysicalDevice, VkFormat format, VkFormatProperties* properties) {
            *properties = {};
            if (format == VK_FORMAT_R32G32_SFLOAT) properties->bufferFeatures = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
        };
        const auto layout = AgcDriver::Graphics::BuildVertexInputLayout(context, std::span(&attribute, 1));
        Require(layout.bindings.size() == 1 && layout.bindings[0].stride == 32 && layout.bindings[0].inputRate == VK_VERTEX_INPUT_RATE_INSTANCE, "incorrect instance input binding");
        Require(layout.attributes[0].format == VK_FORMAT_R32G32_SFLOAT && layout.attributes[0].offset == 0 && layout.attributes[0].location == 0, "incorrect vertex attribute format or offset");
        attribute.components = 4;
        expectFailure([&] { AgcDriver::Graphics::BuildVertexInputLayout(context, std::span(&attribute, 1)); }, "device does not support vertex format");
        attribute.components = 2;
        attribute.resource.fields[1] |= 0x80000000u;
        expectFailure([&] { AgcDriver::Graphics::BuildVertexInputLayout(context, std::span(&attribute, 1)); }, "descriptor flags");
        attribute.resource.fields = {0x1000, 8u << 16u, 3, 71u << 12u};
        attribute.components = 3;
        context.formatProperties = [](VkPhysicalDevice, VkFormat format, VkFormatProperties* properties) {
            *properties = {};
            if (format == VK_FORMAT_R16G16B16A16_SFLOAT || format == VK_FORMAT_R8G8B8A8_UNORM) properties->bufferFeatures = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
        };
        Require(AgcDriver::Graphics::DecodeVertexFormat(attribute).format == VK_FORMAT_R16G16B16_SFLOAT && AgcDriver::Graphics::DecodeVertexFormat(attribute).bytes == 6, "a 3-component 16_16_16_16 float attribute was not decoded as 3 components");
        const auto widened = AgcDriver::Graphics::BuildVertexInputLayout(context, std::span(&attribute, 1));
        Require(widened.attributes[0].format == VK_FORMAT_R16G16B16A16_SFLOAT && widened.bindings[0].stride == 8 && AgcDriver::Graphics::VertexReadBytes(attribute, widened.attributes[0].format) == 8, "a 3-component 16_16_16_16 float attribute was not widened to the format the device supports");
        Require(AgcDriver::Graphics::VertexAttributeSignature(attribute) == "f32x3", "widening the vertex format changed the shader input");
        attribute.resource.fields = {0x1000, 4u << 16u, 3, 56u << 12u};
        Require(AgcDriver::Graphics::BuildVertexInputLayout(context, std::span(&attribute, 1)).attributes[0].format == VK_FORMAT_R8G8B8A8_UNORM, "a 3-component 8_8_8_8 unorm attribute was not widened to the format the device supports");
        attribute.resource.fields = {0x1000, 12u << 16u, 3, 74u << 12u};
        expectFailure([&] { AgcDriver::Graphics::BuildVertexInputLayout(context, std::span(&attribute, 1)); }, "device does not support vertex format 106");
        expectFailure([&] { AgcDriver::Graphics::VertexReadBytes(attribute, VK_FORMAT_R32G32B32A32_SFLOAT); }, "is not a form of");
    }
    for (const auto capability : {spv::CapabilityInt64Atomics, spv::CapabilityInt64ImageEXT}) {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({});
        vertex.spirv.insert(vertex.spirv.begin() + 5, {(2u << 16u) | spv::OpCapability, static_cast<std::uint32_t>(capability)});
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, 0}}};
        const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "unsupported device capability");
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, true);
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({});
        vertex.spirv.insert(vertex.spirv.begin() + 5, {(2u << 16u) | spv::OpCapability, static_cast<std::uint32_t>(spv::CapabilityInt64Atomics)});
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, 0}}};
        const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "unsupported device capability 12");
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, true);
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.layerOutput = true});
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, 0}}};
        const VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "VK_EXT_shader_viewport_index_layer");
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false, false, false, false, false, false, false, false, true);
    }
    for (const auto capability : {spv::CapabilityGroupNonUniform, spv::CapabilityGroupNonUniformBallot, spv::CapabilityGroupNonUniformShuffle}) {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({});
        vertex.spirv.insert(vertex.spirv.begin() + 5, {(2u << 16u) | spv::OpCapability, static_cast<std::uint32_t>(capability)});
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, 0}}};
        VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        subgroup.supportedStages = VK_SHADER_STAGE_VERTEX_BIT;
        subgroup.supportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_BALLOT_BIT | VK_SUBGROUP_FEATURE_SHUFFLE_BIT;
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false);
        subgroup.supportedStages = VK_SHADER_STAGE_FRAGMENT_BIT;
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "unsupported for shader stage");
        subgroup.supportedStages = VK_SHADER_STAGE_VERTEX_BIT;
        subgroup.supportedOperations = capability == spv::CapabilityGroupNonUniform ? 0u : VK_SUBGROUP_FEATURE_BASIC_BIT;
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "device lacks operations");
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({});
        vertex.spirv.insert(vertex.spirv.begin() + 5, {(2u << 16u) | spv::OpCapability, static_cast<std::uint32_t>(spv::CapabilityGroupNonUniformArithmetic)});
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, 0}}};
        VkPhysicalDeviceSubgroupProperties subgroup{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        subgroup.supportedStages = VK_SHADER_STAGE_FRAGMENT_BIT;
        subgroup.supportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT;
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "unsupported for shader stage");
        subgroup.supportedStages = VK_SHADER_STAGE_VERTEX_BIT;
        subgroup.supportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_BALLOT_BIT | VK_SUBGROUP_FEATURE_SHUFFLE_BIT;
        expectFailure([&] { AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false); }, "device lacks operations");
        subgroup.supportedOperations |= VK_SUBGROUP_FEATURE_ARITHMETIC_BIT;
        AgcDriver::Graphics::ValidateShaders(shaders, state, subgroup, false);
    }
    const std::vector<std::uint32_t> words(8, 0);
    const auto validate = [&](const ShaderRecompiler::RecompileResult& vertex, std::uint32_t fragmentOffset) {
        const std::array<AgcDriver::Graphics::CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, fragmentOffset}}};
        AgcDriver::Graphics::ValidateShaders(shaders, state, VkPhysicalDeviceSubgroupProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES}, false);
    };
    const auto pushed = [&](const ModuleShape& shape) {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule(shape);
        vertex.pushConstants.assign(8, std::byte{1});
        vertex.bindings.push_back(makeBinding(Role::GuestBuffers, 0, 2, words));
        return vertex;
    };
    validate(pushed({.push = true, .bufferArray = 2}), 8);
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.shaderData = true});
        vertex.bindings.push_back(makeBinding(Role::ShaderData, 5, 1, {1, 2}));
        validate(vertex, 0);
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.bufferArray = 1, .shaderData = true});
        vertex.bindings.push_back(makeBinding(Role::GuestBuffers, 0, 1, {1, 2, 3, 4}));
        vertex.bindings.push_back(makeBinding(Role::ShaderData, 5, 1, {1, 2}));
        validate(vertex, 0);
    }
    expectFailure([&] { validate(pushed({.push = true, .pushLength = 16, .bufferArray = 2}), 8); }, "32 elements");
    expectFailure([&] { validate(pushed({.push = true, .pushStride = 8, .bufferArray = 2}), 8); }, "ArrayStride of 4");
    expectFailure([&] { validate(pushed({.push = false, .bufferArray = 2}), 8); }, "push constant metadata disagrees with SPIR-V");
    {
        auto vertex = pushed({.push = true, .bufferArray = 2});
        vertex.pushConstants.clear();
        expectFailure([&] { validate(vertex, 8); }, "invalid push constant interface");
    }
    {
        auto vertex = pushed({.push = true, .bufferArray = 3});
        expectFailure([&] { validate(vertex, 8); }, "descriptor array length disagrees");
    }
    {
        auto vertex = pushed({.push = true, .plainBuffer = true});
        expectFailure([&] { validate(vertex, 8); }, "must be declared as a descriptor array");
    }
    {
        auto vertex = pushed({.push = true, .bufferArray = 2});
        vertex.bindings.front().readOnly = true;
        expectFailure([&] { validate(vertex, 8); }, "read-only descriptor metadata is unsupported");
    }
    {
        auto vertex = pushed({.push = true, .bufferArray = 2});
        vertex.bindings.front().descriptorSet = 1;
        expectFailure([&] { validate(vertex, 8); }, "descriptor set other than its stage's");
    }
    {
        auto vertex = pushed({.push = true, .bufferArray = 2});
        vertex.bindings.front().kind = Kind::UniformBuffer;
        expectFailure([&] { validate(vertex, 8); }, "disagrees with recompiler binding metadata");
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.shaderData = true});
        vertex.bindings.push_back(makeBinding(Role::ShaderData, 5, 2, {1, 2}));
        expectFailure([&] { validate(vertex, 0); }, "binding count of one");
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.shaderData = true});
        vertex.bindings.push_back(makeBinding(Role::GuestSamplers, 5, 1, {1, 2}));
        expectFailure([&] { validate(vertex, 0); }, "descriptor role is unsupported");
    }
    {
        ShaderRecompiler::RecompileResult vertex;
        vertex.spirv = makeModule({.shaderData = true});
        expectFailure([&] { validate(vertex, 0); }, "absent from recompiler binding metadata");
    }
}

}

bool recompilesDebugBranch(std::uint32_t opcode) {
    auto queue = makeState();
    queue.context[0x1b3] = 0x2u;
    queue.context[0x1b4] = 0x2u;
    const auto state = AgcDriver::Graphics::DecodeState(queue);
    const auto pixel = AgcDriver::Graphics::DecodePixelStageInfo(queue.context, AgcDriver::Graphics::ExportMappings(state));
    const std::array<std::uint32_t, 4> code{0xbf800000u | (opcode << 16u) | 1u, 0xf800180fu, 0x00000000u, 0xbf810000u};
    ShaderRecompiler::RecompileRequest request{};
    request.shader = {ShaderRecompiler::ShaderStage::Fragment, 0x30000u, code, 0, {}};
    request.context.waveSize = 64;
    request.context.pixel = pixel;
    request.target.vulkanVersion = 0x00401000u;
    request.target.spirvVersion = 0x00010300u;
    request.target.subgroupSize = 64;
    request.target.fragmentShaderBarycentricEnabled = true;
    request.layout.pushConstantSizeBytes = 128;
    request.useCache = false;
    return !ShaderRecompiler::Recompile(request).spirv.Words().empty();
}

void meshIndexBufferTests() {
    const AgcDriver::Pm4::DrawParameters automatic{0, 96, 0, 1, 0, false};
    const auto unindexed = AgcDriver::Graphics::MeshIndexBufferDescriptor(automatic);
    Require(unindexed[0] == 0 && unindexed[1] == 0 && unindexed[2] == 0 && unindexed[3] == 0x31016facu, "a non-indexed mesh draw did not get a null index buffer V#");
    const AgcDriver::Pm4::DrawParameters indexed{0x123456789a00ull, 5, 2, 1, 0, true};
    const auto words = AgcDriver::Graphics::MeshIndexBufferDescriptor(indexed);
    Require(words[0] == 0x56789a00u && words[1] == 0x1234u && words[2] == 12u && words[3] == 0x31016facu, "an indexed mesh draw's index buffer V# changed");
    expectFailure([] { static_cast<void>(AgcDriver::Graphics::MeshIndexBufferDescriptor(AgcDriver::Pm4::DrawParameters{0, 3, 2, 1, 0, true})); }, "invalid mesh index buffer range");
}

void stagingPoolBudgetTests() {
    if (std::getenv("APS5_STAGING_POOL_MIB") != nullptr) return;
    VkPhysicalDeviceMemoryProperties memory{};
    memory.memoryHeapCount = 2;
    memory.memoryHeaps[0] = {VkDeviceSize{32} << 30u, 0};
    memory.memoryHeaps[1] = {VkDeviceSize{16} << 30u, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT};
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory) == VkDeviceSize{2} << 30u, "the staging pool budget is not an eighth of the device-local heap");
    memory.memoryHeaps[1].size = VkDeviceSize{2} << 30u;
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory) == VkDeviceSize{512} << 20u, "the staging pool budget fell below 512 MiB");
    memory.memoryHeapCount = 1;
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory) == VkDeviceSize{512} << 20u, "a host heap sized the staging pool budget");
    memory.memoryHeapCount = 2;
    memory.memoryHeaps[1].size = VkDeviceSize{16} << 30u;
    memory.memoryTypeCount = 2;
    memory.memoryTypes[0] = {VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0};
    memory.memoryTypes[1] = {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1};
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory) == VkDeviceSize{256} << 20u, "the staging pool budget on a 16 GiB heap of system memory is not a sixty-fourth of it");
    memory.memoryHeaps[1].size = VkDeviceSize{4} << 30u;
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory) == VkDeviceSize{128} << 20u, "the staging pool budget on system memory fell below 128 MiB");
    memory.memoryTypes[0] = {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1};
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory) == VkDeviceSize{512} << 20u, "a heap with a device-only memory type was taken for system memory");
    memory.memoryHeaps[1].size = VkDeviceSize{16} << 30u;
    VkPhysicalDeviceMemoryBudgetPropertiesEXT reported{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory, &reported) == VkDeviceSize{2} << 30u, "a zero reported budget did not fall back to an eighth of the heap");
    reported.heapBudget[0] = VkDeviceSize{1} << 30u;
    reported.heapBudget[1] = VkDeviceSize{10} << 30u;
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory, &reported) == VkDeviceSize{1280} << 20u, "a 10 GiB budget on the 16 GiB heap does not limit the staging pool to an eighth of the budget");
    reported.heapBudget[1] = VkDeviceSize{2} << 30u;
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory, &reported) == VkDeviceSize{512} << 20u, "a 2 GiB budget does not keep the 512 MiB floor");
    reported.heapBudget[1] = VkDeviceSize{40} << 30u;
    Require(AgcDriver::Graphics::BufferPool::DeviceBudget(memory, &reported) == VkDeviceSize{2} << 30u, "a budget above the heap size raised the staging pool past an eighth of the heap");
}

void meshArgumentTests() {
    using AgcDriver::Graphics::MeshArguments;
    using AgcDriver::Graphics::ResolveMeshArguments;
    AgcDriver::Graphics::Context context{};
    context.meshLimits.maxMeshWorkGroupCount[0] = 1000;
    context.meshLimits.maxMeshWorkGroupCount[1] = 600;
    context.meshLimits.maxMeshWorkGroupTotalCount = 4000;
    const ShaderRecompiler::MeshConfiguration points{1u, 1u, 1u, 1u, 1u, 64u, 1024u, 0u, 4u};
    const ShaderRecompiler::MeshConfiguration triangles{4u, 32u, 96u, 96u, 32u, 128u, 2048u, 0u, 4u};
    const ShaderRecompiler::MeshConfiguration strip{6u, 8u, 10u, 10u, 8u, 64u, 1024u, 0u, 4u};
    const ShaderRecompiler::MeshConfiguration fan{5u, 8u, 10u, 10u, 8u, 64u, 1024u, 0u, 4u};
    const auto same = [](const MeshArguments& a, const MeshArguments& b) { return a.groups == b.groups && a.instances == b.instances && a.layers == b.layers && a.indexCount == b.indexCount && a.firstIndex == b.firstIndex; };
    const auto rules = [&](const ShaderRecompiler::MeshConfiguration& mesh, std::uint32_t indexCount) { return AgcDriver::Graphics::MeshArgumentRulesFor(context, mesh, indexCount); };
    const auto record = [](std::uint32_t count, std::uint32_t instances, std::uint32_t first) { return AgcDriver::Pm4::DrawArguments{count, instances, first, 0, 0}; };
    Require(same(ResolveMeshArguments(record(1, 512, 0), rules(points, 1)), {1, 512, 1, 1, 0}), "one point, 512 instances");
    Require(same(ResolveMeshArguments(record(1, 0, 0), rules(points, 1)), {0, 0, 0, 1, 0}), "no instances draws nothing");
    Require(same(ResolveMeshArguments(record(0, 4, 0), rules(points, 1)), {0, 0, 0, 0, 0}), "no indices draws nothing");
    Require(same(ResolveMeshArguments(record(96, 2, 0), rules(triangles, 300)), {1, 2, 1, 96, 0}), "one full group");
    Require(same(ResolveMeshArguments(record(99, 2, 0), rules(triangles, 300)), {2, 2, 1, 99, 0}), "a partial second group");
    Require(same(ResolveMeshArguments(record(200, 3, 150), rules(triangles, 300)), {2, 3, 1, 150, 150}), "count clamped to the index buffer");
    Require(same(ResolveMeshArguments(record(9, 1, 300), rules(triangles, 300)), {0, 0, 0, 0, 300}), "first index past the index buffer draws nothing");
    Require(same(ResolveMeshArguments(record(2, 1, 0), rules(triangles, 300)), {0, 0, 0, 2, 0}), "no complete triangle draws nothing");
    Require(same(ResolveMeshArguments(record(10, 1, 0), rules(strip, 64)), {1, 1, 1, 10, 0}), "eight strip triangles are one group");
    Require(same(ResolveMeshArguments(record(11, 1, 0), rules(strip, 64)), {2, 1, 1, 11, 0}), "a ninth strip triangle starts a group");
    Require(same(ResolveMeshArguments(record(10, 1, 0), rules(fan, 64)), {1, 1, 1, 10, 0}), "eight fan triangles are one group");
    Require(same(ResolveMeshArguments(record(11, 1, 0), rules(fan, 64)), {2, 1, 1, 11, 0}), "a ninth fan triangle starts a group");
    Require(same(ResolveMeshArguments(record(1, 601, 0), rules(points, 1)), {0, 0, 0, 1, 0}), "instances over the device limit draw nothing");
    Require(same(ResolveMeshArguments(record(96 * 7, 600, 0), rules(triangles, 96 * 7)), {0, 0, 0, 96 * 7, 0}), "groups times instances over the device limit draw nothing");
    Require(same(ResolveMeshArguments(record(96 * 6, 600, 0), rules(triangles, 96 * 6)), {6, 600, 1, 96 * 6, 0}), "groups times instances at the device limit");
    Require(same(ResolveMeshArguments(record(0xffffffffu, 1, 0xfffffff0u), rules(points, 0xffffffffu)), {15, 1, 1, 15, 0xfffffff0u}), "first index near the end of a huge index buffer");
    Require(same(ResolveMeshArguments(record(0xffffffffu, 1, 0), rules(points, 0xffffffffu)), {0, 0, 0, 0xffffffffu, 0}), "groups over the device limit draw nothing");
}

void meshLimitTests() {
    AgcDriver::Graphics::Context context{};
    context.meshLimits.maxMeshWorkGroupInvocations = 128;
    context.meshLimits.maxMeshWorkGroupSize[0] = 96;
    context.meshLimits.maxMeshOutputVertices = 256;
    context.meshLimits.maxMeshOutputPrimitives = 512;
    context.meshLimits.maxMeshSharedMemorySize = 12284;
    ShaderRecompiler::MeshConfiguration mesh{
        .inputPrimitive = 4u,
        .primitivesPerGroup = 32u,
        .verticesPerGroup = 96u,
        .maxVertices = 256u,
        .maxPrimitives = 512u,
        .threadsPerGroup = 128u,
        .ldsSizeDwords = 3071u,
        .provokingVertex = 0u,
        .esgsItemSize = 4u,
    };
    AgcDriver::Graphics::ValidateMeshLimits(context, mesh, 96);
    expectFailure([&] { AgcDriver::Graphics::ValidateMeshLimits(context, mesh, 97); }, "mesh workgroup of 97 invocations exceeds device limits (128 invocations, 96 wide)");
    mesh.maxVertices = 257;
    expectFailure([&] { AgcDriver::Graphics::ValidateMeshLimits(context, mesh, 96); }, "mesh output vertices 257 exceed the device limit 256");
    mesh.maxVertices = 256;
    mesh.maxPrimitives = 513;
    expectFailure([&] { AgcDriver::Graphics::ValidateMeshLimits(context, mesh, 96); }, "mesh output primitives 513 exceed the device limit 512");
    mesh.maxPrimitives = 512;
    mesh.ldsSizeDwords = 3072;
    expectFailure([&] { AgcDriver::Graphics::ValidateMeshLimits(context, mesh, 96); }, "mesh LDS 12288 bytes exceeds the device shared memory limit 12284");
    mesh.ldsSizeDwords = 3071;
    AgcDriver::Graphics::ValidateMeshLimits(context, mesh, 96);
}

void debugBranchTests() {
    for (const auto opcode : {0x17u, 0x18u, 0x19u, 0x1au}) Require(recompilesDebugBranch(opcode), "a conditional debug branch did not recompile");
}

void highestDrawIndexTests() {
    using AgcDriver::Graphics::HighestDrawIndex;
    const auto bytesOf = [](const auto& values) { return std::as_bytes(std::span(values)); };
    const std::array<std::uint16_t, 5> narrow{0, 7, 0xffff, 3, 0xffff};
    Require(HighestDrawIndex(bytesOf(narrow), 2, true) == 7, "a 16-bit restart index counted as a vertex");
    Require(HighestDrawIndex(bytesOf(narrow), 2, false) == 0xffff, "a 16-bit all-ones index without restart was skipped");
    const std::array<std::uint32_t, 4> wide{9, 0xffffffffu, 0xffff, 2};
    Require(HighestDrawIndex(bytesOf(wide), 4, true) == 0xffff, "a 32-bit restart index counted as a vertex, or 0xffff was taken for it");
    Require(HighestDrawIndex(bytesOf(wide), 4, false) == 0xffffffffu, "a 32-bit all-ones index without restart was skipped");
    const std::array<std::uint16_t, 4> restartOnly{0xffff, 0xffff, 0xffff, 0xffff};
    Require(!HighestDrawIndex(bytesOf(restartOnly), 2, true).has_value(), "a draw of only restart indices reached a vertex");
    Require(HighestDrawIndex(bytesOf(restartOnly), 2, false) == 0xffff, "a 16-bit all-ones index without restart was skipped");
    Require(HighestDrawIndex(bytesOf(narrow).first(4), 2, true) == 7 && HighestDrawIndex(bytesOf(narrow).first(2), 2, true) == 0, "the scan read past its index range");
    expectFailure([&] { HighestDrawIndex(bytesOf(narrow), 1, false); }, "unsupported index size");
    Require(!HighestDrawIndex({}, 2, false).has_value() && !HighestDrawIndex({}, 4, true).has_value(), "an empty index range reached a vertex");
    std::vector<std::uint16_t> long16(1027, 0xffff);
    long16[1026] = 0;
    Require(HighestDrawIndex(std::as_bytes(std::span(long16)), 2, true) == 0, "index 0 after many restart indices was not reached");
    long16[513] = 4000;
    Require(HighestDrawIndex(std::as_bytes(std::span(long16)), 2, true) == 4000, "a 16-bit index inside a long range was missed");
    std::vector<std::uint32_t> long32(1029);
    for (std::size_t i = 0; i < long32.size(); ++i) long32[i] = static_cast<std::uint32_t>(i * 7u % 1000u);
    long32[1028] = 0x80000001u;
    Require(HighestDrawIndex(std::as_bytes(std::span(long32)), 4, true) == 0x80000001u, "a 32-bit index above 2^31 in the tail was missed");
    long32[1028] = 0xffffffffu;
    Require(HighestDrawIndex(std::as_bytes(std::span(long32)), 4, true) == 999u && HighestDrawIndex(std::as_bytes(std::span(long32)), 4, false) == 0xffffffffu, "a 32-bit restart index at the end of a long range");
    std::vector<std::byte> shifted(2 * 1000 + 3);
    for (std::size_t i = 0; i < 1000; ++i) {
        const auto value = static_cast<std::uint16_t>(i == 777 ? 0x8001u : i % 300u);
        std::memcpy(shifted.data() + 1 + 2 * i, &value, sizeof(value));
    }
    Require(HighestDrawIndex(std::span<const std::byte>(shifted).subspan(1, 2 * 1000 + 1), 2, false) == 0x8001u, "an unaligned 16-bit range with a trailing byte");
}

void storeAtFlipTests() {
    using AgcDriver::Graphics::StorageTexture;
    Require(!StorageTexture::StoreAtFlipRequested(nullptr), "an unset APS5_STORE_AT_FLIP stored at each flip");
    Require(StorageTexture::StoreAtFlipRequested("1"), "APS5_STORE_AT_FLIP=1 did not store at each flip");
    for (const char* value : {"0", "", "true", "11"}) expectFailure([&] { StorageTexture::StoreAtFlipRequested(value); }, "expected 1");
}

void storeAtLabelTests() {
    using AgcDriver::Graphics::StorageTexture;
    Require(!StorageTexture::StoreAtLabelRequested(nullptr), "an unset APS5_STORE_AT_LABEL stored at each label");
    Require(StorageTexture::StoreAtLabelRequested("1"), "APS5_STORE_AT_LABEL=1 did not store at each label");
    for (const char* value : {"0", "", "true", "11"}) expectFailure([&] { StorageTexture::StoreAtLabelRequested(value); }, "expected 1");
}

void nullVertexDescriptorTests() {
    using AgcDriver::Graphics::DecodeVertexFormat;
    constexpr std::array formats{VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};
    ShaderRecompiler::VertexAttribute attribute{};
    attribute.formatComponents = 1;
    for (std::uint32_t components = 1; components <= 4; ++components) {
        attribute.components = components;
        const auto format = DecodeVertexFormat(attribute);
        Require(format.format == formats[components - 1] && format.bytes == components * 4u && std::string_view(format.scalar) == "f32", "a null V# did not read as zero floats of the attribute's width");
        Require(AgcDriver::Graphics::VertexBufferReadSize(attribute, 100000, 3, 2) == format.bytes && AgcDriver::Graphics::VertexBufferExtent(attribute) == format.bytes, "a null V# was sized as a guest range");
    }
    attribute.resource.fields = {0x1000, 0, 0, 0};
    expectFailure([&] { DecodeVertexFormat(attribute); }, "unsupported vertex format");
}

void vertexCopyTests() {
    using AgcDriver::Graphics::PlanVertexCopies;
    using AgcDriver::Graphics::VertexFetch;
    {
        const std::array<VertexFetch, 3> fetches{{{0x1018, 0x1018 + 32 * 9 + 8, 32, 0, 4}, {0x1000, 0x1000 + 32 * 9 + 12, 32, 0, 4}, {0x100c, 0x100c + 32 * 9 + 12, 32, 0, 4}}};
        const auto plan = PlanVertexCopies(fetches);
        Require(plan.copies.size() == 1 && plan.copies[0].first == 0x1000 && plan.copies[0].second == 0x1018 + 32 * 9 + 8, "interleaved attributes were not copied as one union");
        Require(plan.copyOf == std::vector<std::size_t>{0, 0, 0} && plan.offsets == std::vector<std::uint64_t>{0x18, 0, 0xc}, "interleaved attribute offsets are wrong");
        Require(plan.alignments == std::vector<std::uint32_t>{4}, "the union's alignment is not its attributes'");
    }
    {
        const std::array<VertexFetch, 2> fetches{{{0x4000, 0x4100, 16, 0, 2}, {0x4004, 0x4104, 16, 0, 4}}};
        const auto plan = PlanVertexCopies(fetches);
        Require(plan.copies.size() == 1 && plan.alignments == std::vector<std::uint32_t>{4}, "a union did not take its strictest attribute alignment");
    }
    {
        const std::array<VertexFetch, 6> fetches{{
            {0x2000, 0x2100, 32, 0, 4},
            {0x2004, 0x2100, 16, 0, 4},
            {0x2020, 0x2120, 32, 0, 4},
            {0x2002, 0x2102, 32, 0, 4},
            {0x2008, 0x2108, 32, 1, 4},
            {0x2000, 0x2010, 0, 0, 4},
        }};
        const auto plan = PlanVertexCopies(fetches);
        Require(plan.copies.size() == 6, "fetches of other records, strides, rates or alignments shared a copy");
        for (std::size_t i = 0; i < fetches.size(); ++i) {
            const auto& copy = plan.copies[plan.copyOf[i]];
            Require(plan.offsets[i] == 0 && copy.first == fetches[i].begin && copy.second == fetches[i].end, "a lone fetch was not copied exactly");
        }
    }
    {
        const std::array<VertexFetch, 2> fetches{{{0x3000, 0x3100, 24, 0, 2}, {0x3002, 0x3102, 24, 0, 2}}};
        const auto plan = PlanVertexCopies(fetches);
        Require(plan.copies.size() == 1 && plan.copies[0].second == 0x3102 && plan.offsets[1] == 2, "aligned 16-bit attributes of one record were not merged");
    }
    {
        const std::array<VertexFetch, 1> empty{{{0x4000, 0x4000, 16, 0, 4}}};
        expectFailure([&] { PlanVertexCopies(empty); }, "empty vertex fetch");
    }
}

void vertexZeroFillTests() {
    using AgcDriver::Graphics::CopyZeroPaddedDrawInput;
    using AgcDriver::Graphics::VertexBufferReadSize;
    constexpr std::uint32_t stride = 24;
    constexpr std::uint32_t records = 3;
    const ShaderRecompiler::VertexAttribute attribute{0, 4, {{0x1000u, stride << 16u, records, 77u << 12u}}, 0};
    const auto extent = VertexBufferReadSize(attribute, records, 1);
    const auto valid = static_cast<std::size_t>(stride) * records;
    std::vector<std::byte> guest(extent);
    std::fill(guest.begin(), guest.begin() + static_cast<std::ptrdiff_t>(valid), std::byte{0x7a});
    std::fill(guest.begin() + static_cast<std::ptrdiff_t>(valid), guest.end(), std::byte{0x55});
    const auto copy = CopyZeroPaddedDrawInput(mockContext(), reinterpret_cast<std::uint64_t>(guest.data()), extent, valid);
    const auto bytes = copy.buffer->Bytes();
    Require(bytes.size() == extent, "a zero-padded vertex fetch changed its size");
    Require(std::all_of(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(valid), [](std::byte value) { return value == std::byte{0x7a}; }), "a vertex fetch did not copy the bytes within the record count");
    Require(std::all_of(bytes.begin() + static_cast<std::ptrdiff_t>(valid), bytes.end(), [](std::byte value) { return value == std::byte{0}; }), "a vertex fetch past the record count did not read as zeros");
}

void vertexOverhangTests() {
    using AgcDriver::Graphics::CopyZeroPaddedDrawInput;
    constexpr std::size_t page = 65536;
    constexpr std::size_t stride = 44;
    constexpr std::size_t overhang = 8;
#ifdef _WIN32
    auto* block = static_cast<std::byte*>(VirtualAlloc(nullptr, page * 2, MEM_RESERVE, PAGE_NOACCESS));
    Require(block != nullptr && VirtualAlloc(block, page, MEM_COMMIT, PAGE_READWRITE) != nullptr, "cannot reserve the guest pages of the overhang test");
#else
    void* mapped = mmap(nullptr, page * 2, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    Require(mapped != MAP_FAILED && mprotect(mapped, page, PROT_READ | PROT_WRITE) == 0, "cannot reserve the guest pages of the overhang test");
    auto* block = static_cast<std::byte*>(mapped);
#endif
    constexpr std::size_t offset = 0x650;
    const auto readable = page - offset;
    std::fill(block + offset, block + page, std::byte{0x7a});
    const auto address = reinterpret_cast<std::uint64_t>(block) + offset;
    {
        const auto copy = CopyZeroPaddedDrawInput(mockContext(), address, readable + overhang, readable + overhang, stride);
        const auto bytes = copy.buffer->Bytes();
        Require(bytes.size() == readable + overhang, "an overhanging vertex fetch changed its size");
        Require(std::all_of(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(readable), [](std::byte value) { return value == std::byte{0x7a}; }), "an overhanging vertex fetch did not copy the readable bytes");
        Require(std::all_of(bytes.begin() + static_cast<std::ptrdiff_t>(readable), bytes.end(), [](std::byte value) { return value == std::byte{0}; }), "an overhanging vertex fetch did not zero its last record");
    }
    expectFailure([&] { CopyZeroPaddedDrawInput(mockContext(), address, readable + stride, readable + stride, stride); }, "is not readable");
    expectFailure([&] { CopyZeroPaddedDrawInput(mockContext(), address, readable + overhang, readable + overhang, 0); }, "is not readable");
#ifdef _WIN32
    VirtualFree(block, 0, MEM_RELEASE);
#else
    munmap(block, page * 2);
#endif
}

void vertexZeroFillMergeTests() {
    using AgcDriver::Graphics::PlanVertexCopies;
    using AgcDriver::Graphics::SoloZeroPaddedFetchIndices;
    using AgcDriver::Graphics::VertexFetch;
    const std::vector<VertexFetch> fetches{{0x1000u, 0x1000u + 88u, 24u, 0u, 4u}, {0x1010u, 0x1010u + 80u, 24u, 0u, 4u}};
    Require(PlanVertexCopies(fetches).copies.size() == 1, "interleaved attributes share one vertex copy");
    Require(SoloZeroPaddedFetchIndices(fetches, {88u, 80u}).empty(), "fully valid fetches share the straight copy");
    const auto solo = SoloZeroPaddedFetchIndices(fetches, {88u, 72u});
    Require(solo.size() == 1 && solo[0] == 1, "a fetch past the records is copied alone");
}

void parallelCompareTests() {
    using AgcDriver::Graphics::CompareSpan;
    using AgcDriver::Graphics::CompareSpansFrom;
    constexpr std::size_t block = 65536;
    constexpr std::size_t count = 48;
    std::vector<std::byte> first(block * count + 100, std::byte{7});
    auto second = first;
    const std::array<std::size_t, 4> changedAt{3 * block + 17, 20 * block, 33 * block + block - 1, 47 * block + 120};
    for (const auto at : changedAt) second[at] = std::byte{9};
    std::vector<CompareSpan> spans;
    for (std::size_t k = 0; k < count; ++k) {
        const auto length = k + 1 == count ? block + 100 : block;
        spans.push_back({first.data() + k * block, second.data() + k * block, length});
    }
    const auto expected = [&](std::size_t k) {
        return std::none_of(changedAt.begin(), changedAt.end(), [&](std::size_t at) { return at >= k * block && at < k * block + spans[k].bytes; });
    };
    for (const std::size_t threshold : {std::size_t{0}, std::size_t{1} << 40u}) {
        for (int round = 0; round < 20; ++round) {
            std::vector<std::uint8_t> equal(count, 2);
            CompareSpansFrom(spans, equal, threshold);
            for (std::size_t k = 0; k < count; ++k) Require((equal[k] == 1) == expected(k) && equal[k] <= 1, "parallel compare misreported a span");
        }
    }
    std::vector<std::uint8_t> left(count, 2), right(count, 2);
    std::thread other([&] {
        for (int round = 0; round < 50; ++round) CompareSpansFrom(spans, right, 0);
    });
    for (int round = 0; round < 50; ++round) CompareSpansFrom(spans, left, 0);
    other.join();
    for (std::size_t k = 0; k < count; ++k) Require((left[k] == 1) == expected(k) && (right[k] == 1) == expected(k), "concurrent parallel compares misreported a span");
    std::vector<std::uint8_t> none;
    CompareSpansFrom({}, none, 0);
}

void ResourceContentKeyTests(bool benchmark) {
    using AgcDriver::Graphics::ShaderResources;
    using ShaderRecompiler::DescriptorKind;
    using ShaderRecompiler::DescriptorRole;
    for (const auto count : {0u, 1u, 31u, 32u, 33u, 16384u}) {
        ShaderRecompiler::RecompileResult program;
        ShaderRecompiler::DescriptorBinding images{};
        images.kind = DescriptorKind::SampledImage;
        images.role = DescriptorRole::GuestImages;
        images.count = count;
        images.guestDescriptor.resize(static_cast<std::size_t>(count) * 8u);
        for (std::size_t i = 0; i < images.guestDescriptor.size(); ++i) images.guestDescriptor[i] = static_cast<std::uint32_t>(i * 1664525u + 1013904223u);
        for (auto* bits : {&images.imageWritten, &images.samplerDepthCompare, &images.imageDepthCompare, &images.imageAtomic, &images.samplerUnnormalized, &images.samplerIntegerBorder, &images.imageUnnormalized, &images.imageUnnormalizedBaseLevel, &images.bufferWritten}) {
            bits->resize(count);
            for (std::size_t i = 0; i < bits->size(); ++i) (*bits)[i] = i % 3u == 0;
        }
        images.imageSamplers.resize(count, 7u);
        images.imageShapes.resize(count, ShaderRecompiler::DescriptorImageShape::Image2D);
        program.bindings.push_back(std::move(images));
        ShaderRecompiler::DescriptorBinding data{};
        data.kind = DescriptorKind::StorageBuffer;
        data.role = DescriptorRole::ShaderData;
        data.count = 1;
        data.guestDescriptor.resize(257u, 0x12345678u);
        program.bindings.push_back(std::move(data));
        ShaderRecompiler::DescriptorBinding buffers{};
        buffers.kind = DescriptorKind::StorageBuffer;
        buffers.role = DescriptorRole::GuestBuffers;
        buffers.count = 1;
        buffers.guestDescriptor = {0x1000u, 0x10000u, 4u, 0x1234u};
        buffers.bufferWritten = {false};
        program.bindings.push_back(std::move(buffers));
        const AgcDriver::Graphics::CompiledShader shader{ShaderRecompiler::ShaderStage::Compute, &program, 0};
        for (const bool dataWords : {false, true}) for (const bool movable : {false, true}) {
            const auto original = ShaderResources::ContentKey(shader, dataWords, movable);
            program.bindings[1].guestDescriptor.back() ^= 1u;
            Require((ShaderResources::ContentKey(shader, dataWords, movable) != original) == (dataWords && !movable), "content key changed its data-word policy");
            program.bindings[1].guestDescriptor.back() ^= 1u;
            program.bindings[2].guestDescriptor[0] += 0x1000u;
            Require((ShaderResources::ContentKey(shader, dataWords, movable) != original) == !movable, "content key changed its movable-buffer policy");
            program.bindings[2].guestDescriptor[0] -= 0x1000u;
            program.bindings[2].bufferWritten[0] = true;
            Require(ShaderResources::ContentKey(shader, dataWords, movable) != original, "content key ignored the written-buffer mask");
            program.bindings[2].bufferWritten[0] = false;
            if (count != 0) {
                for (auto* bits : {&program.bindings[0].imageWritten, &program.bindings[0].samplerDepthCompare, &program.bindings[0].imageDepthCompare, &program.bindings[0].imageAtomic, &program.bindings[0].samplerUnnormalized, &program.bindings[0].samplerIntegerBorder, &program.bindings[0].imageUnnormalized, &program.bindings[0].imageUnnormalizedBaseLevel}) {
                    bits->back().flip();
                    Require(ShaderResources::ContentKey(shader, dataWords, movable) != original, "content key ignored the last packed flag");
                    bits->back().flip();
                }
                ++program.bindings[0].imageSamplers.back();
                Require(ShaderResources::ContentKey(shader, dataWords, movable) != original, "content key ignored the last sampler association");
                --program.bindings[0].imageSamplers.back();
                program.bindings[0].imageShapes.back() = ShaderRecompiler::DescriptorImageShape::Image3D;
                Require(ShaderResources::ContentKey(shader, dataWords, movable) != original, "content key ignored the last image shape");
                program.bindings[0].imageShapes.back() = ShaderRecompiler::DescriptorImageShape::Image2D;
            }
            Require(original.capacity() == original.size(), "content key reserved a different word count than it serialized");
            std::uint64_t hash = 14695981039346656037ull;
            for (const auto word : original) hash = (hash ^ word) * 1099511628211ull;
            std::cout << "content-key count=" << count << " data=" << dataWords << " movable=" << movable << " words=" << original.size() << " capacity=" << original.capacity() << " hash=" << hash;
            if (benchmark) {
                constexpr std::size_t repeats = 256;
                std::uint64_t checksum = 0;
                const auto begin = std::chrono::steady_clock::now();
                for (std::size_t i = 0; i < repeats; ++i) {
                    const auto key = ShaderResources::ContentKey(shader, dataWords, movable);
                    checksum += key.size() + key.back();
                }
                const auto elapsed = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - begin).count();
                std::cout << " us/key=" << elapsed / repeats << " checksum=" << checksum;
            }
            std::cout << '\n';
        }
    }
}

// The pipeline journal without a GPU: the recipe codec and the store key agree with each other and
// with the canonical state, identities follow the module and the specialization, the journal
// records each pipeline once, and a replay into the recording context (the lavapipe test replays
// into a second device) rebuilds the recorded graphics pipeline so the same configuration hits the
// store, hands the compute result to its hook, recovers a tail torn inside a record's padding and
// stops when cancelled.
void pipelineStoreLazyBufferPoolTests() {
    using namespace AgcDriver::Graphics;
    mock = MockVulkan{};
    auto context = mockContext();
    context.device = makeHandle<VkDevice>();
    context.graphicsPipelineLibrary = true;
    context.pipelineLinkTimeOptimization = false;
    context.limits.framebufferNoAttachmentsSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    ShaderRecompiler::RecompileResult vertex, fragment;
    vertex.spirv = makeModule({});
    fragment.spirv = makeModule({.fragment = true});
    vertex.variantId = 201;
    fragment.variantId = 202;
    const std::array<CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, PipelinePushSlotBytes}}};
    std::array<std::uint32_t, 1> mainKey{31}, fragmentKey{32};
    const PipelineSetLayouts resources{makeHandle<VkDescriptorSetLayout>(), makeHandle<VkDescriptorSetLayout>(), true, mainKey, fragmentKey};
    Require(context.bufferPool == nullptr, "the lazy buffer pool fixture already owns a buffer pool");
    auto first = CachedPipeline(context, State{}, VertexInputLayout{}, resources, shaders);
    const auto created = mock.pipelineCreateCount;
    static_cast<void>(GetBufferPool(context));
    auto again = CachedPipeline(context, State{}, VertexInputLayout{}, resources, shaders);
    Require(again == first && mock.pipelineCreateCount == created, "creating the buffer pool dropped a cached pipeline of the live device");
    ClearCachedPipelines(context.device);
    again.reset();
    first.reset();
    context.bufferPool.reset();
    Require(mock.live == 0, "lazy buffer pool pipeline tests leaked Vulkan objects");
}

// The pipeline store's bound without a GPU: 1536 configurations cycled through the default 1024
// entries keep coming back after their eviction, so the bound doubles and a later pass finds every
// pipeline resident; a bound set with APS5_PIPELINE_STORE_ENTRIES never moves.
void pipelineStoreGrowthTests(bool fixed) {
    using namespace AgcDriver::Graphics;
    mock = MockVulkan{};
    auto context = mockContext();
    context.device = makeHandle<VkDevice>();
    context.graphicsPipelineLibrary = true;
    context.pipelineLinkTimeOptimization = false;
    context.limits.framebufferNoAttachmentsSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    ShaderRecompiler::RecompileResult vertex, fragment;
    vertex.spirv = makeModule({});
    fragment.spirv = makeModule({.fragment = true});
    vertex.variantId = 301;
    fragment.variantId = 302;
    const std::array<CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, PipelinePushSlotBytes}}};
    std::array<std::uint32_t, 1> mainKey{41}, fragmentKey{42};
    const PipelineSetLayouts resources{makeHandle<VkDescriptorSetLayout>(), makeHandle<VkDescriptorSetLayout>(), true, mainKey, fragmentKey};
    constexpr std::uint32_t configurations = 1536;
    const auto pass = [&](std::uint32_t count) {
        std::uint32_t built = 0;
        for (std::uint32_t configuration = 0; configuration < count; ++configuration) {
            VertexInputLayout input;
            input.bindings.push_back({0, 4u * (configuration + 1u), VK_VERTEX_INPUT_RATE_VERTEX});
            bool created = false;
            static_cast<void>(CachedPipeline(context, State{}, input, resources, shaders, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, &created));
            built += created ? 1u : 0u;
        }
        return built;
    };
    std::uint32_t lastBuilt = 0;
    for (int round = 0; round < 6; ++round) lastBuilt = pass(configurations);
    const auto grown = PipelineStoreTotals();
    if (fixed) {
        Require(grown.bound == 1024, "a bound set with APS5_PIPELINE_STORE_ENTRIES changed");
        Require(lastBuilt == configurations, "a fixed 1024-entry bound kept pipelines of a 1536-configuration cycle");
    } else {
        Require(grown.bound == 2048, "the bound did not grow to 2048 while evicted pipelines kept coming back");
        Require(lastBuilt == 0, "a pass over 1536 configurations still rebuilt pipelines after the bound grew");
    }
    ClearCachedPipelines(context.device);
    Require(mock.live == 0, "pipeline store growth tests leaked Vulkan objects");
}

// A journal longer than the store's starting bound: 1032 recorded pipelines replayed into an empty
// store all stay resident (the replay grows the bound instead of dropping what does not fit).
void pipelineStoreReplayGrowthTests() {
    using namespace AgcDriver::Graphics;
    const auto root = std::filesystem::temp_directory_path() / ("aps5-pipeline-store-replay-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(root);
    struct Cleanup {
        std::filesystem::path directory;
        ~Cleanup() {
            ShaderRecompiler::ShaderDiskCache::Shutdown();
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
    } cleanup{root};
#ifdef _WIN32
    _putenv_s("ANYPS5_NO_SHADER_CACHE", "0");
    _wputenv_s(L"ANYPS5_SHADER_CACHE_DIR", root.c_str());
    _putenv_s("APS5_PIPELINE_JOURNAL_THREADS", "1");
#else
    setenv("ANYPS5_NO_SHADER_CACHE", "0", 1);
    setenv("ANYPS5_SHADER_CACHE_DIR", root.string().c_str(), 1);
    setenv("APS5_PIPELINE_JOURNAL_THREADS", "1", 1);
#endif
    Require(PipelineJournalEnabled(), "the journal is disabled with a shader cache directory set");
    mock = MockVulkan{};
    auto context = mockContext();
    context.device = makeHandle<VkDevice>();
    context.graphicsPipelineLibrary = true;
    context.pipelineLinkTimeOptimization = false;
    context.limits.framebufferNoAttachmentsSampleCounts = VK_SAMPLE_COUNT_1_BIT;
    ShaderRecompiler::RecompileResult vertex, fragment;
    vertex.spirv = makeModule({});
    fragment.spirv = makeModule({.fragment = true});
    vertex.variantId = 401;
    fragment.variantId = 402;
    const std::array<CompiledShader, 2> shaders{{{ShaderRecompiler::ShaderStage::Vertex, &vertex, 0}, {ShaderRecompiler::ShaderStage::Fragment, &fragment, PipelinePushSlotBytes}}};
    auto cache = std::make_unique<DescriptorCache>(context);
    context.descriptorCache = cache.get();
    const auto layouts = DescribeSetLayouts(context, shaders);
    const auto resources = layouts.View();
    constexpr std::uint64_t fingerprint = 0x5eed'0000'0000'0002ull;
    {
        auto absent = StartPipelineJournalReplay(context, fingerprint, {});
        Require(absent != nullptr, "the first replay did not start");
        absent->Wait();
    }
    constexpr std::uint32_t configurations = 1032;
    for (std::uint32_t configuration = 0; configuration < configurations; ++configuration) {
        VertexInputLayout input;
        input.bindings.push_back({0, 4u * (configuration + 1u), VK_VERTEX_INPUT_RATE_VERTEX});
        static_cast<void>(CachedPipeline(context, State{}, input, resources, shaders));
    }
    FlushPipelineJournal();
    ClearCachedPipelines(context.device);
    Require(PipelineStoreTotals().cached == 0, "clearing the device left pipelines in the store");
    auto replay = StartPipelineJournalReplay(context, fingerprint, {});
    Require(replay != nullptr, "the replay did not start");
    replay->Wait();
    const auto totals = PipelineStoreTotals();
    Require(totals.cached == configurations, "a replay of " + std::to_string(configurations) + " recorded pipelines kept " + std::to_string(totals.cached) + " in the store (bound " + std::to_string(totals.bound) + ")");
    ClearCachedPipelines(context.device);
    context.descriptorCache = nullptr;
    cache.reset();
    Require(mock.live == 0, "pipeline store replay growth tests leaked Vulkan objects");
}

void pipelineJournalTests(bool longPath = false, bool extended = false) {
    using namespace AgcDriver::Graphics;
    using Stage = ShaderRecompiler::ShaderStage;
    const auto root = std::filesystem::temp_directory_path() / ("aps5-pipeline-journal-mock-" + std::to_string(std::random_device{}()));
    auto directory = root;
    if (longPath) {
        Require(root.native().size() < 180, "the temporary root is too long for the journal path fixture");
        directory /= std::filesystem::path(u8"cache-\u00e4\u6f22");
        directory /= std::string(229 - directory.native().size(), 'd');
    }
    std::filesystem::create_directories(directory);
    struct Cleanup {
        std::filesystem::path directory;
        std::filesystem::path journal;
        ~Cleanup() {
            ShaderRecompiler::ShaderDiskCache::Shutdown();
            std::error_code error;
            if (!journal.empty()) {
                std::filesystem::remove(journal, error);
                auto lock = journal;
                lock += ".lock";
                std::filesystem::remove(lock, error);
            }
            std::filesystem::remove_all(directory, error);
        }
    } cleanup{root, {}};
#ifdef _WIN32
    if (extended) directory = std::filesystem::path(L"\\\\?\\" + directory.make_preferred().native());
    _putenv_s("ANYPS5_NO_SHADER_CACHE", "0");
    _wputenv_s(L"ANYPS5_SHADER_CACHE_DIR", directory.c_str());
    _putenv_s("APS5_PIPELINE_JOURNAL_THREADS", "2");
#else
    setenv("ANYPS5_NO_SHADER_CACHE", "0", 1);
    setenv("ANYPS5_SHADER_CACHE_DIR", directory.string().c_str(), 1);
    setenv("APS5_PIPELINE_JOURNAL_THREADS", "2", 1);
#endif
    Require(PipelineJournalEnabled(), "the journal is disabled with a shader cache directory set");
    auto journalFile = PipelineJournalPath();
#ifdef _WIN32
    std::error_code pathError;
    journalFile = ShaderRecompiler::NativeCachePath_nid_no_patch(journalFile, pathError);
    Require(!pathError, "cannot normalize the journal fixture path");
#endif
    cleanup.journal = journalFile;
    if (longPath) Require(journalFile.native().size() > 260, "the journal fixture did not cross MAX_PATH");
    const auto module = [](std::uint32_t salt) {
        ShaderRecompiler::RecompileResult result;
        result.spirv = std::vector<std::uint32_t>{0x07230203u, 0x00010000u, 0u, 8u, 0u, 0x00020011u, 1u, 0x00020011u, 2u + salt};
        result.runtimeAbiVersion = ShaderRecompiler::RuntimeAbi::Version;
        return result;
    };
    {
        auto first = module(1);
        const auto& constFirst = first;
        auto copy = first;
        const auto& constCopy = copy;
        Require(!constFirst.Identity().Empty() && constFirst.Identity() == constCopy.Identity() && constFirst.spirv.data() == constCopy.spirv.data(), "a copied result did not share its module and identity");
        copy.spirv[8] = 99;
        Require(!(constCopy.Identity() == constFirst.Identity()) && constCopy.spirv.data() != constFirst.spirv.data() && constFirst.Identity() == module(1).Identity(), "writing a copied module did not give it an identity of its own");
        auto specialized = module(1);
        specialized.specialization.push_back({3, 4});
        Require(!(specialized.Identity() == constFirst.Identity()), "specialization constants did not enter the identity");
        auto subgroup = module(1);
        subgroup.hostSubgroupSize = 64;
        Require(!(subgroup.Identity() == constFirst.Identity()), "the host subgroup size did not enter the identity");
        Require(ShaderRecompiler::RecompileResult{}.Identity().Empty(), "a result without a module has an identity");
    }
    mock = MockVulkan{};
    auto context = mockContext();
    context.device = makeHandle<VkDevice>();
    context.graphicsPipelineLibrary = true;
    context.tessellationShader = true;
    context.limits.maxTessellationPatchSize = 32;
    context.limits.maxColorAttachments = 8;
    context.limits.maxPushConstantsSize = 256;
    context.limits.maxVertexInputBindings = 16;
    context.limits.maxVertexInputAttributes = 16;
    context.limits.maxFramebufferWidth = 4096;
    context.limits.maxFramebufferHeight = 4096;
    context.limits.maxFramebufferLayers = 1;
    std::uint64_t replayedComputeIdentityLow = 0;
    {
        DescriptorCache cache(context);
        context.descriptorCache = &cache;
        auto vertex = module(1);
        vertex.bindings.push_back(makeBinding(Role::GuestBuffers, 0, 1, std::vector<std::uint32_t>(4)));
        auto fragment = module(2);
        fragment.pushConstants.resize(8);
        fragment.bindings.push_back(makeBinding(Role::ShaderData, 0, 1, std::vector<std::uint32_t>(4)));
        fragment.bindings.back().descriptorSet = ShaderRecompiler::RuntimeAbi::FragmentDescriptorSet;
        const std::array<CompiledShader, 2> shaders{{{Stage::Vertex, &vertex, 0}, {Stage::Fragment, &fragment, PipelinePushSlotBytes}}};
        State state{};
        state.stages = {ShaderPath::Vertex, 7u, 64u, 32u, std::nullopt, std::nullopt};
        state.stages.tessellation = ShaderRecompiler::TessellationConfiguration{3, 3, 2, 1, 4, 128, 16, true};
        ColorTarget color{};
        color.address = 0x1000;
        color.extent = {64, 32};
        color.format = VK_FORMAT_R8G8B8A8_UNORM;
        color.bytes = 64 * 32 * 4;
        color.componentMapping = 0xec;
        state.colors = {color, color};
        state.colors[1].format = VK_FORMAT_B8G8R8A8_UNORM;
        state.colors[1].exportIndex = 1;
        state.color = color;
        state.hasColorTarget = true;
        VkPipelineColorBlendAttachmentState blend{};
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorWriteMask = 0xf;
        state.blends = {blend, blend};
        state.blend = blend;
        state.blendConstants = {0.25f, 0.5f, 0.75f, 1.0f};
        DepthTarget depth{};
        depth.address = 0x2000;
        depth.format = VK_FORMAT_D32_SFLOAT;
        depth.samples = 1;
        state.depth = depth;
        state.depthTest = true;
        state.depthWrite = true;
        state.depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;
        state.stencilTest = true;
        state.stencilFront = {VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 0xff, 0xff, 1};
        state.stencilBack = state.stencilFront;
        state.renderExtent = {64, 32};
        state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        state.viewport = {0, 32, 64, -32, 0, 1};
        state.scissor = {{0, 0}, {64, 32}};
        state.cullMode = VK_CULL_MODE_BACK_BIT;
        state.frontFace = VK_FRONT_FACE_CLOCKWISE;
        state.samples = 1;
        const VertexInputLayout input{{{0, 16, VK_VERTEX_INPUT_RATE_VERTEX}}, {{0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0}}};
        const auto layouts = DescribeSetLayouts(context, shaders);
        Require(layouts.layouts.split, "a fragment stage with the fragment set did not split the set layouts");
        const auto recipe = MakeGraphicsPipelineRecipe(state, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(recipe.stages.size() == 2 && recipe.stages[0].identity == vertex.Identity() && recipe.stages[1].identity == fragment.Identity() && !recipe.stages[0].generated && recipe.stages[1].pushConstantOffset == PipelinePushSlotBytes && recipe.splitSets, "the recipe does not name its stages");
        Require(recipe.state.colors.size() == 2 && recipe.state.colors[0].address == 0 && recipe.state.colors[0].componentMapping == 0xec && recipe.state.depth && recipe.state.depth->address == 0 && recipe.state.viewport.width == 0 && recipe.state.stages.registerValue == 0 && recipe.state.stages.tessellation && recipe.state.stages.tessellation->breakAtInstance, "the canonical state kept or dropped the wrong fields");
        const auto bytes = EncodeGraphicsPipelineRecipe(recipe);
        GraphicsPipelineRecipe decoded;
        Require(DecodeGraphicsPipelineRecipe(bytes, decoded), "the recipe does not decode");
        Require(EncodeGraphicsPipelineRecipe(decoded) == bytes, "the recipe does not round-trip byte for byte");
        for (std::size_t length = 0; length < bytes.size(); length += 7) {
            GraphicsPipelineRecipe scratch;
            Require(!DecodeGraphicsPipelineRecipe(std::span(bytes).first(length), scratch), "a truncated recipe decoded");
        }
        std::vector<std::byte> original, canonical, replayed, moved, mapped;
        BuildPipelineStoreKey(original, context, state, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        BuildPipelineStoreKey(canonical, context, recipe.state, recipe.vertexInput, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        const PipelineSetLayouts decodedLayouts{layouts.layouts.main, layouts.layouts.fragment, decoded.splitSets, decoded.mainLayoutKey, decoded.fragmentLayoutKey};
        BuildPipelineStoreKey(replayed, context, decoded.state, decoded.vertexInput, decodedLayouts, shaders, decoded.attachmentLayout);
        Require(!original.empty() && original == canonical && original == replayed, "the canonical or the decoded state keys another pipeline");
        auto fmaskState = state;
        fmaskState.samples = 8;
        fmaskState.color.samples = 8;
        fmaskState.color.fmaskAddress = 0x30000;
        fmaskState.color.fmaskPipeBankXor = 0x1200;
        for (auto& target : fmaskState.colors) {
            target.samples = 8;
            target.fmaskAddress = 0x40000;
            target.fmaskPipeBankXor = 0x2300;
        }
        const auto fmaskRecipe = MakeGraphicsPipelineRecipe(fmaskState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(fmaskRecipe.state.color.fmaskAddress == 0 && fmaskRecipe.state.color.fmaskPipeBankXor == 0, "the canonical state kept the primary target's FMASK location");
        for (const auto& canonicalColor : fmaskRecipe.state.colors) {
            Require(canonicalColor.fmaskAddress == 0 && canonicalColor.fmaskPipeBankXor == 0, "the canonical state kept an attachment's FMASK location");
        }
        const auto fmaskBytes = EncodeGraphicsPipelineRecipe(fmaskRecipe);
        GraphicsPipelineRecipe fmaskDecoded;
        Require(DecodeGraphicsPipelineRecipe(fmaskBytes, fmaskDecoded) && EncodeGraphicsPipelineRecipe(fmaskDecoded) == fmaskBytes, "the FMASK pipeline recipe did not round-trip byte for byte");
        std::vector<std::byte> fmaskKey, fmaskReplayedKey;
        BuildPipelineStoreKey(fmaskKey, context, fmaskState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        BuildPipelineStoreKey(fmaskReplayedKey, context, fmaskDecoded.state, fmaskDecoded.vertexInput, layouts.View(), shaders, fmaskDecoded.attachmentLayout);
        Require(fmaskKey != original && fmaskKey == fmaskReplayedKey, "the decoded FMASK state keys another pipeline");
        auto relocatedFmask = fmaskState;
        relocatedFmask.color.fmaskAddress += 0x10000;
        relocatedFmask.color.fmaskPipeBankXor ^= 0x100;
        for (auto& target : relocatedFmask.colors) {
            target.fmaskAddress += 0x20000;
            target.fmaskPipeBankXor ^= 0x200;
        }
        std::vector<std::byte> relocatedFmaskKey;
        BuildPipelineStoreKey(relocatedFmaskKey, context, relocatedFmask, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(relocatedFmaskKey == fmaskKey, "the FMASK resource location changed the pipeline key");
        Require(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(relocatedFmask, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)) == fmaskBytes, "the FMASK resource location changed the journal recipe");
        auto gridState = state;
        gridState.samples = 2;
        gridState.customSampleLocations = true;
        gridState.sampleLocationGrid = true;
        gridState.perSampleShading = true;
        gridState.sampleLocations[31] = 0x5a;
        GraphicsPipelineRecipe gridDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(gridState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), gridDecoded), "the sample grid recipe does not decode");
        Require(gridDecoded.state.sampleLocationGrid && gridDecoded.state.perSampleShading && gridDecoded.state.sampleLocations[31] == 0x5a, "the recipe dropped the sample grid, per-sample shading or the last sample location");
        std::vector<std::byte> grid, gridReplayed;
        BuildPipelineStoreKey(grid, context, gridState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        BuildPipelineStoreKey(gridReplayed, context, gridDecoded.state, gridDecoded.vertexInput, layouts.View(), shaders, gridDecoded.attachmentLayout);
        Require(grid != original && grid == gridReplayed, "the decoded sample grid state keys another pipeline");
        auto lineState = state;
        lineState.stages.mesh = ShaderRecompiler::MeshConfiguration{1, 2, 2, 4, 4, 32, 64, 1, 4, 2};
        GraphicsPipelineRecipe lineDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(lineState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), lineDecoded), "the line strip mesh recipe does not decode");
        Require(lineDecoded.state.stages.mesh && lineDecoded.state.stages.mesh->outputPrimitiveSize == 2u && lineDecoded.state.stages.mesh->esgsItemSize == 4u, "the recipe dropped the mesh output primitive size");
        auto pointState = state;
        pointState.vertexPoints = true;
        pointState.pointSize = 4.0f;
        pointState.pointSizeMin = 0.5f;
        pointState.pointSizeMax = 8.0f;
        GraphicsPipelineRecipe pointDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(pointState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), pointDecoded), "the point list recipe does not decode");
        Require(pointDecoded.state.vertexPoints && pointDecoded.state.pointSize == 4.0f && pointDecoded.state.pointSizeMin == 0.5f && pointDecoded.state.pointSizeMax == 8.0f, "the recipe dropped the vertex point list or its point sizes");
        auto meshPointState = pointState;
        meshPointState.vertexPoints = false;
        meshPointState.stages.mesh = *lineState.stages.mesh;
        meshPointState.stages.mesh->outputPrimitiveSize = 1u;
        meshPointState.stages.mesh->provokingVertex = 0u;
        GraphicsPipelineRecipe meshPointDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(meshPointState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), meshPointDecoded), "the mesh point recipe does not decode");
        Require(!meshPointDecoded.state.vertexPoints && meshPointDecoded.state.stages.mesh && meshPointDecoded.state.stages.mesh->outputPrimitiveSize == 1u && meshPointDecoded.state.pointSize == 4.0f && meshPointDecoded.state.pointSizeMin == 0.5f && meshPointDecoded.state.pointSizeMax == 8.0f, "the recipe dropped mesh points or their sizes without the vertex point flag");
        auto hullState = state;
        hullState.stages.hullWaveSize = 32u;
        hullState.stages.tessellation->mergedHull = true;
        hullState.stages.tessellationGeometry = ShaderRecompiler::MeshConfiguration{4, 10, 30, 30, 60, 192, 512, 0, 4, 3};
        hullState.rasterizerDiscard = true;
        GraphicsPipelineRecipe hullDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(hullState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), hullDecoded), "the tessellation geometry recipe does not decode");
        Require(hullDecoded.state.stages.hullWaveSize == 32u && hullDecoded.state.stages.tessellation && hullDecoded.state.stages.tessellation->mergedHull && hullDecoded.state.stages.tessellationGeometry && hullDecoded.state.stages.tessellationGeometry->threadsPerGroup == 192u && hullDecoded.state.rasterizerDiscard, "the recipe dropped the hull wave size, the merged hull, the geometry stage after tessellation or rasterizer discard");
        auto fourSampleState = state;
        fourSampleState.samples = 4;
        auto maskState = fourSampleState;
        maskState.quadSampleMask = {0x3u, 0x3u, 0x3u, 0x3u};
        maskState.stencilReplayReference = 7u;
        maskState.reversedBlend = 1u;
        GraphicsPipelineRecipe maskDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(maskState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), maskDecoded), "the sample mask recipe does not decode");
        Require(maskDecoded.state.quadSampleMask == maskState.quadSampleMask && maskDecoded.state.stencilReplayReference.has_value() && maskDecoded.state.reversedBlend == 1u, "the recipe dropped the quad sample masks, the replayed stencil reference or the reversed blend");
        std::vector<std::byte> fourSamples, masked, maskReplayed;
        BuildPipelineStoreKey(fourSamples, context, fourSampleState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        BuildPipelineStoreKey(masked, context, maskState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        BuildPipelineStoreKey(maskReplayed, context, maskDecoded.state, maskDecoded.vertexInput, layouts.View(), shaders, maskDecoded.attachmentLayout);
        Require(masked != fourSamples && masked == maskReplayed, "the decoded sample mask state keys another pipeline");
        auto mergedState = fourSampleState;
        mergedState.mergedCoverage = true;
        GraphicsPipelineRecipe mergedDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(mergedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), mergedDecoded), "the merged coverage recipe does not decode");
        Require(mergedDecoded.state.mergedCoverage, "the recipe dropped merged coverage");
        std::vector<std::byte> mergedKey, mergedReplayed;
        BuildPipelineStoreKey(mergedKey, context, mergedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        BuildPipelineStoreKey(mergedReplayed, context, mergedDecoded.state, mergedDecoded.vertexInput, layouts.View(), shaders, mergedDecoded.attachmentLayout);
        Require(mergedKey != fourSamples && mergedKey == mergedReplayed, "the decoded merged coverage state keys another pipeline");
        auto partialState = state;
        partialState.colors[0].partialWriteMask = 0xf000u;
        GraphicsPipelineRecipe partialDecoded;
        Require(DecodeGraphicsPipelineRecipe(EncodeGraphicsPipelineRecipe(MakeGraphicsPipelineRecipe(partialState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL)), partialDecoded), "the partial write recipe does not decode");
        Require(partialDecoded.state.colors.size() == partialState.colors.size() && partialDecoded.state.colors[0].partialWriteMask == 0xf000u, "the recipe dropped the merged target's partial write mask");
        std::vector<std::byte> partialKey, partialReplayed;
        BuildPipelineStoreKey(partialKey, context, partialState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        BuildPipelineStoreKey(partialReplayed, context, partialDecoded.state, partialDecoded.vertexInput, layouts.View(), shaders, partialDecoded.attachmentLayout);
        Require(partialKey != original && partialKey == partialReplayed, "the decoded partial write state keys another pipeline");
        auto movedState = state;
        movedState.viewport.x = 5;
        movedState.colors[0].address = 0x9000;
        movedState.renderLayers = 4;
        BuildPipelineStoreKey(moved, context, movedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(moved == original, "a field the pipeline does not read changed the key");
        auto mappedState = state;
        mappedState.colors[0].componentMapping = 0x24;
        BuildPipelineStoreKey(mapped, context, mappedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(mapped != original, "the component mapping did not enter the key");
        std::vector<std::byte> unclamped, clamped, biased;
        auto clampedState = state;
        clampedState.depthBiasClamp = 1.0f;
        BuildPipelineStoreKey(clamped, context, clampedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(clamped == original, "a depth bias clamp without depth bias changed the key");
        clampedState.depthBias = true;
        BuildPipelineStoreKey(clamped, context, clampedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        clampedState.depthBiasClamp = 0.0f;
        BuildPipelineStoreKey(unclamped, context, clampedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(unclamped != original && clamped != unclamped && CanonicalPipelineState(clampedState).depthBias && CanonicalPipelineState(mappedState).depthBiasClamp == 0.0f, "the depth bias clamp did not enter the key");
        clampedState.depthBiasClamp = 2.0f;
        BuildPipelineStoreKey(biased, context, clampedState, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL);
        Require(biased == clamped, "the depth bias clamp's value entered the key");
        auto compute = module(3);
        compute.bindings.push_back(makeBinding(Role::GuestBuffers, 0, 2, std::vector<std::uint32_t>(8)));
        compute.pushConstants.resize(16);
        compute.hostSubgroupSize = 32;
        constexpr std::uint64_t fingerprint = 0x5eed'0000'0000'0001ull;
        {
            auto bound = StartPipelineJournalReplay(context, fingerprint, {});
            Require(bound != nullptr, "the first replay did not start");
            bound->Wait();
            Require(bound->Done() && PipelineJournalTotals().loadedRecipes == 0, "an absent journal loaded pipelines");
        }
        const auto before = PipelineJournalTotals();
        RecordGraphicsPipeline(recipe, shaders);
        RecordComputePipeline(compute, true);
        FlushPipelineJournal();
        const auto recorded = PipelineJournalTotals();
        Require(recorded.recordedGraphics == before.recordedGraphics + 1 && recorded.recordedCompute == before.recordedCompute + 1 && recorded.recordedModules == before.recordedModules + 3 && recorded.writeFailures == 0, "the journal did not record one graphics and one compute pipeline with three modules");
        RecordGraphicsPipeline(recipe, shaders);
        RecordComputePipeline(compute, true);
        FlushPipelineJournal();
        const auto repeated = PipelineJournalTotals();
        Require(repeated.recordedGraphics == recorded.recordedGraphics && repeated.recordedCompute == recorded.recordedCompute && repeated.recordedModules == recorded.recordedModules, "a repeated pipeline was recorded again");
        Require(std::filesystem::file_size(journalFile) > 0, "the journal file is empty");
        const auto storeBefore = PipelineStoreTotals();
        std::vector<std::byte> receivedWords;
        ShaderRecompiler::RecompileResult received;
        bool receivedPush = false;
        std::uint32_t computeCalls = 0;
        std::mutex computeMutex;
        PipelineJournalHooks hooks{[&](const ShaderRecompiler::RecompileResult& shader, bool pushConstants) {
            std::lock_guard lock(computeMutex);
            ++computeCalls;
            received = shader;
            receivedPush = pushConstants;
            return true;
        }};
        auto replay = StartPipelineJournalReplay(context, fingerprint, hooks);
        Require(replay != nullptr, "the replay did not start");
        replay->Wait();
        Require(replay->Done(), "the replay did not finish");
        const auto totals = PipelineJournalTotals();
        Require(totals.loadedRecipes == 2 && totals.loadedModules == 3 && totals.droppedRecords == 0, "the replay did not load the two recorded pipelines and three modules");
        Require(totals.replayedGraphics == 1 && totals.replayedCompute == 1 && totals.replayRejected == 0 && totals.replayFailed == 0, "the replay did not build one graphics and one compute pipeline");
        Require(computeCalls == 1 && receivedPush && received.Identity() == compute.Identity() && received.spirv == compute.spirv && received.bindings.size() == 1 && received.bindings[0].count == 2 && received.pushConstants.size() == 16 && received.hostSubgroupSize == 32, "the compute hook did not receive the recorded result");
        replayedComputeIdentityLow = received.Identity().low;
        const auto storeReplayed = PipelineStoreTotals();
        Require(storeReplayed.misses == storeBefore.misses + 1 && storeReplayed.cached == storeBefore.cached + 1, "the replayed graphics pipeline did not enter the store");
        const auto created = mock.pipelineCreateCount;
        bool built = true;
        const auto pipeline = CachedPipeline(context, state, input, layouts.View(), shaders, VK_IMAGE_LAYOUT_GENERAL, &built);
        const auto storeHit = PipelineStoreTotals();
        Require(pipeline != nullptr && !built && mock.pipelineCreateCount == created && storeHit.hits == storeReplayed.hits + 1 && storeHit.misses == storeReplayed.misses, "the draw's configuration did not find the replayed pipeline");
        {
            const auto& journalPath = journalFile;
            const auto whole = std::filesystem::file_size(journalPath);
            Require(whole % 8 == 0 && whole > 3, "the journal file is not record aligned");
            std::filesystem::resize_file(journalPath, whole - 3);
            auto later = module(4);
            later.bindings.push_back(makeBinding(Role::GuestBuffers, 0, 1, std::vector<std::uint32_t>(4)));
            RecordComputePipeline(later, false);
            FlushPipelineJournal();
            Require(std::filesystem::file_size(journalPath) % 8 == 0 && PipelineJournalTotals().writeFailures == 0, "an append after a tail torn inside a record's padding left the file unaligned");
            auto torn = StartPipelineJournalReplay(context, fingerprint, hooks);
            Require(torn != nullptr, "the replay after the torn tail did not start");
            torn->Wait();
            const auto reloaded = PipelineJournalTotals();
            Require(reloaded.droppedRecords == 0 && reloaded.loadedModules == 4 && reloaded.loadedRecipes == 2 && reloaded.replayFailed == 0, "the journal did not recover from a tail torn inside a record's padding");
            const auto writtenBefore = PipelineJournalTotals().writtenBytes;
            RecordComputePipeline(compute, true);
            FlushPipelineJournal();
            Require(PipelineJournalTotals().recordedCompute == reloaded.recordedCompute + 1 && PipelineJournalTotals().recordedModules == reloaded.recordedModules, "the pipeline whose record was torn was not recorded again, or its module was");
            const auto recordBytes = PipelineJournalTotals().writtenBytes - writtenBefore;
            std::vector<std::byte> bytes;
            Require(ShaderRecompiler::ReadWholeFile(journalFile, bytes) && recordBytes > 0 && recordBytes <= bytes.size(), "cannot read the re-recorded journal entry");
            {
                std::ofstream file(journalFile, std::ios::binary | std::ios::app);
                file.write(reinterpret_cast<const char*>(bytes.data() + bytes.size() - recordBytes), static_cast<std::streamsize>(recordBytes));
                const std::array<char, 8> tornTail{};
                file.write(tornTail.data(), tornTail.size());
                file.close();
                Require(file.good(), "cannot append an external complete record and torn tail");
            }
            auto another = module(5);
            another.bindings.push_back(makeBinding(Role::GuestBuffers, 0, 1, std::vector<std::uint32_t>(4)));
            const auto beforeAppend = PipelineJournalTotals();
            RecordComputePipeline(another, false);
            FlushPipelineJournal();
            const auto appended = PipelineJournalTotals();
            Require(appended.writeFailures == 0 && appended.recordedCompute == beforeAppend.recordedCompute + 1, "the append after an external torn tail failed");
            Require(std::filesystem::file_size(journalFile) == bytes.size() + recordBytes + appended.writtenBytes - beforeAppend.writtenBytes, "tail recovery discarded an external complete record or retained torn bytes");
            auto recovered = StartPipelineJournalReplay(context, fingerprint, hooks);
            Require(recovered != nullptr, "the replay after the external append did not start");
            recovered->Wait();
            Require(PipelineJournalTotals().loadedModules == 5 && PipelineJournalTotals().loadedRecipes == 4 && PipelineJournalTotals().droppedRecords == 0, "the external append and torn-tail recovery did not replay cleanly");
        }
        {
            const auto before = PipelineJournalTotals();
            auto cancelled = StartPipelineJournalReplay(context, fingerprint, hooks);
            Require(cancelled != nullptr, "the third replay did not start");
            cancelled->Cancel();
            cancelled->Wait();
            const auto after = PipelineJournalTotals();
            Require(cancelled->Done() && after.replayFailed == 0 && after.replayRejected == 0 && after.replayedGraphics + after.replayPresent <= before.replayedGraphics + before.replayPresent + 1, "a cancelled replay did not stop cleanly");
        }
        {
            const std::uint32_t meshWithoutOutputSize = 2u;
            std::fstream file(journalFile, std::ios::in | std::ios::out | std::ios::binary);
            file.seekp(4);
            file.write(reinterpret_cast<const char*>(&meshWithoutOutputSize), sizeof(meshWithoutOutputSize));
            file.close();
            Require(file.good(), "the journal header could not be rewritten");
            auto stale = StartPipelineJournalReplay(context, fingerprint, hooks);
            Require(stale != nullptr, "the replay of a journal in the previous format did not start");
            stale->Wait();
            const auto discarded = PipelineJournalTotals();
            Require(discarded.loadedRecipes == 0 && discarded.loadedModules == 0, "a journal in format 2, whose mesh records lack the output primitive size, was loaded");
        }
        ClearCachedPipelines(context.device);
    }
    Require(replayedComputeIdentityLow != 0, "the replayed compute identity is empty");
    Require(mock.live == 0, "the pipeline journal test leaked Vulkan objects");
}

int main(int argc, char** argv) {
#ifdef _WIN32
    _putenv_s("APS5_PIN_WAIT_MS", "200");
    _putenv_s("APS5_HEAP_MIRROR_MIB", "4");
#else
    setenv("APS5_PIN_WAIT_MS", "200", 1);
    setenv("APS5_HEAP_MIRROR_MIB", "4", 1);
#endif
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--descriptor-cache") {
            descriptorCacheTests();
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--unused-viewport-depth") {
            UnusedViewportDepthTests();
            DepthClipTests();
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--unused-stencil-reference") {
            stencilReferenceUnusedTests();
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--command-batch-fence-failure") {
            commandBatchFenceFailureTests();
            return 0;
        }
        if (argc == 2 && (std::string_view(argv[1]) == "--pipeline-journal" || std::string_view(argv[1]) == "--pipeline-journal-long-paths" || std::string_view(argv[1]) == "--pipeline-journal-extended-paths")) {
            pipelineJournalTests(std::string_view(argv[1]) != "--pipeline-journal", std::string_view(argv[1]) == "--pipeline-journal-extended-paths");
            std::cout << "pipeline journal recipe, record and replay tests passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--pipeline-library-clear-waits-for-build") {
            pipelineLibraryClearWaitsForBuildTests();
            std::cout << "pipeline library clear waits for active part builds\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--pipeline-store-replay-growth") {
            pipelineStoreReplayGrowthTests();
            std::cout << "a replayed journal longer than the store bound stays resident\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--pipeline-store-growth") {
            pipelineStoreGrowthTests(std::getenv("APS5_PIPELINE_STORE_ENTRIES") != nullptr);
            std::cout << "pipeline store bound follows the evicted pipelines that come back\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--pipeline-store-lazy-buffer-pool") {
            pipelineStoreLazyBufferPoolTests();
            std::cout << "cached pipelines survive lazy buffer pool creation\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--copy-input-kept-bytes") {
            copyInputKeptBytesTests();
            std::cout << "GPU-copy input snapshot accounting and retention passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--depth-copy-kept-bytes") {
            depthCopyKeptBytesTests();
            std::cout << "Depth-copy linear buffer accounting and retention passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--sparse-buffer-holes") {
            bool passed = false;
            {
                auto context = mockContext();
                context.limits.maxStorageBufferRange = 1u << 27;
                passed = RunSparseBufferHoleTests(context, {
                    [](VkBuffer buffer) -> std::span<std::byte> { return mockBufferMemory(buffer); }, {}, {}, {}, {}
                });
            }
            Require(mock.live == 0, "sparse buffer hole tests leaked Vulkan objects");
            std::cout << "sparse buffer hole tests " << (passed ? "passed" : "failed") << '\n';
            return passed ? 0 : 1;
        }
        if (argc == 2 && (std::string_view(argv[1]) == "--heap-mirror-unwatch" || std::string_view(argv[1]) == "--heap-mirror-unwatched-import")) {
            {
                mock = MockVulkan{};
                auto context = mockContext();
                context.bufferDeviceAddress = true;
                context.limits.maxStorageBufferRange = 1u << 27;
                RunHeapMirrorUnwatchTests(context, {
                    [](VkBuffer buffer) -> std::span<std::byte> { return mockBufferMemory(buffer); },
                    {},
                    [](VkDeviceAddress address) {
                        const auto offset = address - 0x100000000000ULL;
                        const auto buffer = reinterpret_cast<VkBuffer>(offset / 0x10000);
                        return mockBufferMemory(buffer).subspan(offset % 0x10000);
                    },
                    {},
                    {}
                }, std::string_view(argv[1]) == "--heap-mirror-unwatched-import");
            }
            Require(mock.live == 0, "the heap mirror unwatch test leaked Vulkan objects");
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--sparse-heap-mirror-unwatch") {
            {
                mock = MockVulkan{};
                auto context = mockContext();
                context.bufferDeviceAddress = true;
                context.limits.maxStorageBufferRange = 1u << 27;
                RunSparseHeapMirrorUnwatchTests(context, {
                    [](VkBuffer buffer) -> std::span<std::byte> { return mockBufferMemory(buffer); },
                    {},
                    [](VkDeviceAddress address) {
                        const auto offset = address - 0x100000000000ULL;
                        const auto buffer = reinterpret_cast<VkBuffer>(offset / 0x10000);
                        return mockBufferMemory(buffer).subspan(offset % 0x10000);
                    },
                    {},
                    {}
                });
            }
            Require(mock.live == 0, "the sparse heap mirror unwatch test leaked Vulkan objects");
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--sparse-heap-mirror-budget") {
            {
                mock = MockVulkan{};
                auto context = mockContext();
                context.bufferDeviceAddress = true;
                context.limits.maxStorageBufferRange = 1u << 27;
                RunSparseHeapMirrorBudgetTests(context, {
                    [](VkBuffer buffer) -> std::span<std::byte> { return mockBufferMemory(buffer); },
                    {},
                    {},
                    {},
                    {}
                });
            }
            Require(mock.live == 0, "the sparse heap mirror budget test leaked Vulkan objects");
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--heap-mirror-run-unwatch") {
            {
                mock = MockVulkan{};
                auto context = mockContext();
                context.bufferDeviceAddress = true;
                context.limits.maxStorageBufferRange = 1u << 27;
                RunHeapMirrorRunUnwatchTests(context, {
                    [](VkBuffer buffer) -> std::span<std::byte> { return mockBufferMemory(buffer); },
                    {},
                    [](VkDeviceAddress address) {
                        const auto offset = address - 0x100000000000ULL;
                        const auto buffer = reinterpret_cast<VkBuffer>(offset / 0x10000);
                        return mockBufferMemory(buffer).subspan(offset % 0x10000);
                    },
                    {},
                    {}
                });
            }
            Require(mock.live == 0, "the heap mirror run unwatch test leaked Vulkan objects");
            return 0;
        }
        if (argc == 2 && (std::string_view(argv[1]) == "--address-copy-cap" || std::string_view(argv[1]) == "--address-copy-cap-remembered")) {
#ifdef _WIN32
            _putenv_s("APS5_ADDRESS_COPY_MAX_MIB", "1");
#else
            setenv("APS5_ADDRESS_COPY_MAX_MIB", "1", 1);
#endif
            {
                mock = MockVulkan{};
                auto context = mockContext();
                context.bufferDeviceAddress = true;
                context.limits.maxStorageBufferRange = 1u << 27;
                RunAddressCopyCapTests(context, {
                    [](VkBuffer buffer) -> std::span<std::byte> { return mockBufferMemory(buffer); },
                    {},
                    [](VkDeviceAddress address) {
                        const auto offset = address - 0x100000000000ULL;
                        const auto buffer = reinterpret_cast<VkBuffer>(offset / 0x10000);
                        return mockBufferMemory(buffer).subspan(offset % 0x10000);
                    },
                    {},
                    {}
                }, std::string_view(argv[1]) == "--address-copy-cap-remembered");
            }
            Require(mock.live == 0, "the address copy cap test leaked Vulkan objects");
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--address-space-teardown") {
            addressSpaceTeardownTests();
            std::cout << "address-space device teardown releases cached guest allocations\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--buffer-budget-placement") {
            bufferBudgetPlacementTests();
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--sample-location-grid-fallback") {
            sampleLocationGridFallbackTests();
            std::cout << "Sample location grid fallback checks passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--sample-order") {
            sampleOrderTests();
            std::cout << "Sample order checks passed\n";
            return 0;
        }
        {
            AgcDriver::Graphics::PipelineCacheSynchronization synchronization;
            auto pipelineUse = synchronization.LockForPipelineUse();
            bool snapshotAcquired = false;
            std::thread snapshot([&] { snapshotAcquired = synchronization.TryLockForSnapshot().owns_lock(); });
            snapshot.join();
            Require(!snapshotAcquired, "pipeline cache snapshot overlapped pipeline creation");
            pipelineUse.unlock();
            Require(synchronization.TryLockForSnapshot().owns_lock(), "pipeline cache snapshot stayed blocked after pipeline creation finished");
        }
        if (argc == 2 && std::string_view(argv[1]) == "--undithered-alpha-to-coverage-state") {
            UnditheredAlphaToCoverageTests();
            std::cout << "Existing undithered alpha-to-coverage states passed\n";
            return 0;
        }
        if (argc == 2 && (std::string_view(argv[1]) == "--resource-content-key" || std::string_view(argv[1]) == "--benchmark-resource-content-key")) {
            ResourceContentKeyTests(std::string_view(argv[1]) == "--benchmark-resource-content-key");
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--descriptor-layout-flags") {
            descriptorLayoutFlagTests();
            std::cout << "Descriptor layout native and captured binding flags passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--alpha-to-coverage-state") {
            MsaaEnableSingleSampleTests();
            AlphaToCoverageTests();
            std::cout << "Alpha-to-coverage state and refusal tests passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--image-table-allocation") {
            imageTableAllocationTests();
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--image-table-snapshot") {
            imageTableSnapshotAllocationTests();
            return 0;
        }
        const std::array<std::string_view, 3> samplerCases{"--sampler-table-lod-bias", "--sampler-table-anisotropy", "--sampler-table-reduction-compare"};
        for (std::uint32_t index = 0; index < samplerCases.size(); ++index) {
            if (argc == 2 && std::string_view(argv[1]) == samplerCases[index]) {
                samplerTableDeviceLimitTests(index);
                return 0;
            }
        }
        if (argc == 2 && std::string_view(argv[1]) == "--descriptor-pool-reset-failure") {
            descriptorPoolResetFailureTest();
            std::cerr << "descriptor pool reset failure did not terminate\n";
            return 1;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--unobserved-provoking-vertex") {
            UnobservedProvokingVertexTests();
            ProvokingVertexTests();
            std::cout << "Unobserved provoking vertex checks passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--covered-centroid-state") {
            multisampleTests();
            std::cout << "Covered-centroid and multisample state checks passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--multisample-dcc-state") {
            multisampleDccStateTests();
            std::cout << "Multisampled DCC state checks passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--depth-bounds-state") {
            DepthBoundsBiasTests();
            std::cout << "Depth bounds state checks passed\n";
            return 0;
        }
        {
            const AgcDriver::Graphics::Context context{};
            const AgcDriver::Graphics::State state{};
            AgcDriver::Pm4::DrawParameters draw{0, 0, 0, 1, 0, false};
            AgcDriver::Graphics::Draw(context, state, draw, {});
            draw.indexCount = 3;
            draw.instanceCount = 0;
            AgcDriver::Graphics::Draw(context, state, draw, {});
            draw.instanceCount = 2;
            draw.firstInstance = 0xffffffffu;
            expectFailure([&] { AgcDriver::Graphics::Draw(context, state, draw, {}); }, "instance range overflow");
            draw.firstInstance = 0;
            draw.firstVertex = 0xffffffffu;
            expectFailure([&] { AgcDriver::Graphics::Draw(context, state, draw, {}); }, "vertex range overflow");
            draw.firstVertex = 0;
            draw.indexAddress = 1;
            expectFailure([&] { AgcDriver::Graphics::Draw(context, state, draw, {}); }, "must not reference an index buffer");
            draw.indexAddress = 0;
            draw.flags = 1;
            expectFailure([&] { AgcDriver::Graphics::Draw(context, state, draw, {}); }, "draw modifiers");
            auto bounded = state;
            bounded.depthBoundsTest = true;
            bounded.minDepthBounds = 0.25f;
            AgcDriver::Graphics::ValidateDepthBounds(context, bounded);
            bounded.minDepthBounds = 1.5f;
            expectFailure([&] { AgcDriver::Graphics::ValidateDepthBounds(context, bounded); }, "depth bounds outside [0, 1]");
            auto unrestricted = context;
            unrestricted.depthRangeUnrestricted = true;
            AgcDriver::Graphics::ValidateDepthBounds(unrestricted, bounded);
        }
        RunGuestLeaseWaitTests();
        ResourceContentKeyTests(false);
        stateTests();
        pipelineStoreLazyBufferPoolTests();
        retainedPixelStageTests();
        pipelineLibrarySetTests();
        pipelineLibraryClearWaitsForBuildTests();
        tessellationPipelineLibraryCacheTests();
        framebufferOwnerTests();
        hdrComponentSwapTests();
        depthMaintenanceTests();
        hardwareScreenOffsetTests();
        srgb8TargetTests();
        DepthClipTests();
        DepthRemapTests();
        DepthBoundsEmulationTests();
        UnusedViewportDepthTests();
        DepthStencilTests();
        OneDimensionalColorTests();
        htileDepthClearTests();
        multisampledSingleClearTests();
        memoryValueFillTests();
        ZExportTests();
        unorm10_11_11TargetTests();
        float11_11_10TargetTests();
        unorm4_4_4_4TargetTests();
        uintTargetTests();
        DepthBoundsBiasTests();
        conservativeZExportTests();
        earlyFragmentTestsTests();
        orderedPixelShaderTests();
        stencilRefExportTests();
        stencilReferenceUnusedTests();
        stencilLogicOperationTests();
        aliasedBufferViewTests();
        LineStripTests();
        ShaderModuleKeyTests();
        ProvokingVertexTests();
        ProvokingVertexRenderPassTests();
        ConservativeRasterizationTests();
        DisabledColorTests();
        OpaqueDestinationAlphaTests();
        Color565TargetTests();
        Color1555TargetTests();
        BlendBypassTests();
        UnclampedNormalizedColorTests();
        CompactedExportTests();
        ReversedComponentOrderTests();
        ReversedComponentOrderBlendTests();
        ReversedBlendRoutingTests();
        ShadersWriteMemoryTests();
        DualSourceBlendTests();
        ReversedBlendShaderTests();
        metadataPassTests();
        depthCopyPassTests();
        cmaskTests();
        uint16ExportTests();
        uint8x2TargetTests();
        uint8x4TargetTests();
        msaaEnableClearDepthTests();
        multisampleTests();
        MsaaEnableSingleSampleTests();
        multisampledDepthTests();
        clipDistanceTests();
        framebufferLayerTests();
        msaaEnableClearOneSampleTargetTests();
        UnwrittenSampleLocationTests();
        sampleLocationGridFallbackTests();
        sampleOrderTests();
        UnwrittenDepthPlaneTests();
        depthCopyFormatTests();
        quadPixelMaskTests();
        ShaderStageTests();
        UnsupportedTessellationFieldTests();
        NggVertexWorkgroupTests();
        inactiveVertexPointSizeTests();
        PolygonPrimitiveTests();
        vertexPointListTests();
        TuningFieldTests();
        PixelInputLayoutTests();
        UnexportedPixelInputTests();
        ColorPipeBankXorTests();
        HeaderlessVertexStageTests();
        HiddenUserWordVertexTablesTests();
        ComputeScratchTests();
        shaderUserDataTailPaddingTests();
        InitialContextTests();
        AlphaToCoverageTests();
        descriptorPoolTests();
        pushConstantTests();
        resourceTests();
        oversizedDescriptorTests();
        guestBufferViewBytesTests();
        descriptorSnapshotTests();
        dispatchMoveTests();
        descriptorCacheTests();
        bufferBudgetPlacementTests();
        imageTableAllocationTests();
        imageTableSnapshotAllocationTests();
        samplerTableNonSeamlessCubeTests();
        samplerTableDeviceLimitTests();
        tableUpdateAfterBindTests();
        misalignedShaderDataTests();
        imageTableTemplateRefreshTests();
        debugBranchTests();
        hostImportBudgetTests();
        textureCacheBudgetTests();
        sampledTextureBudgetTests();
        storageTextureBudgetTests();
        sampledBudgetReportTests();
        meshArgumentTests();
        meshLimitTests();
        meshIndexBufferTests();
        stagingPoolBudgetTests();
        validationTests();
        vertexCopyTests();
        highestDrawIndexTests();
        storeAtFlipTests();
        storeAtLabelTests();
        nullVertexDescriptorTests();
        vertexZeroFillTests();
        vertexOverhangTests();
        vertexZeroFillMergeTests();
        parallelCompareTests();
        pixelParameterSlotTests();
        rectListTests();
        quadListTests();
        floatControlsModeTests();
        barycentricEmulationTests();
        copiedWriterIndirectOverlapTests();
        registeredReadableCoverTests();
        mock = MockVulkan{};
        auto bdaContext = mockContext();
        bdaContext.bufferDeviceAddress = true;
        bdaContext.limits.maxStorageBufferRange = 1u << 27;
        RunBdaResourceTests(bdaContext, {
            [](VkBuffer buffer) -> std::span<std::byte> { return mockBufferMemory(buffer); },
            [](std::uint32_t binding) {
                for (auto it = mock.writes.rbegin(); it != mock.writes.rend(); ++it) {
                    if (it->binding == binding) return it->buffers.at(0);
                }
                throw std::runtime_error("missing BDA test descriptor");
            },
            [](VkDeviceAddress address) {
                const auto offset = address - 0x100000000000ULL;
                const auto buffer = reinterpret_cast<VkBuffer>(offset / 0x10000);
                return mockBufferMemory(buffer).subspan(offset % 0x10000);
            },
            [](std::optional<VkDeviceSize> headroom) {
                mock.memoryLimit = headroom.has_value() ? std::optional<VkDeviceSize>(mock.allocatedBytes + *headroom) : std::nullopt;
            },
            [] { return mock.allocationAttempts; },
            [](std::optional<VkDeviceSize> limit) { mock.importLimit = limit; }
        });
        Require(mock.live == 0, "BDA resources leaked Vulkan objects");
        RunGuestAllocationTests();
        RunUnmappedGapTests();
        RunColorTargetLayoutTests();
        RunLiveStackAccessTests();
        RunTextureFormatTests();
        RunTextureTilingTests();
        RunGuestTextureResourceTests();
        RunDepthSurfaceReuseTests();
        RunGuestSamplerResourceTests();
        RunLastRecordIndicesTests();
        mock = MockVulkan{};
        auto textureDetilerContext = mockContext();
        textureDetilerContext.limits.minStorageBufferOffsetAlignment = 16;
        textureDetilerContext.limits.maxStorageBufferRange = 256;
        RunTextureDetilerTests(textureDetilerContext, {
            [](std::uint64_t size) {
                VkBuffer buffer{};
                VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
                bufferInfo.size = size;
                bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                mockCreateBuffer(VK_NULL_HANDLE, &bufferInfo, nullptr, &buffer);
                VkMemoryRequirements requirements{};
                mockGetBufferMemoryRequirements(VK_NULL_HANDLE, buffer, &requirements);
                VkMemoryAllocateInfo allocationInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
                allocationInfo.allocationSize = requirements.size;
                VkDeviceMemory memory{};
                mockAllocateMemory(VK_NULL_HANDLE, &allocationInfo, nullptr, &memory);
                mockBindBufferMemory(VK_NULL_HANDLE, buffer, memory, 0);
                return buffer;
            },
            [](VkBuffer buffer) -> std::vector<std::byte>& { return mock.memories.at(mock.bufferMemory.at(buffer)); },
            [] {
                Require(mock.writes.size() >= 2, "expected texture detiling descriptor writes");
                const auto& destinationWrite = mock.writes.back();
                const auto& sourceWrite = mock.writes[mock.writes.size() - 2];
                DetilerCapture capture{};
                capture.groupsX = mock.lastDispatchGroups.x;
                capture.groupsY = mock.lastDispatchGroups.y;
                capture.groupsZ = mock.lastDispatchGroups.z;
                capture.pushConstants = mock.lastPushConstants;
                capture.sourceBuffer = sourceWrite.buffers.at(0).buffer;
                capture.sourceOffset = sourceWrite.buffers.at(0).offset;
                capture.sourceRange = sourceWrite.buffers.at(0).range;
                capture.destinationBuffer = destinationWrite.buffers.at(0).buffer;
                capture.destinationOffset = destinationWrite.buffers.at(0).offset;
                capture.destinationRange = destinationWrite.buffers.at(0).range;
                return capture;
            },
            [] { return mock.pipelineCreateCount; },
            [] { return mock.pipelineSpecializations.back(); },
            [] { return mock.descriptorPools.size(); },
            [] {
                std::uint32_t resets = 0;
                for (const auto& [handle, pool] : mock.descriptorPools) resets += pool.resets;
                return resets;
            }
        });
        std::cout << "Graphics validation tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
