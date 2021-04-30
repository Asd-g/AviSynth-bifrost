#include <cstring>
#include <algorithm>

#include "avisynth.h"

enum BlendDirection
{
    bdNext,
    bdPrev,
    bdBoth
};

template <typename T>
static void applyBlockRainbowMask(const T* srcp_u, const T* srcp_v,
    const T* srcc_u, const T* srcc_v,
    const T* srcn_u, const T* srcn_v,
    T* dst_u, T* dst_v,
    int block_width_uv, int block_height_uv,
    int srcp_stride_uv, int srcc_stride_uv, int srcn_stride_uv, int dst_stride_uv,
    BlendDirection blenddirection, int col)
{
    for (int y = 0; y < block_height_uv; ++y)
    {
        if (blenddirection == bdNext)
        {
            for (int x = 0; x < block_width_uv; ++x)
            {
                if (dst_v[x])
                {
                    dst_u[x] = (srcc_u[x] + srcn_u[x] + 1) >> 1;
                    dst_v[x] = (srcc_v[x] + srcn_v[x] + 1) >> 1;
                }
                else
                {
                    dst_u[x] = srcc_u[x];
                    dst_v[x] = srcc_v[x];
                }
            }
        }
        else if (blenddirection == bdPrev)
        {
            for (int x = 0; x < block_width_uv; ++x)
            {
                if (dst_v[x])
                {
                    dst_u[x] = (srcc_u[x] + srcp_u[x] + 1) >> 1;
                    dst_v[x] = (srcc_v[x] + srcp_v[x] + 1) >> 1;
                }
                else
                {
                    dst_u[x] = srcc_u[x];
                    dst_v[x] = srcc_v[x];
                }
            }
        }
        else if (blenddirection == bdBoth)
        {
            for (int x = 0; x < block_width_uv; ++x)
            {
                if (dst_v[x])
                {
                    dst_u[x] = (2 * srcc_u[x] + srcp_u[x] + srcn_u[x] + 3) >> 2;
                    dst_v[x] = (2 * srcc_v[x] + srcp_v[x] + srcn_v[x] + 3) >> 2;
                }
                else
                {
                    dst_u[x] = srcc_u[x];
                    dst_v[x] = srcc_v[x];
                }
            }
        }

        if (col)
        {
            for (int x = block_width_uv; x < block_width_uv + col; ++x)
            {
                dst_u[x] = srcc_u[x];
                dst_v[x] = srcc_v[x];
            }
        }

        srcp_u += srcp_stride_uv;
        srcp_v += srcp_stride_uv;

        srcc_u += srcc_stride_uv;
        srcc_v += srcc_stride_uv;

        srcn_u += srcn_stride_uv;
        srcn_v += srcn_stride_uv;

        dst_u += dst_stride_uv;
        dst_v += dst_stride_uv;
    }
}

template <typename T>
static void processBlockRainbowMask(T* dst_u, T* dst_v, int block_width_uv, int block_height_uv, int dst_stride_uv, bool conservative_mask)
{
    // Maybe needed later.
    T* tmp = dst_v;

    //denoise mask, remove marked pixels with no horizontal marked neighbors
    for (int y = 0; y < block_height_uv; ++y)
    {
        dst_v[0] = dst_u[0] && dst_u[1];

        for (int x = 1; x < block_width_uv - 1; ++x)
            dst_v[x] = dst_u[x] && (dst_u[x - 1] || dst_u[x + 1]);

        dst_v[block_width_uv - 1] = dst_u[block_width_uv - 1] && dst_u[block_width_uv - 2];

        dst_u += dst_stride_uv;
        dst_v += dst_stride_uv;
    }

    //expand mask vertically
    if (!conservative_mask)
    {
        dst_v = tmp;

        for (int x = 0; x < block_width_uv; ++x)
            dst_v[x] = dst_v[x] || dst_v[x + dst_stride_uv];

        dst_v += dst_stride_uv;

        for (int y = 1; y < block_height_uv - 1; ++y)
        {
            for (int x = 0; x < block_width_uv; ++x)
                dst_v[x] = dst_v[x] || (dst_v[x + dst_stride_uv] && dst_v[x - dst_stride_uv]);

            dst_v += dst_stride_uv;
        }

        for (int x = 0; x < block_width_uv; ++x)
            dst_v[x] = dst_v[x] || dst_v[x - dst_stride_uv];
    }
}

