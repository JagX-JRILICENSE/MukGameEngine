#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

// Platform macros
#if defined(MUK_PLATFORM_WINDOWS)
    #define MUK_API
#else
    #define MUK_API
#endif

// Common types
namespace Muk {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

} // namespace Muk

// Logging macros (implemented in Log.cpp)
#define MUK_CORE_TRACE(...)    ::Muk::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define MUK_CORE_INFO(...)     ::Muk::Log::GetCoreLogger()->info(__VA_ARGS__)
#define MUK_CORE_WARN(...)     ::Muk::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define MUK_CORE_ERROR(...)    ::Muk::Log::GetCoreLogger()->error(__VA_ARGS__)
#define MUK_CORE_CRITICAL(...) ::Muk::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define MUK_ASSERT(x, ...) { if(!(x)) { MUK_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
