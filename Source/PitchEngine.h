#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstdint>

// MADENCITUNE DSP v2
//  Yol A (varsayilan): periyot-senkron gecikme hatti kaydirici -> gecikme ~ yarim periyot (2-5 ms), temiz ses.
//  Yol B (formant / madenci robot modu): PSOLA granuler kaydirici, formant bagimsiz + "hold" ile temiz robot ses.
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
    static constexpr int kBuf = 1 << 15, kMask = kBuf - 1;

    std::atomic<float> detMidi { -1.f }, tgtMidi { -1.f };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int c = 0; c < 2; ++c) { in[c].assign (kBuf, 0.f); out[c].assign (kBuf, 0.f); lpA[c] = lpB[c] = 0.f; }
        mono.assign (kBuf, 0.f); seg.assign (kBuf, 0.f); dfn.assign (kBuf, 0.f);
        t = 0; hop = 0; hopLen = 128; win = 512;
        period = sr / 200.0; ratio = ratioCur = 1.f; slow = 60.f; wasVoiced = false; voicedNow = false;
        curNote = -1000; corrSm = 0.f; transTimer = 0.f; fr = 1.f; holdN = 1; hardK = 0.f;
        dCur = dOld = 8.0; fadePos = fadeLen = 0;
        nextGrain = 0.0; bActive = false; wB = 0.f; prevMark = -1e9; heldValid = false; holdLeft = 0; heldMark = 0.0;
        latencyB = (int) std::ceil (3.3 * sr / 70.0) + 8;
        rCoef = 1.f - std::exp (-1.f / (0.0015f * (float) sr));
        wCoef = 1.f - std::exp (-1.f / (0.012f * (float) sr));
    }

    // Raporlanan gecikme: yol A degisken (~yarim periyot) oldugu icin 0 bildiriyoruz.
    int getLatency() const { return 0; }

    void process (float* const* ch, int nch, int n, const EngineParams& p)
    {
        updateFx (p);
        if (useB && ! bActive) { bActive = true; nextGrain = (double) t; prevMark = -1e9; heldValid = false; holdLeft = 0; }

        for (int i = 0; i < n; ++i)
        {
            float m = 0.f;
            for (int c = 0; c < nch; ++c) { in[c][(size_t) (t & kMask)] = ch[c][i]; m += ch[c][i]; }
            mono[(size_t) (t & kMask)] = m / (float) nch;
            ++t;
            if (++hop >= hopLen) { hop = 0; analyse (p); }
            const int64_t ot = t - 1;

            ratioCur += (ratio - ratioCur) * rCoef;
            wB += ((useB ? 1.f : 0.f) - wB) * wCoef;
            if (! useB && bActive && wB < 1e-4f) bActive = false;

            // ---------- YOL A: periyot-senkron gecikme hatti ----------
            {
                const double step = 1.0 - (double) ratioCur;
                dCur += step;
                if (fadePos < fadeLen) dOld += step;
                const double T = std::max (20.0, period);
                const int fadeTarget = (int) std::max (32.0, std::min (std::round (0.5 * T), std::round (0.004 * sr)));
                const double dMin = 4.0 + (double) fadeTarget * (double) std::max (0.f, std::min (1.f, ratioCur - 1.f));
                if (fadePos >= fadeLen)
                {
                    if (dCur < dMin)               { dOld = dCur; dCur += T; fadeLen = fadeTarget; fadePos = 0; }
                    else if (dCur > dMin + T + 1.0) { dOld = dCur; dCur -= T; fadeLen = fadeTarget; fadePos = 0; }
                }
                dCur = std::max (2.5, std::min ((double) (kBuf / 2), dCur));
                dOld = std::max (2.5, std::min ((double) (kBuf / 2), dOld));
            }
            float wA = 1.f;
            if (fadePos < fadeLen)
            {
                wA = 0.5f - 0.5f * std::cos (3.14159265f * (float) (fadePos + 1) / (float) fadeLen);
                ++fadePos;
            }

            // ---------- YOL B: PSOLA ----------
            if (bActive)
            {
                for (;;)
                {
                    const double Tin = std::max (20.0, period);
                    const int N = std::max (16, (int) std::lround (2.0 * Tin));
                    const double half = 0.5 * N;
                    if (nextGrain - half > (double) ot) break;
                    synthB (nch, N, half, Tin, ot);
                    nextGrain += Tin / (double) std::max (0.5f, ratio);
                }
            }

            for (int c = 0; c < nch; ++c)
            {
                const size_t oi = (size_t) (ot & kMask);
                const float bOut = out[c][oi]; out[c][oi] = 0.f;

                float a = readTap (c, dCur);
                if (wA < 1.f) a = (1.f - wA) * readTap (c, dOld) + wA * a;
                float x = (1.f - wB) * a + wB * bOut;

                if (madenci > 0.f)
                {
                    lpA[c] += lpCoefLow * (x - lpA[c]);
                    x += madenci * 0.9f * lpA[c];
                    x = std::tanh (x * (1.f + 1.2f * madenci)) / (1.f + 0.35f * madenci);
                }
                if (tech > 0.f)
                {
                    lpB[c] += lpCoefHigh * (x - lpB[c]);
                    x += tech * 0.5f * (x - lpB[c]);
                    x = std::round (x * levels) / levels;
                }
                const double dryDelay = (1.0 - (double) wB) * dCur + (double) wB * (double) latencyB;
                const float dry = hermite (in[c], (double) (t - 1) - dryDelay);
                ch[c][i] = (dry * (1.f - p.mix) + x * p.mix) * p.outGain;
            }
        }
    }