template <typename T>
static void makeBlockRainbowMask(const T* srcp_u, const T* srcp_v,
    const T* srcc_u, const T* srcc_v,
    const T* srcn_u, const T* srcn_v,
    T* dst_u, T* dst_v,
    int block_width_uv, int block_height_uv,
    int srcp_stride_uv, int srcc_stride_uv, int srcn_stride_uv, int dst_stride_uv,
    int variation)
{
    for (int y = 0; y < block_height_uv; ++y)
    {
        for (int x = 0; x < block_width_uv; ++x)
        {
            int up = srcp_u[x];
            int uc = srcc_u[x];
            int un = srcn_u[x];

            int vp = srcp_v[x];
            int vc = srcc_v[x];
            int vn = srcn_v[x];

            int ucup = uc - up;
            int ucun = uc - un;

            int vcvp = vc - vp;
            int vcvn = vc - vn;

            dst_u[x] = (((ucup + variation) & (ucun + variation)) < 0)
                || (((-ucup + variation) & (-ucun + variation)) < 0)
                || (((vcvp + variation) & (vcvn + variation)) < 0)
                || (((-vcvp + variation) & (-vcvn + variation)) < 0);
        }

        srcp_u += srcp_stride_uv;
        srcp_v += srcp_stride_uv;

        srcc_u += srcc_stride_uv;
        srcc_v += srcc_stride_uv;

        srcn_u += srcn_stride_uv;
        srcn_v += srcn_stride_uv;

        dst_u += dst_stride_uv;
        dst_v += dst_stride_uv;
    }
}

template <typename T>
static void copyChromaBlock(T* dst_u, T* dst_v,
    const T* src_u, const T* src_v,
    int block_width_uv, int block_height_uv,
    int dst_stride_uv, int src_stride_uv, int col)
{
    for (int y = 0; y < block_height_uv; ++y)
    {
        memcpy(dst_u, src_u, (block_width_uv + static_cast<int64_t>(col)) * sizeof(T));
        memcpy(dst_v, src_v, (block_width_uv + static_cast<int64_t>(col)) * sizeof(T));

        dst_u += dst_stride_uv;
        dst_v += dst_stride_uv;
        src_u += src_stride_uv;
        src_v += src_stride_uv;
    }
}

template <typename T>
static float blockLumaDiff(const T* src1_y, const T* src2_y, int block_width, int block_height, int src1_stride_y, int src2_stride_y)
{
    int diff = 0;

    for (int y = 0; y < block_height; ++y)
    {
        for (int x = 0; x < block_width; ++x)
        {
            diff += abs(src1_y[x] - src2_y[x]);
        }

        src1_y += src1_stride_y;
        src2_y += src2_stride_y;
    }

    return static_cast<float>(diff) / (block_width * block_height);
}

class Bifrost : public GenericVideoFilter
{
    int offset;
    PClip child2;
    float luma_thresh;
    int variation;
    bool conservative_mask;
    int block_width, block_height, block_width_uv, block_height_uv;
    int blocks_x, blocks_y;
    float relativeframediff;
    bool has_at_least_v8;

    template <typename T>
    PVideoFrame Framedepth(PVideoFrame& dst, PVideoFrame& altsrcc, PVideoFrame& srcnn, PVideoFrame& srcn, PVideoFrame& srcc, PVideoFrame& srcp, PVideoFrame& srcpp, IScriptEnvironment* env);

