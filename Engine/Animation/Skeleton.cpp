#include "Skeleton.h"
#include "Math/Math.h"
#include <cmath>
#include <algorithm>

namespace Muk {

Skeleton* AnimationSystem::GetSkeleton(const std::string& n) {
    auto it = m_Skeletons.find(n);
    return it == m_Skeletons.end() ? nullptr : &it->second;
}

AnimationClip* AnimationSystem::GetClip(const std::string& n) {
    auto it = m_Clips.find(n);
    return it == m_Clips.end() ? nullptr : &it->second;
}

BoneKey AnimationSystem::SampleTrack(const BoneTrack& track, f32 t) {
    if (track.Keys.empty()) return {};
    if (track.Keys.size() == 1) return track.Keys[0];
    if (t <= track.Keys.front().Time) return track.Keys.front();
    if (t >= track.Keys.back().Time) return track.Keys.back();

    for (size_t i = 0; i + 1 < track.Keys.size(); ++i) {
        const auto& a = track.Keys[i];
        const auto& b = track.Keys[i + 1];
        if (t >= a.Time && t <= b.Time) {
            f32 u = (t - a.Time) / std::max(1e-5f, b.Time - a.Time);
            BoneKey k;
            k.Time = t;
            k.Position = a.Position + (b.Position - a.Position) * u;
            k.Rotation = a.Rotation + (b.Rotation - a.Rotation) * u;
            k.Scale = a.Scale + (b.Scale - a.Scale) * u;
            return k;
        }
    }
    return track.Keys.back();
}

Mat4 AnimationSystem::KeyToMatrix(const BoneKey& k) {
    Transform t;
    t.Position = k.Position;
    t.Rotation = k.Rotation;
    t.Scale = k.Scale;
    return t.GetMatrix();
}

void AnimationSystem::Update(Animator& anim, f32 dt) {
    if (!anim.Playing) return;
    auto* sk = GetSkeleton(anim.SkeletonName);
    auto* clip = GetClip(anim.ClipName);
    if (!sk || !clip || sk->Bones.empty()) return;

    anim.Time += dt * anim.Speed;
    if (clip->Loop && clip->Duration > 0) {
        while (anim.Time > clip->Duration) anim.Time -= clip->Duration;
    } else if (anim.Time > clip->Duration) {
        anim.Time = clip->Duration;
        anim.Playing = false;
    }

    const int n = (int)sk->Bones.size();
    std::vector<Mat4> local(n, Mat4::Identity());
    for (int i = 0; i < n; ++i) local[i] = sk->Bones[i].LocalBind;

    for (const auto& track : clip->Tracks) {
        if (track.BoneIndex < 0 || track.BoneIndex >= n) continue;
        local[track.BoneIndex] = KeyToMatrix(SampleTrack(track, anim.Time));
    }

    std::vector<Mat4> global(n, Mat4::Identity());
    for (int i = 0; i < n; ++i) {
        int p = sk->Bones[i].Parent;
        global[i] = (p >= 0 && p < n) ? (global[p] * local[i]) : local[i];
    }

    anim.SkinMatrices.resize(n);
    for (int i = 0; i < n; ++i)
        anim.SkinMatrices[i] = global[i] * sk->Bones[i].InverseBind;
}

void AnimationSystem::EnsureDemoAssets() {
    if (m_Skeletons.count("DemoArm")) return;

    Skeleton sk;
    sk.Name = "DemoArm";
    Bone root;
    root.Name = "Root";
    root.Parent = -1;
    root.LocalBind = Mat4::Translation({0, 0, 0});
    root.InverseBind = Mat4::Translation({0, 0, 0}); // identity-ish
    Bone upper;
    upper.Name = "Upper";
    upper.Parent = 0;
    upper.LocalBind = Mat4::Translation({0, 0.5f, 0});
    upper.InverseBind = Mat4::Translation({0, -0.5f, 0});
    Bone lower;
    lower.Name = "Lower";
    lower.Parent = 1;
    lower.LocalBind = Mat4::Translation({0, 0.5f, 0});
    lower.InverseBind = Mat4::Translation({0, -1.0f, 0});
    sk.Bones = { root, upper, lower };
    m_Skeletons[sk.Name] = sk;

    AnimationClip wave;
    wave.Name = "Wave";
    wave.Duration = 2.0f;
    wave.Loop = true;
    BoneTrack t1;
    t1.BoneIndex = 1;
    t1.Keys = {
        {0.0f, {0,0.5f,0}, {0,0,0}, {1,1,1}},
        {0.5f, {0,0.5f,0}, {0,0,35}, {1,1,1}},
        {1.0f, {0,0.5f,0}, {0,0,0}, {1,1,1}},
        {1.5f, {0,0.5f,0}, {0,0,-35}, {1,1,1}},
        {2.0f, {0,0.5f,0}, {0,0,0}, {1,1,1}},
    };
    BoneTrack t2;
    t2.BoneIndex = 2;
    t2.Keys = {
        {0.0f, {0,0.5f,0}, {0,0,0}, {1,1,1}},
        {0.5f, {0,0.5f,0}, {0,0,20}, {1,1,1}},
        {1.5f, {0,0.5f,0}, {0,0,-20}, {1,1,1}},
        {2.0f, {0,0.5f,0}, {0,0,0}, {1,1,1}},
    };
    wave.Tracks = { t1, t2 };
    m_Clips[wave.Name] = wave;
}

} // namespace Muk
