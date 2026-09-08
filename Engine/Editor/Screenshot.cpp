#include "Screenshot.h"
#include "Core/Log.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>

namespace Muk {

bool Screenshot::WriteSolidTga(const std::string& path, u32 w, u32 h, u8 r, u8 g, u8 b) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    unsigned char header[18] = {};
    header[2] = 2; // uncompressed true-color
    header[12] = (unsigned char)(w & 255);
    header[13] = (unsigned char)((w >> 8) & 255);
    header[14] = (unsigned char)(h & 255);
    header[15] = (unsigned char)((h >> 8) & 255);
    header[16] = 24;
    out.write(reinterpret_cast<char*>(header), 18);
    std::vector<unsigned char> row(w * 3);
    for (u32 x = 0; x < w; ++x) {
        row[x * 3 + 0] = b;
        row[x * 3 + 1] = g;
        row[x * 3 + 2] = r;
    }
    for (u32 y = 0; y < h; ++y)
        out.write(reinterpret_cast<char*>(row.data()), row.size());
    return true;
}

bool Screenshot::CapturePlaceholder(const std::string& label) {
    try {
        std::filesystem::create_directories("Assets/Screenshots");
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream name;
        name << "Assets/Screenshots/shot_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_" << label;
        std::string tga = name.str() + ".tga";
        if (!WriteSolidTga(tga, 320, 180, 40, 30, 70)) return false;
        MUK_CORE_INFO("Screenshot saved: {0}", tga.c_str());
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace Muk
