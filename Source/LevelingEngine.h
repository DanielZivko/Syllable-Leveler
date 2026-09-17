#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace syllable {
// All indices are in ORIGINAL SOURCE samples. End is exclusive.
struct Segment {
    std::size_t start = 0, end = 0;
    double rmsDb = -120.0, peakDb = -120.0, gainDb = 0.0;
};
struct Settings {
    double amount = 0.5;
    double maxBoostDb = 6.0, maxCutDb = 12.0;
    double floorDb = -48.0;
    double minSegmentMs = 100.0, silenceMs = 50.0;
    double valleyDepthDb = 5.0, transitionMs = 12.0;
    double ceilingDb = -1.0;
};
struct Analysis {
    double sampleRate = 0.0, targetDb = -120.0;
    std::size_t sampleCount = 0;
    Settings settings;
    std::vector<Segment> segments;
};
inline double db(double amplitude) {
    return 20.0 * std::log10(std::max(amplitude, 1.0e-6));
}
inline void validate(const Settings& s) {
    const double values[] = { s.amount, s.maxBoostDb, s.maxCutDb, s.floorDb,
        s.minSegmentMs, s.silenceMs, s.valleyDepthDb, s.transitionMs, s.ceilingDb };
    for (double v : values) if (!std::isfinite(v)) throw std::invalid_argument("Non-finite setting");
    if (s.amount < 0 || s.amount > 1 || s.maxBoostDb < 0 || s.maxBoostDb > 24
        || s.maxCutDb < 0 || s.maxCutDb > 48 || s.floorDb < -100 || s.floorDb > -6
        || s.minSegmentMs < 30 || s.minSegmentMs > 2000 || s.silenceMs < 10
        || s.silenceMs > 2000 || s.valleyDepthDb < 1 || s.valleyDepthDb > 40
        || s.transitionMs < 1 || s.transitionMs > 100 || s.ceilingDb > 0
        || s.ceilingDb < -24) throw std::invalid_argument("Setting outside supported range");
}
inline void recalculate(Analysis& a, const Settings& settings) {
    validate(settings);
    a.settings = settings;
    std::vector<double> levels;
    for (const auto& s : a.segments) levels.push_back(s.rmsDb);
    if (levels.empty()) { a.targetDb = -120; return; }
    std::sort(levels.begin(), levels.end());
    const auto middle = levels.size() / 2;
    a.targetDb = levels.size() % 2 ? levels[middle] : (levels[middle-1] + levels[middle]) / 2;
    for (auto& s : a.segments) {
        const double requested = std::clamp(settings.amount * (a.targetDb - s.rmsDb),
                                            -settings.maxCutDb, settings.maxBoostDb);
        // Limit POSITIVE gain by the segment's measured sample peak. This is not a
        // true-peak limiter. Bypass/amount=0 never attenuates the original signal.
        s.gainDb = std::min(requested, std::max(0.0, settings.ceilingDb - s.peakDb));
    }
}

