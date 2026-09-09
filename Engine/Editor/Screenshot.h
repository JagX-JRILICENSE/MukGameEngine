#pragma once

#include "Core/Core.h"
#include <string>

namespace Muk {

class Renderer;

class Screenshot {
public:
    static bool CapturePlaceholder(const std::string& label = "viewport");
    static bool WriteSolidTga(const std::string& path, u32 w, u32 h, u8 r, u8 g, u8 b);
    static bool WriteTgaRGBA(const std::string& path, u32 w, u32 h, const u8* rgba);

    /** Read back editor SceneRT color target to TGA under Assets/Screenshots */
    static bool CaptureSceneRT(Renderer& renderer, const std::string& label = "viewport");
};

} // namespace Muk