    PVideoFrame (Bifrost::*depth)(PVideoFrame& dst, PVideoFrame& altsrcc, PVideoFrame& srcnn, PVideoFrame& srcn, PVideoFrame& srcc, PVideoFrame& srcp, PVideoFrame& srcpp, IScriptEnvironment* env);

public:
    Bifrost(PClip _child, PClip _child2, float _luma_thresh, int _variation, bool _conservative_mask, bool _interlaced, int _block_width, int _block_height, IScriptEnvironment* env)
        : GenericVideoFilter(_child), child2(_child2), luma_thresh(_luma_thresh), variation(_variation), conservative_mask(_conservative_mask), block_width(_block_width), block_height(_block_height)
    {
        if (luma_thresh < 0.0f || luma_thresh > 255.0f)
            env->ThrowError("Bifrost: luma_thresh must be between 0.0..255.0.");
        if (variation < 0 || variation > 255)
            env->ThrowError("Bifrost: variation must be between 0..255.");

        relativeframediff = 1.2f;
        offset = _interlaced ? 2 : 1;
        blocks_x = vi.width / block_width;
        blocks_y = vi.height / block_height;

        if (vi.ComponentSize() == 2)
        {
            const int peak = (1 << vi.BitsPerComponent()) - 1;
            luma_thresh *= peak / 255;
            variation *= peak / 255;

            depth = &Bifrost::Framedepth<uint16_t>;
        }
        else
            depth = &Bifrost::Framedepth<uint8_t>;

        block_width_uv = block_width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
        block_height_uv = block_height >> vi.GetPlaneHeightSubsampling(PLANAR_U);
        if (block_width_uv < 2 || block_height_uv < 2)
            env->ThrowError("Bifrost: The requested block size is too small.");

        has_at_least_v8 = true;
        try { env->CheckVersion(8); }
        catch (const AvisynthError&) { has_at_least_v8 = false; };
    }

    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
};

