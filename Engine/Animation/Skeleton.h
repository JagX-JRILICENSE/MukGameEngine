#pragma once

#include "Core/Core.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "ECS/Component.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Muk {

struct Bone {
    std::string Name;
    int Parent = -1;
    Mat4 LocalBind = Mat4::Identity();
    Mat4 InverseBind = Mat4::Identity();
};

struct Skeleton {
    std::string Name;
    std::vector<Bone> Bones;

    int FindBone(const std::string& name) const {
        for (int i = 0; i < (int)Bones.size(); ++i)
            if (Bones[i].Name == name) return i;
        return -1;
    }
};

struct BoneKey {
    f32 Time = 0;
    Vec3 Position{0,0,0};
    Vec3 Rotation{0,0,0}; // euler deg
    Vec3 Scale{1,1,1};
};

struct BoneTrack {
    int BoneIndex = 0;
    std::vector<BoneKey> Keys;
};

struct AnimationClip {
    std::string Name;
    f32 Duration = 1.0f;
    bool Loop = true;
    std::vector<BoneTrack> Tracks;
};

/** ECS component: plays a clip on a skeleton */
struct Animator : public IComponent {
    std::string SkeletonName;
    std::string ClipName;
    f32 Time = 0;
    f32 Speed = 1.0f;
    bool Playing = true;
    // Final skin matrices (bone count)
    std::vector<Mat4> SkinMatrices;
};

class AnimationSystem {
public:
    void RegisterSkeleton(const Skeleton& sk) { m_Skeletons[sk.Name] = sk; }
    void RegisterClip(const AnimationClip& clip) { m_Clips[clip.Name] = clip; }

    Skeleton* GetSkeleton(const std::string& n);
    AnimationClip* GetClip(const std::string& n);

    void Update(Animator& anim, f32 dt);

    // Demo: simple 2-bone arm skeleton + wave clip
    void EnsureDemoAssets();

private:
    std::unordered_map<std::string, Skeleton> m_Skeletons;
    std::unordered_map<std::string, AnimationClip> m_Clips;

    static BoneKey SampleTrack(const BoneTrack& track, f32 t);
    static Mat4 KeyToMatrix(const BoneKey& k);
};

} // namespace Muk
