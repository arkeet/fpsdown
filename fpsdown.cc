/*
Copyright 2011-2012 Adrian Keet

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "windows.h"
#include "Avisynth.h"

#include <map>
#include <cmath>

inline unsigned char blendfunc(unsigned char x1, unsigned char x2, double gamma)
{
    double g1 = pow(x1 / 255.0, gamma);
    double g2 = pow(x2 / 255.0, gamma);
    double blend = (g1 + g2) / 2;
    return (unsigned char)(pow(blend, 1/gamma) * 255.0);
}

class FpsDownFilter : public GenericVideoFilter
{
private:
    std::map<int, bool> frameDiffer;
    bool CompareFrames(int n, IScriptEnvironment* env);
    unsigned int threshold;
    bool debugShow;
    bool maxNorm;
    bool adjust3xSlowdown;

    unsigned char blendtable[256][256];
    inline unsigned char blend(unsigned char x1, unsigned char x2);

public:
    FpsDownFilter(PClip _child, bool _maxNorm, int _threshold, bool _debugShow, bool _adjust3xSlowdown, double _gamma, const IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

FpsDownFilter::FpsDownFilter(PClip _child, bool _maxNorm, int _threshold, bool _debugShow, bool _adjust3xSlowdown, double _gamma, const IScriptEnvironment* env)
    : GenericVideoFilter(_child)
{
    threshold = _threshold;
    debugShow = _debugShow;
    maxNorm = _maxNorm;
    vi.fps_denominator *= 2;
    vi.num_frames /= 2;

    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            blendtable[i][j] = blendfunc(i, j, _gamma);
        }
    }
}

bool FpsDownFilter::CompareFrames(int n, IScriptEnvironment* env)
{
    std::map<int, bool>::iterator it = frameDiffer.find(n);
    if (it != frameDiffer.end())
        return it->second;

    PVideoFrame f1 = child->GetFrame(n, env);
    PVideoFrame f2 = child->GetFrame(n + 1, env);

    const unsigned char* p1 = f1->GetReadPtr();
    const unsigned char* p2 = f2->GetReadPtr();

    const int pitch = f1->GetPitch();
    const int width = f1->GetRowSize();
    const int height = f1->GetHeight();

    unsigned int diff = 0;

    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width / 4; w++) {
            unsigned char* bgra1 = (unsigned char*) ((unsigned int *)p1 + w);
            unsigned char* bgra2 = (unsigned char*) ((unsigned int *)p2 + w);
            unsigned int pixelDiff =
                max(max(abs((int)bgra1[0] - (int)bgra2[0]),
                abs((int)bgra1[1] - (int)bgra2[1])),
                abs((int)bgra1[2] - (int)bgra2[2]));

            if (maxNorm) {
                diff = pixelDiff;
            } else {
                diff += pixelDiff;
            }

            if (diff >= threshold) {
                return frameDiffer[n] = true;
            }
        }
        p1 += pitch;
        p2 += pitch;
    }
    return frameDiffer[n] = false;
}

inline unsigned char FpsDownFilter::blend(unsigned char x1, unsigned char x2)
{
    return blendtable[x1][x2];
}

PVideoFrame __stdcall FpsDownFilter::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src1 = child->GetFrame(2*n    , env);
    PVideoFrame src2 = child->GetFrame(2*n + 1, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    const unsigned char* srcp1 = src1->GetReadPtr();
    const unsigned char* srcp2 = src2->GetReadPtr();
    unsigned char* dstp = dst->GetWritePtr();

    const int src_pitch = src1->GetPitch();
    const int src_width = src1->GetRowSize();
    const int src_height = src1->GetHeight();

    const int dst_pitch = dst->GetPitch();
    const int dst_width = dst->GetRowSize();
    const int dst_height = dst->GetHeight();

    if (!vi.IsRGB32()) {
        env->ThrowError("FpsDown: input to filter must be in RGB32");
    }

    bool mode = CompareFrames(2*n + 1, env);
    if (adjust3xSlowdown && !mode && !CompareFrames(2*n - 1, env) &&
            CompareFrames(2*n + 3, env) && !CompareFrames(2*n + 5, env))
    {
        mode = true;
    }

    for (int h = 0; h < src_height; h++) {
        for (int w = 0; w < src_width / 4; w++) {
            unsigned char* bgra1 = (unsigned char*) ((unsigned int *)srcp1 + w);
            unsigned char* bgra2 = (unsigned char*) ((unsigned int *)srcp2 + w);
            unsigned char* bgrad = (unsigned char*) ((unsigned int *)dstp + w);

            if (mode) {
                bgrad[0] = blend(bgra1[0], bgra2[0]);
                bgrad[1] = blend(bgra1[1], bgra2[1]);
                bgrad[2] = blend(bgra1[2], bgra2[2]);
                bgrad[3] = blend(bgra1[3], bgra2[3]);
                if (debugShow && w < 32 && h < 32) {
                    *(unsigned int *)bgrad = ~0;
                }
            } else {
                bgrad[0] = bgra1[0];
                bgrad[1] = bgra1[1];
                bgrad[2] = bgra1[2];
                bgrad[3] = bgra1[3];
                if (debugShow && w < 32 && h < 32) {
                    *(unsigned int *)bgrad = 0;
                }
            }
        }
        srcp1 += src_pitch;
        srcp2 += src_pitch;
        dstp += dst_pitch;
    }

    return dst;
}

AVSValue __cdecl Create_FpsDownFilter(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    bool maxnorm = args[1].AsBool(true);
    return new FpsDownFilter(
        args[0].AsClip(),
        maxnorm,
        args[2].AsInt(maxnorm ? 8 : 2000000),
        args[3].AsBool(false),
        args[4].AsBool(false),
        args[5].AsFloat(2.2),
        env);
}

extern "C" __declspec(dllexport) const char* _stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("FpsDown", "c[maxnorm]b[threshold]i[debug]b[adjust3xslowdown]b[gamma]f", Create_FpsDownFilter, 0);
    return "`FpsDown' FpsDown plugin";
}