template <typename T>
PVideoFrame Bifrost::Framedepth(PVideoFrame& dst, PVideoFrame& altsrcc, PVideoFrame& srcnn, PVideoFrame& srcn, PVideoFrame& srcc, PVideoFrame& srcp, PVideoFrame& srcpp, IScriptEnvironment* env)
{
    const T* srcpp_y = reinterpret_cast<const T*>(srcpp->GetReadPtr(PLANAR_Y));
    const T* srcpp_u = reinterpret_cast<const T*>(srcpp->GetReadPtr(PLANAR_U));
    const T* srcpp_v = reinterpret_cast<const T*>(srcpp->GetReadPtr(PLANAR_V));
          
    const T* srcp_y = reinterpret_cast<const T*>(srcp->GetReadPtr(PLANAR_Y));
    const T* srcp_u = reinterpret_cast<const T*>(srcp->GetReadPtr(PLANAR_U));
    const T* srcp_v = reinterpret_cast<const T*>(srcp->GetReadPtr(PLANAR_V));
          
    const T* srcc_y = reinterpret_cast<const T*>(srcc->GetReadPtr(PLANAR_Y));
    const T* srcc_u = reinterpret_cast<const T*>(srcc->GetReadPtr(PLANAR_U));
    const T* srcc_v = reinterpret_cast<const T*>(srcc->GetReadPtr(PLANAR_V));
          
    const T* srcn_y = reinterpret_cast<const T*>(srcn->GetReadPtr(PLANAR_Y));
    const T* srcn_u = reinterpret_cast<const T*>(srcn->GetReadPtr(PLANAR_U));
    const T* srcn_v = reinterpret_cast<const T*>(srcn->GetReadPtr(PLANAR_V));
          
    const T* srcnn_y = reinterpret_cast<const T*>(srcnn->GetReadPtr(PLANAR_Y));
    const T* srcnn_u = reinterpret_cast<const T*>(srcnn->GetReadPtr(PLANAR_U));
    const T* srcnn_v = reinterpret_cast<const T*>(srcnn->GetReadPtr(PLANAR_V));
          
    const T* altsrcc_u = reinterpret_cast<const T*>(altsrcc->GetReadPtr(PLANAR_U));
    const T* altsrcc_v = reinterpret_cast<const T*>(altsrcc->GetReadPtr(PLANAR_V));

    T* dst_u = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_U));
    T* dst_v = reinterpret_cast<T*>(dst->GetWritePtr(PLANAR_V));

    const int srcpp_pitch_y = srcpp->GetPitch(PLANAR_Y) / sizeof(T);
    const int srcpp_pitch_uv = srcpp->GetPitch(PLANAR_U) / sizeof(T);

    const int srcp_pitch_y = srcp->GetPitch(PLANAR_Y) / sizeof(T);
    const int srcp_pitch_uv = srcp->GetPitch(PLANAR_U) / sizeof(T);

    const int srcc_pitch_y = srcc->GetPitch(PLANAR_Y) / sizeof(T);
    const int srcc_pitch_uv = srcc->GetPitch(PLANAR_U) / sizeof(T);

    const int srcn_pitch_y = srcn->GetPitch(PLANAR_Y) / sizeof(T);
    const int srcn_pitch_uv = srcn->GetPitch(PLANAR_U) / sizeof(T);

    const int srcnn_pitch_y = srcnn->GetPitch(PLANAR_Y) / sizeof(T);
    const int srcnn_pitch_uv = srcnn->GetPitch(PLANAR_U) / sizeof(T);

    const int altsrcc_pitch_uv = srcc->GetPitch(PLANAR_U) / sizeof(T);

    const int dst_pitch_uv = dst->GetPitch(PLANAR_U) / sizeof(T);

    const int rowsize_y = srcc->GetRowSize(PLANAR_Y) / sizeof(T);
    const int height_y = srcc->GetHeight(PLANAR_Y);

    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y), srcc->GetReadPtr(PLANAR_Y), srcc->GetPitch(PLANAR_Y), srcc->GetRowSize(PLANAR_Y), height_y);

    const int col = (rowsize_y - rowsize_y / block_width * block_width) >> vi.GetPlaneWidthSubsampling(PLANAR_U);

    for (int y = 0; y < blocks_y; ++y)
    {
        for (int x = 0; x < blocks_x; ++x)
        {
            float ldprev = blockLumaDiff<T>(srcp_y + block_width * static_cast<int64_t>(x), srcc_y + block_width * static_cast<int64_t>(x), block_width, block_height, srcp_pitch_y, srcc_pitch_y);
            float ldnext = blockLumaDiff<T>(srcc_y + block_width * static_cast<int64_t>(x), srcn_y + block_width * static_cast<int64_t>(x), block_width, block_height, srcc_pitch_y, srcn_pitch_y);
            float ldprevprev = 0.0f;
            float ldnextnext = 0.0f;

            //too much movement in both directions?
            if (ldnext > luma_thresh && ldprev > luma_thresh)
            {
                copyChromaBlock<T>(dst_u + block_width_uv * static_cast<int64_t>(x), dst_v + block_width_uv * static_cast<int64_t>(x),
                    altsrcc_u + block_width_uv * static_cast<int64_t>(x), altsrcc_v + block_width_uv * static_cast<int64_t>(x),
                    block_width_uv, block_height_uv, dst_pitch_uv, altsrcc_pitch_uv, (x == blocks_x - 1) ? col : 0);
                continue;
            }

            if (ldnext > luma_thresh)
            {
                ldprevprev = blockLumaDiff<T>(srcpp_y + block_width * static_cast<int64_t>(x), srcp_y + block_width * static_cast<int64_t>(x), block_width, block_height, srcpp_pitch_y, srcp_pitch_y);
            }
            else if (ldprev > luma_thresh)
            {
                ldnextnext = blockLumaDiff<T>(srcn_y + block_width * static_cast<int64_t>(x), srcnn_y + block_width * static_cast<int64_t>(x), block_width, block_height, srcn_pitch_y, srcnn_pitch_y);
            }

            //two consecutive frames in one direction to generate mask?
            if ((ldnext > luma_thresh && ldprevprev > luma_thresh) ||
                (ldprev > luma_thresh && ldnextnext > luma_thresh))
            {
                copyChromaBlock<T>(dst_u + block_width_uv * static_cast<int64_t>(x), dst_v + block_width_uv * static_cast<int64_t>(x),
                    altsrcc_u + block_width_uv * static_cast<int64_t>(x), altsrcc_v + block_width_uv * static_cast<int64_t>(x),
                    block_width_uv, block_height_uv, dst_pitch_uv, altsrcc_pitch_uv, (x == blocks_x - 1) ? col : 0);
                continue;
            }

            //generate mask from correct side of scenechange
            if (ldnext > luma_thresh)
            {
                makeBlockRainbowMask<T>(srcpp_u + block_width_uv * static_cast<int64_t>(x), srcpp_v + block_width_uv * static_cast<int64_t>(x),
                    srcp_u + block_width_uv * static_cast<int64_t>(x), srcp_v + block_width_uv * static_cast<int64_t>(x),
                    srcc_u + block_width_uv * static_cast<int64_t>(x), srcc_v + block_width_uv * static_cast<int64_t>(x),
                    dst_u + block_width_uv * static_cast<int64_t>(x), dst_v + block_width_uv * static_cast<int64_t>(x),
                    block_width_uv, block_height_uv,
                    srcpp_pitch_uv, srcp_pitch_uv, srcc_pitch_uv, dst_pitch_uv,
                    variation);
            }
            else if (ldprev > luma_thresh)
            {
                makeBlockRainbowMask<T>(srcc_u + block_width_uv * static_cast<int64_t>(x), srcc_v + block_width_uv * static_cast<int64_t>(x),
                    srcn_u + block_width_uv * static_cast<int64_t>(x), srcn_v + block_width_uv * static_cast<int64_t>(x),
                    srcnn_u + block_width_uv * static_cast<int64_t>(x), srcnn_v + block_width_uv * static_cast<int64_t>(x),
                    dst_u + block_width_uv * static_cast<int64_t>(x), dst_v + block_width_uv * static_cast<int64_t>(x),
                    block_width_uv, block_height_uv,
                    srcc_pitch_uv, srcn_pitch_uv, srcnn_pitch_uv, dst_pitch_uv,
                    variation);
            }
            else
            {
                makeBlockRainbowMask<T>(srcp_u + block_width_uv * static_cast<int64_t>(x), srcp_v + block_width_uv * static_cast<int64_t>(x),
                    srcc_u + block_width_uv * static_cast<int64_t>(x), srcc_v + block_width_uv * static_cast<int64_t>(x),
                    srcn_u + block_width_uv * static_cast<int64_t>(x), srcn_v + block_width_uv * static_cast<int64_t>(x),
                    dst_u + block_width_uv * static_cast<int64_t>(x), dst_v + block_width_uv * static_cast<int64_t>(x),
                    block_width_uv, block_height_uv,
                    srcp_pitch_uv, srcc_pitch_uv, srcn_pitch_uv, dst_pitch_uv,
                    variation);
            }

            //denoise and expand mask
            processBlockRainbowMask<T>(dst_u + block_width_uv * static_cast<int64_t>(x), dst_v + block_width_uv * static_cast<int64_t>(x),
                block_width_uv, block_height_uv, dst_pitch_uv, conservative_mask);

            //determine direction to blend in
            BlendDirection direction;
            if (ldprev > ldnext * relativeframediff)
                direction = bdNext;
            else if (ldnext > ldprev * relativeframediff)
                direction = bdPrev;
            else
                direction = bdBoth;

            applyBlockRainbowMask<T>(srcp_u + block_width_uv * static_cast<int64_t>(x), srcp_v + block_width_uv * static_cast<int64_t>(x),
                srcc_u + block_width_uv * static_cast<int64_t>(x), srcc_v + block_width_uv * static_cast<int64_t>(x),
                srcn_u + block_width_uv * static_cast<int64_t>(x), srcn_v + block_width_uv * static_cast<int64_t>(x),
                dst_u + block_width_uv * static_cast<int64_t>(x), dst_v + block_width_uv * static_cast<int64_t>(x),
                block_width_uv, block_height_uv,
                srcp_pitch_uv, srcc_pitch_uv, srcn_pitch_uv, dst_pitch_uv,
                direction, (x == blocks_x - 1) ? col : 0);
        }

        srcpp_y += block_height * static_cast<int64_t>(srcpp_pitch_y);
        srcpp_u += block_height_uv * static_cast<int64_t>(srcpp_pitch_uv);
        srcpp_v += block_height_uv * static_cast<int64_t>(srcpp_pitch_uv);

        srcp_y += block_height * static_cast<int64_t>(srcp_pitch_y);
        srcp_u += block_height_uv * static_cast<int64_t>(srcp_pitch_uv);
        srcp_v += block_height_uv * static_cast<int64_t>(srcp_pitch_uv);

        srcc_y += block_height * static_cast<int64_t>(srcc_pitch_y);
        srcc_u += block_height_uv * static_cast<int64_t>(srcc_pitch_uv);
        srcc_v += block_height_uv * static_cast<int64_t>(srcc_pitch_uv);

        srcn_y += block_height * static_cast<int64_t>(srcn_pitch_y);
        srcn_u += block_height_uv * static_cast<int64_t>(srcn_pitch_uv);
        srcn_v += block_height_uv * static_cast<int64_t>(srcn_pitch_uv);

        srcnn_y += block_height * static_cast<int64_t>(srcnn_pitch_y);
        srcnn_u += block_height_uv * static_cast<int64_t>(srcnn_pitch_uv);
        srcnn_v += block_height_uv * static_cast<int64_t>(srcnn_pitch_uv);

        altsrcc_u += block_height_uv * static_cast<int64_t>(altsrcc_pitch_uv);
        altsrcc_v += block_height_uv * static_cast<int64_t>(altsrcc_pitch_uv);

        dst_u += block_height_uv * static_cast<int64_t>(dst_pitch_uv);
        dst_v += block_height_uv * static_cast<int64_t>(dst_pitch_uv);
    }

    const int row = height_y / block_height * block_height;
    if (row != height_y)
    {
        const int width_uv = srcc->GetRowSize(PLANAR_U);
        const int height_uv = srcc->GetHeight(PLANAR_U);
        const int h = row >> vi.GetPlaneHeightSubsampling(PLANAR_U);

        for (int y = h; y < height_uv; ++y)
        {
            memcpy(dst_u, srcc_u, width_uv);
            memcpy(dst_v, srcc_v, width_uv);

            srcc_u += srcc_pitch_uv;
            srcc_v += srcc_pitch_uv;
            dst_u += dst_pitch_uv;
            dst_v += dst_pitch_uv;
        }
    }

    return dst;
}


