#pragma once

#include "Core/Core.h"
#include <cmath>

namespace Muk {

struct Vec2 {
    f32 x = 0.0f, y = 0.0f;

    Vec2() = default;
    Vec2(f32 x, f32 y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(f32 s) const { return {x * s, y * s}; }

    f32 Length() const { return std::sqrt(x * x + y * y); }
    Vec2 Normalized() const {
        f32 len = Length();
        return len > 0 ? Vec2{x / len, y / len} : Vec2{};
    }
};

struct Vec3 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(f32 s) const { return {x * s, y * s, z * s}; }

    f32 Length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 Normalized() const {
        f32 len = Length();
        return len > 0 ? Vec3{x / len, y / len, z / len} : Vec3{};
    }

    static f32 Dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 Cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

struct Vec4 {
    f32 x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    Vec4() = default;
    Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
};

} // namespace Muk
