#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstdint>

// MADENCITUNE DSP: YIN pitch tespiti + pitch-senkron granuler (PSOLA benzeri) pitch/formant kaydirici.
struct EngineParams
{
    float speedMs = 10.f, transitionMs = 0.f, toleranceCents = 0.f;
    float vibrato = 1.f, formantSemis = 0.f, amount = 1.f;
    float refHz = 440.f, fMin = 70.f, fMax = 1000.f;
    float mix = 1.f, outGain = 1.f;
    int allowedMask = 0xFFF, heldMask = 0;
};

class PitchEngine
{
public:
    static constexpr int kBuf = 1 << 15, kMask = kBuf - 1, kHop = 320, kWin = 800;

    std::atomic<float> detMidi { -1.f }, tgtMidi { -1.f };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c) { in[c].assign (kBuf, 0.f); out[c].assign (kBuf, 0.f); lpA[c] = lpB[c] = 0.f; }
        mono.assign (kBuf, 0.f); seg.assign (kBuf, 0.f); dfn.assign (kBuf, 0.f);
        latency = (int) std::ceil (3.5 * sr / 65.0) + 8;
        t = 0; hop = 0; nextGrain = 0.0;
        period = sr / 200.0; ratio = ratioT = 1.f; slow = 60.f; wasVoiced = false;
        curNote = -1000; corrSm = 0.f; transTimer = 0.f; lenF = 1.f; fr = 1.f; prevMark = -1e9; voicedNow = false;
    }

    int getLatency() const { return latency; }

    void process (float* const* ch, int nch, int n, const EngineParams& p)
    {
        updateFx (p);
        for (int i = 0; i < n; ++i)
        {
            float m = 0.f;
            for (int c = 0; c < nch; ++c) { in[c][(size_t) (t & kMask)] = ch[c][i]; m += ch[c][i]; }
            mono[(size_t) (t & kMask)] = m / (float) nch;
            ++t;
            if (++hop >= kHop) { hop = 0; analyse (p); }

            const int64_t ot = t - 1;
            for (;;)
            {
                const double Tin = period;
                const int N = std::max (16, (int) std::lround (2.0 * Tin * lenF));
                const double half = 0.5 * N;
                if (nextGrain - half > (double) ot) break;
                synth (nch, N, half, Tin, ot);
                nextGrain += Tin / (double) ratio;
            }

            for (int c = 0; c < nch; ++c)
            {
                const size_t oi = (size_t) (ot & kMask);
                float x = out[c][oi]; out[c][oi] = 0.f;
                if (madenci > 0.f)
                {
                    lpA[c] += lpCoefLow * (x - lpA[c]);
                    x += madenci * 1.4f * lpA[c];
                    x = std::tanh (x * (1.f + 2.f * madenci)) / (1.f + 0.5f * madenci);
                }
                if (tech > 0.f)
                {
                    lpB[c] += lpCoefHigh * (x - lpB[c]);
                    x += tech * 0.6f * (x - lpB[c]);
                    x = std::round (x * levels) / levels;
                }
                const float dry = in[c][(size_t) ((ot - latency) & kMask)];
                ch[c][i] = (dry * (1.f - p.mix) + x * p.mix) * p.outGain;
            }
        }
    }

