#pragma once

#include "Vector.h"
#include "Matrix.h"

namespace Muk {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

inline float ToRadians(float degrees) { return degrees * DEG2RAD; }
inline float ToDegrees(float radians) { return radians * RAD2DEG; }

} // namespace Muk