PVideoFrame __stdcall Bifrost::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame srcpp = child->GetFrame(std::max(n - offset * 2, 0), env);
    PVideoFrame srcp = child->GetFrame(std::max(n - offset, 0), env);
    PVideoFrame srcc = child->GetFrame(n, env);
    PVideoFrame srcn = child->GetFrame(std::min(n + offset, vi.num_frames - 1), env);
    PVideoFrame srcnn = child->GetFrame(std::min(n + offset * 2, vi.num_frames - 1), env);

    PVideoFrame altsrcc = child2->GetFrame(n, env);

    PVideoFrame dst = (has_at_least_v8) ? env->NewVideoFrameP(vi, &srcc) : env->NewVideoFrame(vi);

    return (this->*depth)(dst, altsrcc, srcnn, srcn, srcc, srcp, srcpp, env);
}

AVSValue __cdecl Create_Bifrost(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    PClip clip = args[0].AsClip();
    const VideoInfo& vi = clip->GetVideoInfo();

    if (!vi.IsPlanar() || vi.IsRGB() || vi.NumComponents() != 3 || vi.BitsPerComponent() == 32)
        env->ThrowError("Bifrost: clip must be in YUV 8..16-bit planar format and must have 3 planes.");

    const int blockx = args[6].AsInt(4);
    const int blocky = args[7].AsInt(4);

    if (blockx % (1 << vi.GetPlaneWidthSubsampling(PLANAR_U)) || blocky % (1 << vi.GetPlaneHeightSubsampling(PLANAR_U)))
        env->ThrowError("Bifrost: The requested block size is incompatible with the clip's subsampling.");

    PClip InClip = [&]()
    {
        if (args[1].Defined())
        {
            const VideoInfo& vi2 = args[1].AsClip()->GetVideoInfo();

            if (vi.IsSameColorspace(vi2) || vi.width != vi2.width || vi.height != vi2.height || vi.num_frames != vi2.num_frames)
                env->ThrowError("Bifrost: The two clips must have the same pixel type, dimensions and length.");

            return args[1].AsClip();
        }
        else
            return clip;
    }();

    if (args[5].AsBool(true))
    {
        InClip = env->Invoke("SeparateFields", InClip).AsClip();
        return env->Invoke("Weave", new Bifrost(env->Invoke("SeparateFields", clip).AsClip(), InClip, (float)args[2].AsFloat(10.0), args[3].AsInt(5), args[4].AsBool(false), true, blockx, blocky, env));
    }
    else
        return new Bifrost(clip, InClip, (float)args[2].AsFloat(10.0), args[3].AsInt(5), args[4].AsBool(false), false, blockx, blocky, env);
}

const AVS_Linkage* AVS_linkage;

extern "C" __declspec(dllexport)
const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    env->AddFunction("Bifrost", "c[altclip]c[luma_thresh]f[variation]i[conservative_mask]b[interlaced]b[blockx]i[blocky]i", Create_Bifrost, 0);

    return 0;
};

