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

    static Mat4 Perspective(f32 fovYRadians, f32 aspect, f32 nearZ, f32 farZ) {
        Mat4 result{};
        f32 tanHalfFovy = std::tan(fovYRadians * 0.5f);
        result.m[0] = 1.0f / (aspect * tanHalfFovy);
        result.m[5] = 1.0f / tanHalfFovy;
        result.m[10] = farZ / (nearZ - farZ);
        result.m[11] = -1.0f;
        result.m[14] = (nearZ * farZ) / (nearZ - farZ);
        result.m[15] = 0.0f;
        return result;
    }

    // Right-handed LookAt (camera at eye looking at target)
    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 z = (eye - target).Normalized(); // forward (into scene is -Z in RH)
        Vec3 x = Vec3::Cross(up, z).Normalized();
        Vec3 y = Vec3::Cross(z, x);

        Mat4 result;
        result.m[0] = x.x;  result.m[4] = x.y;  result.m[8]  = x.z;  result.m[12] = -Vec3::Dot(x, eye);
        result.m[1] = y.x;  result.m[5] = y.y;  result.m[9]  = y.z;  result.m[13] = -Vec3::Dot(y, eye);
        result.m[2] = z.x;  result.m[6] = z.y;  result.m[10] = z.z;  result.m[14] = -Vec3::Dot(z, eye);
        result.m[3] = 0;    result.m[7] = 0;    result.m[11] = 0;    result.m[15] = 1;
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
