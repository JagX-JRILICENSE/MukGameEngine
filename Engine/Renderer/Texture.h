#pragma once

#include "Core/Core.h"
#include <string>
#include <vector>
#include <memory>

namespace Muk {

struct Texture {
    std::string Name;
    u32 Width = 0;
    u32 Height = 0;
    u32 Channels = 4; // RGBA
    std::vector<u8> Pixels; // RGBA8

    bool IsValid() const { return Width > 0 && Height > 0 && !Pixels.empty(); }

    static std::shared_ptr<Texture> CreateSolid(u32 w, u32 h, u8 r, u8 g, u8 b, u8 a = 255) {
        auto t = std::make_shared<Texture>();
        t->Name = "Solid";
        t->Width = w;
        t->Height = h;
        t->Channels = 4;
        t->Pixels.resize(w * h * 4);
        for (u32 i = 0; i < w * h; ++i) {
            t->Pixels[i * 4 + 0] = r;
            t->Pixels[i * 4 + 1] = g;
            t->Pixels[i * 4 + 2] = b;
            t->Pixels[i * 4 + 3] = a;
        }
        return t;
    }
};

} // namespace Muk
