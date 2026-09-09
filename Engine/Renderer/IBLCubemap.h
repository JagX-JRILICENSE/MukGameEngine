#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "PostSettings.h"
#include "Texture.h"
#include <memory>
#include <vector>

namespace Muk {

/** Procedural environment cubemap faces (CPU) for IBL approximation */
class IBLCubemap {
public:
    static constexpr int FaceSize = 64;

    void Generate(const PostSettings& post);
    const Texture* Face(int i) const { return i >= 0 && i < 6 ? m_Faces[i].get() : nullptr; }
    bool IsReady() const { return m_Ready; }

    /** Sample direction → irradiance-ish color */
    Vec3 SampleDir(const Vec3& dir) const;

private:
    std::shared_ptr<Texture> m_Faces[6];
    bool m_Ready = false;
    PostSettings m_Cached{};
};

/** Cheap bloom: extract bright pixels into a smaller buffer (CPU side for post panel preview) */
struct BloomExtract {
    static void ExtractBright(const std::vector<u8>& rgba, u32 w, u32 h, float threshold,
                              std::vector<u8>& outBright);
};

} // namespace Muk