private:
    double sr = 44100.0;
    std::vector<float> in[2], out[2], mono, seg, dfn;
    int latency = 2048, hop = 0;
    int64_t t = 0;
    double nextGrain = 0.0, period = 220.0;
    float ratio = 1.f, ratioT = 1.f, slow = 60.f, corrSm = 0.f, transTimer = 0.f, lenF = 1.f, fr = 1.f;
    bool wasVoiced = false, voicedNow = false;
    double prevMark = -1e9;
    int curNote = -1000;
    float madenci = 0.f, tech = 0.f, levels = 32768.f, lpCoefLow = 0.f, lpCoefHigh = 0.f, lpA[2] {}, lpB[2] {};

    void updateFx (const EngineParams& p)
    {
        fr = std::pow (2.f, p.formantSemis / 12.f);
        madenci = p.formantSemis < -6.f ? std::min (1.f, (-p.formantSemis - 6.f) / 6.f) : 0.f;
        tech = p.formantSemis > 0.f ? std::min (1.f, p.formantSemis / 12.f) : 0.f;
        levels = std::pow (2.f, 15.f - 8.f * tech);
        lpCoefLow = 1.f - std::exp (-6.2831853f * 180.f / (float) sr);
        lpCoefHigh = 1.f - std::exp (-6.2831853f * 3000.f / (float) sr);
    }

    static float hermite (const std::vector<float>& b, double pos)
    {
        const int64_t i = (int64_t) std::floor (pos);
        const float f = (float) (pos - (double) i);
        const float y0 = b[(size_t) ((i - 1) & kMask)], y1 = b[(size_t) (i & kMask)],
                    y2 = b[(size_t) ((i + 1) & kMask)], y3 = b[(size_t) ((i + 2) & kMask)];
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3,
                    c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }

    void synth (int nch, int N, double half, double Tin, int64_t ot)
    {
        // Pitch isareti (mark) hizalamasi: analiz merkezini periyodik latise oturt, tepe noktasina ince ayar yap
        double cIn = nextGrain - latency;
        if (voicedNow)
        {
            double cand = cIn;
            if (prevMark > -1e8) cand = prevMark + std::round ((cIn - prevMark) / Tin) * Tin;
            const int64_t lo = (int64_t) std::floor (cand - 0.15 * Tin), hi = (int64_t) std::ceil (cand + 0.15 * Tin);
            if (hi < t - 3 && lo > 0)
            {
                int64_t bi = lo; float bv = -1e9f;
                for (int64_t j = lo; j <= hi; ++j) { const float v = mono[(size_t) (j & kMask)]; if (v > bv) { bv = v; bi = j; } }
                cand = (double) bi;
            }
            cIn = cand; prevMark = cand;
        }
        else prevMark = -1e9;
        const int64_t startOut = (int64_t) std::floor (nextGrain - half);
        const float scale = std::min (3.f, std::max (0.3f, (float) (2.0 * (Tin / ratio) / N)));
        const double availMax = (double) (t - 3);
        for (int n = 0; n < N; ++n)
        {
            const int64_t oi = startOut + n;
            if (oi < ot) continue;
            const float w = 0.5f - 0.5f * std::cos (6.2831853f * ((float) n + 0.5f) / (float) N);
            double src = cIn + ((double) n - half) * fr;
            if (src > availMax) src = availMax;
            for (int c = 0; c < nch; ++c)
                out[c][(size_t) (oi & kMask)] += w * scale * hermite (in[c], src);
        }
    }

    void analyse (const EngineParams& p)
    {
        const double dt = (double) kHop / sr;
        double e = 0.0;
        for (int j = 0; j < kWin; ++j) { const float x = mono[(size_t) ((t - 1 - j) & kMask)]; e += (double) x * x; }
        const double rms = std::sqrt (e / kWin);

        const int tauMin = std::max (2, (int) (sr / p.fMax));
        const int tauMax = std::min ((int) (sr / p.fMin), (int) (sr / 65.0));
        float meas = 0.f; bool voiced = false;

        if (rms > 0.002 && tauMax > tauMin + 3)
        {
            const int total = kWin + tauMax;
            for (int j = 0; j < total; ++j) seg[(size_t) j] = mono[(size_t) ((t - total + j) & kMask)];
            dfn[0] = 1.f; float run = 0.f;
            for (int tau = 1; tau < tauMax; ++tau)
            {
                float d = 0.f;
                for (int j = 0; j < kWin; ++j) { const float df = seg[(size_t) (tauMax + j)] - seg[(size_t) (tauMax + j - tau)]; d += df * df; }
                run += d;
                dfn[(size_t) tau] = run > 0.f ? d * (float) tau / run : 1.f;
            }
            int best = -1, minT = -1; float minV = 1e9f;
            for (int tau = tauMin; tau < tauMax - 1; ++tau)
            {
                if (dfn[(size_t) tau] < minV) { minV = dfn[(size_t) tau]; minT = tau; }
                if (dfn[(size_t) tau] < 0.15f)
                {
                    while (tau + 1 < tauMax - 1 && dfn[(size_t) tau + 1] < dfn[(size_t) tau]) ++tau;
                    best = tau; break;
                }
            }
            if (best < 0 && minV < 0.3f) best = minT;
            if (best > 1)
            {
                const float a = dfn[(size_t) best - 1], b = dfn[(size_t) best], c2 = dfn[(size_t) best + 1];
                const float den = a - 2.f * b + c2;
                float shift = std::fabs (den) > 1e-9f ? 0.5f * (a - c2) / den : 0.f;
                shift = std::max (-1.f, std::min (1.f, shift));
                meas = (float) best + shift; voiced = true;
            }
        }

        const float k = std::max (0.f, std::min (1.f, -p.speedMs / 10.f)); // negatif speed = ekstra sert
        lenF = 1.f - 0.45f * k;

        if (voiced)
        {
            const float m = (float) (69.0 + 12.0 * std::log2 (sr / (double) meas / (double) p.refHz));
            period += ((double) meas - period) * (wasVoiced ? 0.6 : 1.0);
            period = std::max (sr / 1200.0, std::min (sr / 65.0, period));

            const float aSlow = (float) (1.0 - std::exp (-dt / 0.08));
            if (! wasVoiced || std::fabs (m - slow) > 1.2f) { slow = m; corrSm = 0.f; }
            else slow += (m - slow) * aSlow;
            const float vib = (m - slow) * (1.f - k);

            int allowed = p.heldMask ? p.heldMask : p.allowedMask;
            if ((allowed & 0xFFF) == 0) allowed = 0xFFF;
            const int base = (int) std::floor (slow);
            int bn = base; float bd = 1e9f;
            for (int kk = base - 7; kk <= base + 8; ++kk)
            {
                const int pc = ((kk % 12) + 12) % 12;
                if (allowed & (1 << pc)) { const float d = std::fabs (slow - (float) kk); if (d < bd) { bd = d; bn = kk; } }
            }
            const float hyst = 0.12f * (1.f - k);
            if (curNote != -1000 && bn != curNote && (allowed & (1 << (((curNote % 12) + 12) % 12)))
                && bd > std::fabs (slow - (float) curNote) - hyst)
                bn = curNote;
            if (bn != curNote) { curNote = bn; transTimer = p.transitionMs * 0.001f; }
            transTimer -= (float) dt;

            const float corr = (std::fabs (slow - (float) curNote) * 100.f < p.toleranceCents) ? 0.f : ((float) curNote - slow);
            double tau = p.speedMs > 0.f ? (double) p.speedMs * 0.001 : 0.0;
            if (transTimer > 0.f) tau = std::max (tau, (double) p.transitionMs * 0.001);
            const float aC = tau < 1e-4 ? 1.f : (float) (1.0 - std::exp (-dt / tau));
            corrSm += (corr - corrSm) * aC;

            const float desired = slow + corrSm * p.amount + vib * p.vibrato;
            const float semis = std::max (-12.f, std::min (12.f, desired - m));
            ratioT = std::pow (2.f, semis / 12.f);
            detMidi.store (m); tgtMidi.store (desired);
        }
        else
        {
            ratioT = 1.f; corrSm = 0.f;
            detMidi.store (-1.f); tgtMidi.store (-1.f);
        }
        wasVoiced = voiced; voicedNow = voiced;
        ratio += (ratioT - ratio) * 0.6f;
        ratio = std::max (0.5f, std::min (2.f, ratio));
    }
};
