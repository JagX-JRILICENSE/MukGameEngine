#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include <cmath>

namespace Muk {

struct Frustum {
    // planes: ax+by+cz+d >= 0 inside
    float Planes[6][4]{};

    static Frustum FromViewProj(const float* vp /*16*/) {
        Frustum f;
        // Extract planes from clip matrix (row-major vp)
        auto set = [&](int i, float a, float b, float c, float d) {
            float len = std::sqrt(a*a+b*b+c*c);
            if (len < 1e-8f) len = 1;
            f.Planes[i][0]=a/len; f.Planes[i][1]=b/len; f.Planes[i][2]=c/len; f.Planes[i][3]=d/len;
        };
        // left, right, bottom, top, near, far
        set(0, vp[3]+vp[0], vp[7]+vp[4], vp[11]+vp[8], vp[15]+vp[12]);
        set(1, vp[3]-vp[0], vp[7]-vp[4], vp[11]-vp[8], vp[15]-vp[12]);
        set(2, vp[3]+vp[1], vp[7]+vp[5], vp[11]+vp[9], vp[15]+vp[13]);
        set(3, vp[3]-vp[1], vp[7]-vp[5], vp[11]-vp[9], vp[15]-vp[13]);
        set(4, vp[3]+vp[2], vp[7]+vp[6], vp[11]+vp[10], vp[15]+vp[14]);
        set(5, vp[3]-vp[2], vp[7]-vp[6], vp[11]-vp[10], vp[15]-vp[14]);
        return f;
    }

    bool SphereVisible(const Vec3& c, float r) const {
        for (int i = 0; i < 6; ++i) {
            float d = Planes[i][0]*c.x + Planes[i][1]*c.y + Planes[i][2]*c.z + Planes[i][3];
            if (d < -r) return false;
        }
        return true;
    }
};

struct LODSettings {
    float Lod0 = 15.f;
    float Lod1 = 40.f;
    float Lod2 = 80.f;

    int SelectLOD(float distance) const {
        if (distance < Lod0) return 0;
        if (distance < Lod1) return 1;
        if (distance < Lod2) return 2;
        return 3; // culled / impostor
    }
};

} // namespace Muk