// A conservative acoustic baseline, NOT a phoneme/syllable recognizer.
// No pitch-triggered splitting; sustained vowels can remain long segments.
inline Analysis analyse(const float* const* channels, int channelCount,
                        std::size_t count, double sampleRate, Settings settings = {}) {
    validate(settings);
    if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 384000
        || channelCount < 1 || channelCount > 2 || channels == nullptr)
        throw std::invalid_argument("Expected mono/stereo audio at 8-384 kHz");
    for (int c = 0; c < channelCount; ++c)
        if (!channels[c]) throw std::invalid_argument("Null audio channel");
    Analysis a; a.sampleRate = sampleRate; a.sampleCount = count; a.settings = settings;
    if (count == 0) return a;
    const std::size_t hop = std::max<std::size_t>(1, std::llround(sampleRate * .01));
    const auto frames = (count + hop - 1) / hop;
    std::vector<double> power(frames), level(frames);
    for (std::size_t f = 0; f < frames; ++f) {
        const auto begin = f * hop, end = std::min(count, begin + hop);
        double sum = 0;
        for (int c = 0; c < channelCount; ++c)
            for (std::size_t n = begin; n < end; ++n) {
                const double x = channels[c][n];
                if (!std::isfinite(x)) throw std::invalid_argument("Non-finite audio sample");
                sum += x*x;
            }
        power[f] = sum / static_cast<double>((end-begin) * channelCount);
    }
    for (std::size_t f = 0; f < frames; ++f) {
        const auto l = f ? f-1 : f, r = std::min(frames-1, f+1);
        double sum = 0;
        for (auto j = l; j <= r; ++j) sum += power[j];
        level[f] = db(std::sqrt(sum / static_cast<double>(r-l+1)));
    }
    const std::size_t minFrames = std::max<std::size_t>(3, std::llround(settings.minSegmentMs / 10));
    const std::size_t quietFrames = std::max<std::size_t>(1, std::llround(settings.silenceMs / 10));
    const auto addSegment = [&](std::size_t beginFrame, std::size_t endFrame) {
        Segment s; s.start = beginFrame * hop; s.end = std::min(count, endFrame * hop);
        if (s.end <= s.start) return;
        // Gate RMS measurement at frame level; preserve low-level edges in playback.
        double energy = 0, peak = 0; std::size_t measured = 0;
        for (std::size_t f = beginFrame; f < endFrame; ++f) {
            const bool active = db(std::sqrt(power[f])) >= settings.floorDb;
            for (auto n = f * hop; n < std::min(count, (f+1)*hop); ++n)
                for (int c = 0; c < channelCount; ++c) {
                    const double x = channels[c][n]; peak = std::max(peak, std::abs(x));
                    if (active) { energy += x*x; ++measured; }
                }
        }
        if (measured == 0) return;
        s.rmsDb = db(std::sqrt(energy / static_cast<double>(measured)));
        s.peakDb = db(peak); a.segments.push_back(s);
    };
    const auto splitIsland = [&](std::size_t begin, std::size_t end) {
        if ((end-begin) * hop < sampleRate * .03) return; // Ignore isolated clicks.
        std::size_t last = begin;
        for (auto f = begin + minFrames; f + minFrames < end; ++f) {
            if (f-last < minFrames || level[f] > level[f-1] || level[f] >= level[f+1]) continue;
            const auto left = *std::max_element(level.begin()+static_cast<std::ptrdiff_t>(std::max(last, f-minFrames)),
                                                level.begin()+static_cast<std::ptrdiff_t>(f));
            const auto right = *std::max_element(level.begin()+static_cast<std::ptrdiff_t>(f+1),
                                                 level.begin()+static_cast<std::ptrdiff_t>(f+minFrames+1));
            if (std::min(left, right) - level[f] >= settings.valleyDepthDb) {
                addSegment(last, f); last = f;
            }
        }
        addSegment(last, end);
    };
    std::size_t begin = 0, quiet = 0; bool inVoice = false;
    for (std::size_t f = 0; f < frames; ++f) {
        if (level[f] >= settings.floorDb) {
            if (!inVoice) { begin = f ? f-1 : 0; inVoice = true; }
            quiet = 0;
        } else if (inVoice && ++quiet >= quietFrames) {
            splitIsland(begin, std::min(frames, f-quiet+2)); inVoice = false; quiet = 0;
        }
    }
    if (inVoice) splitIsland(begin, frames);
    recalculate(a, settings);
    return a;
}

// Gain transitions take place INSIDE each segment, returning to unity at every
// boundary. Thus a neighbour's boost never crosses into a louder segment.
// Silence outside segments remains bit-identical; no destructive audio cuts.
inline double gainAt(const Analysis& a, std::size_t sample) noexcept {
    auto it = std::upper_bound(a.segments.begin(), a.segments.end(), sample,
                              [](std::size_t n, const Segment& s) { return n < s.end; });
    if (it == a.segments.end() || sample < it->start) return 1.0;
    const double fade = std::min(a.sampleRate * a.settings.transitionMs / 1000.0,
                                static_cast<double>(it->end-it->start) / 2.0);
    const double distance = static_cast<double>(std::min(sample-it->start, it->end-1-sample));
    const double t = std::clamp(distance / std::max(1.0, fade), 0.0, 1.0);
    const double smooth = t*t*(3-2*t);
    return std::pow(10.0, it->gainDb * smooth / 20.0);
}
inline void apply(const Analysis& a, float* const* channels, int channelCount,
                  std::size_t count, std::size_t startInSource = 0) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
        const float gain = static_cast<float>(gainAt(a, startInSource+i));
        for (int c = 0; c < channelCount; ++c) channels[c][i] *= gain;
    }
}
} // namespace syllable
