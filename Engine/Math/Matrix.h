#pragma once

#include "Vector.h"
#include <cmath>

namespace Muk {

struct Mat4 {
    f32 m[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    Mat4() = default;

    static Mat4 Identity() { return Mat4{}; }

    static Mat4 Translation(const Vec3& t) {
        Mat4 result;
        result.m[12] = t.x;
        result.m[13] = t.y;
        result.m[14] = t.z;
        return result;
    }

    static Mat4 Scale(const Vec3& s) {
        Mat4 result;
        result.m[0] = s.x;
        result.m[5] = s.y;
        result.m[10] = s.z;
        return result;
    }

    // Perspective projection (simple)
    static Mat4 Perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
        Mat4 result{};
        f32 tanHalfFovy = std::tan(fovY * 0.5f);
        result.m[0] = 1.0f / (aspect * tanHalfFovy);
        result.m[5] = 1.0f / tanHalfFovy;
        result.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        result.m[15] = 0.0f;
        return result;
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 result{};
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                result.m[col * 4 + row] =
                    m[0 * 4 + row] * other.m[col * 4 + 0] +
                    m[1 * 4 + row] * other.m[col * 4 + 1] +
                    m[2 * 4 + row] * other.m[col * 4 + 2] +
                    m[3 * 4 + row] * other.m[col * 4 + 3];
            }
        }
        return result;
    }
};

} // namespace Muk
