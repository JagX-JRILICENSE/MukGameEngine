#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace Muk {

struct ProjectSettings {
    std::string ProjectName = "Muk Project";
    float GridSnap = 0.25f;
    bool SnapEnabled = true;
    float CameraSpeed = 5.0f;
    float CameraOrbitSensitivity = 0.25f;

    bool FogEnabled = true;
    Vec3 FogColor{0.08f, 0.09f, 0.12f};
    float FogDensity = 0.015f;
    float FogStart = 15.0f;
    float FogEnd = 80.0f;

    bool DayNightCycle = false;
    float TimeOfDay = 12.0f;
    float DaySpeed = 0.5f;

    void TickDayNight(float dt) {
        if (!DayNightCycle) return;
        TimeOfDay += DaySpeed * dt;
        if (TimeOfDay >= 24.f) TimeOfDay -= 24.f;
    }

    float DayLightFactor() const {
        float t = TimeOfDay / 24.f;
        float angle = (t - 0.25f) * 6.28318f;
        float s = std::sin(angle);
        return std::max(0.08f, s * 0.5f + 0.5f);
    }

    static float Snap(float v, float step) {
        if (step <= 1e-6f) return v;
        return std::round(v / step) * step;
    }

    Vec3 SnapVec(const Vec3& v) const {
        if (!SnapEnabled) return v;
        return { Snap(v.x, GridSnap), Snap(v.y, GridSnap), Snap(v.z, GridSnap) };
    }

    bool Save(const std::string& path = "Assets/project.settings") const {
        std::ofstream out(path);
        if (!out) return false;
        out << "name=" << ProjectName << "\n";
        out << "snap=" << GridSnap << "\n";
        out << "snap_on=" << (SnapEnabled ? 1 : 0) << "\n";
        out << "fog=" << (FogEnabled ? 1 : 0) << "\n";
        out << "fog_density=" << FogDensity << "\n";
        out << "day_night=" << (DayNightCycle ? 1 : 0) << "\n";
        out << "time=" << TimeOfDay << "\n";
        return true;
    }

    bool Load(const std::string& path = "Assets/project.settings") {
        std::ifstream in(path);
        if (!in) return false;
        std::string line;
        while (std::getline(in, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            auto k = line.substr(0, eq);
            auto v = line.substr(eq + 1);
            if (k == "name") ProjectName = v;
            else if (k == "snap") GridSnap = (float)atof(v.c_str());
            else if (k == "snap_on") SnapEnabled = atoi(v.c_str()) != 0;
            else if (k == "fog") FogEnabled = atoi(v.c_str()) != 0;
            else if (k == "fog_density") FogDensity = (float)atof(v.c_str());
            else if (k == "day_night") DayNightCycle = atoi(v.c_str()) != 0;
            else if (k == "time") TimeOfDay = (float)atof(v.c_str());
        }
        return true;
    }
};

} // namespace Muk