private:
    double sr = 44100.0;
    std::vector<float> in[2], out[2], mono, seg, dfn;
    int hop = 0, hopLen = 128, win = 512, latencyB = 2048;
    int64_t t = 0;
    double period = 220.0, nextGrain = 0.0, dCur = 8.0, dOld = 8.0, prevMark = -1e9, heldMark = 0.0;
    int fadePos = 0, fadeLen = 0, holdN = 1, holdLeft = 0, curNote = -1000;
    float ratio = 1.f, ratioCur = 1.f, slow = 60.f, corrSm = 0.f, transTimer = 0.f, fr = 1.f, hardK = 0.f, wB = 0.f, rCoef = 0.01f, wCoef = 0.001f;
    bool wasVoiced = false, voicedNow = false, bActive = false, useB = false, heldValid = false;
    float madenci = 0.f, tech = 0.f, levels = 32768.f, lpCoefLow = 0.f, lpCoefHigh = 0.f, lpA[2] {}, lpB[2] {};

    void updateFx (const EngineParams& p)
    {
        fr = std::pow (2.f, p.formantSemis / 12.f);
        madenci = p.formantSemis < -6.f ? std::min (1.f, (-p.formantSemis - 6.f) / 6.f) : 0.f;
        tech = p.formantSemis > 0.f ? std::min (1.f, p.formantSemis / 12.f) : 0.f;
        levels = std::pow (2.f, 15.f - 7.f * tech);
        lpCoefLow = 1.f - std::exp (-6.2831853f * 180.f / (float) sr);
        lpCoefHigh = 1.f - std::exp (-6.2831853f * 3000.f / (float) sr);
        hardK = std::max (0.f, std::min (1.f, -p.speedMs / 10.f));
        holdN = 1 + (int) std::lround (hardK * 3.f);
        useB = std::fabs (p.formantSemis) > 0.05f || holdN > 1;
        latencyB = (int) std::ceil (3.3 * sr / (double) std::max (40.f, p.fMin)) + 8;
        const int tauMaxL = (int) (sr / (double) p.fMin);
        win = std::max (320, std::min (1024, (int) (1.15 * tauMaxL)));
        hopLen = std::max (96, win / 4);
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

    float readTap (int c, double d) const { return hermite (in[c], (double) (t - 1) - d); }

    void synthB (int nch, int N, double half, double Tin, int64_t ot)
    {
        double cIn = nextGrain - latencyB;
        if (voicedNow)
        {
            if (heldValid && holdLeft > 0) { cIn = heldMark; --holdLeft; }
            else
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
                cIn = cand; prevMark = cand; heldMark = cand; heldValid = true; holdLeft = holdN - 1;
            }
        }
        else { prevMark = -1e9; heldValid = false; holdLeft = 0; }

        const int64_t startOut = (int64_t) std::floor (nextGrain - half);
        const float scale = std::min (3.f, std::max (0.3f, (float) (2.0 * (Tin / std::max (0.5f, ratio)) / N)));
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
        const double dt = (double) hopLen / sr;
        double e = 0.0; const int eN = 256;
        for (int j = 0; j < eN; ++j) { const float x = mono[(size_t) ((t - 1 - j) & kMask)]; e += (double) x * x; }
        const double rms = std::sqrt (e / eN);

        // tespit icin decimation (CPU): ~24 kHz alan
        const int dec = std::max (1, (int) (sr / 24000.0));
        const double srD = sr / dec;
        const int tauMin = std::max (2, (int) (srD / p.fMax));
        const int tauMax = (int) (srD / p.fMin);
        const int winD = win / dec;
        float meas = 0.f; bool voiced = false;

        if (rms > 0.002 && tauMax > tauMin + 3 && winD > 16)
        {
            const int total = winD + tauMax;
            for (int j = 0; j < total; ++j)
            {
                const int64_t base = t - (int64_t) (total - j) * dec;
                float s = 0.f; for (int q = 0; q < dec; ++q) s += mono[(size_t) ((base + q) & kMask)];
                seg[(size_t) j] = s / (float) dec;
            }
            dfn[0] = 1.f; float run = 0.f;
            for (int tau = 1; tau < tauMax; ++tau)
            {
                float d = 0.f;
                const float* a = &seg[(size_t) tauMax];
                const float* b = &seg[(size_t) (tauMax - tau)];
                for (int j = 0; j < winD; ++j) { const float df = a[j] - b[j]; d += df * df; }
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
                meas = ((float) best + shift) * (float) dec; voiced = true;
            }
        }

        const float k = hardK;
        if (voiced)
        {
            const float m = (float) (69.0 + 12.0 * std::log2 (sr / (double) meas / (double) p.refHz));
            period += ((double) meas - period) * (wasVoiced ? 0.6 : 1.0);
            period = std::max (sr / 1200.0, std::min (sr / (double) std::max (40.f, p.fMin), period));

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
            ratio = std::pow (2.f, semis / 12.f);
            detMidi.store (m); tgtMidi.store (desired);
        }
        else
        {
            ratio = 1.f; corrSm = 0.f;
            detMidi.store (-1.f); tgtMidi.store (-1.f);
        }
        wasVoiced = voiced; voicedNow = voiced;
        ratio = std::max (0.5f, std::min (2.f, ratio));
    }
};
