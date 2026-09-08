#pragma once

#include "Core/Core.h"
#include <string>

namespace Muk {

/** Save a simple TGA screenshot placeholder note; full GPU readback can hook SceneRT later */
class Screenshot {
public:
    // Writes Assets/Screenshots/shot_YYYYMMDD_HHMMSS.txt marker + optional solid TGA
    static bool CapturePlaceholder(const std::string& label = "viewport");
    static bool WriteSolidTga(const std::string& path, u32 w, u32 h, u8 r, u8 g, u8 b);
};

} // namespace Muk
