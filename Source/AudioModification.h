#pragma once
#include "LevelingEngine.h"

// Message-thread analysis and persistence. Playback only takes a nonblocking
// read lock and never allocates. v0.1 intentionally limits full-source length.
class SyllableAudioModification final : public ARAAudioModification,
                                       private ARAAudioSource::Listener
{
public:
    SyllableAudioModification(ARAAudioSource* source, ARA::ARAAudioModificationHostRef ref,
                             const ARAAudioModification* clone)
        : ARAAudioModification(source, ref, clone)
    {
        source->addListener(this);
        if (clone != nullptr) {
            const auto* other = static_cast<const SyllableAudioModification*>(clone);
            analysis = other->snapshot(); enabled.store(other->isEnabled());
        }
    }
    ~SyllableAudioModification() override { getAudioSource()->removeListener(this); }
    bool isEnabled() const { return enabled.load(std::memory_order_relaxed); }
    void setEnabled(bool b) { enabled.store(b, std::memory_order_relaxed); }
    syllable::Analysis snapshot() const { const ScopedReadLock l(mapLock); return analysis; }
    bool hasAnalysis() const { const ScopedReadLock l(mapLock); return analysis.sampleRate > 0; }
    String status = "Duplo clique: analisar | botao direito: intensidade";
    std::atomic<bool> formatMismatch { false };

    bool analyseSource() {
        auto* source = getAudioSource();
        if (!source->isSampleAccessEnabled()) { status = "Audio ainda indisponivel pelo ARA"; return false; }
        const auto count = source->getSampleCount();
        const auto rate = source->getSampleRate();
        if (rate < 8000 || rate > 384000 || !std::isfinite(rate)
            || count <= 0 || count > rate * 120.0 || count > 24000000
            || source->getChannelCount() < 1 || source->getChannelCount() > 2) {
            status = "v0.1: arquivo mono/estereo de ate 120 s e 24 milhoes de amostras"; return false;
        }
        try {
            ARAAudioSourceReader reader(source);
            AudioBuffer<float> audio(source->getChannelCount(), static_cast<int>(count));
            for (int offset = 0; offset < audio.getNumSamples(); offset += 8192)
                if (!reader.read(&audio, offset, std::min(8192, audio.getNumSamples()-offset), offset, true, true)) {
                    status = "Falha na leitura ARA; analise anterior preservada"; return false;
                }
            auto next = syllable::analyse(audio.getArrayOfReadPointers(), audio.getNumChannels(),
                                          static_cast<std::size_t>(count), rate, snapshot().settings);
            const auto size = next.segments.size();
            { const ScopedWriteLock l(mapLock); analysis = std::move(next); }
            enabled.store(true);
            status = String(static_cast<int>(size)) + " segmentos acusticos (estimativa)";
            return true;
        } catch (const std::exception& e) { status = String("Analise: ") + e.what(); return false; }
    }
    void setAmount(double amount) {
        auto next = snapshot(); auto settings = next.settings; settings.amount = amount;
        syllable::recalculate(next, settings);
        { const ScopedWriteLock l(mapLock); analysis = std::move(next); }
    }
    void apply(AudioBuffer<float>& buffer, int offset, int count, int64 startInSource) const noexcept {
        if (!isEnabled() || startInSource < 0) return;
        const ScopedTryReadLock l(mapLock);
        if (!l.isLocked()) return; // Preserve original audio on contention.
        for (int n = 0; n < count; ++n) {
            const auto gain = static_cast<float>(syllable::gainAt(analysis, static_cast<std::size_t>(startInSource+n)));
            for (int c = 0; c < buffer.getNumChannels(); ++c)
                buffer.getWritePointer(c)[offset+n] *= gain;
        }
    }
    void notifyEdit(bool notifyHost = true) {
        notifyContentChanged(ARAContentUpdateScopes::samplesAreAffected(), notifyHost);
        for (auto* region : getPlaybackRegions())
            region->notifyContentChanged(ARAContentUpdateScopes::samplesAreAffected(), notifyHost);
    }
    bool writeArchive(ARAOutputStream& out) const {
        const auto a = snapshot();
        if (!out.writeBool(isEnabled()) || !out.writeDouble(a.sampleRate)
            || !out.writeInt64(static_cast<int64>(a.sampleCount)) || !out.writeDouble(a.settings.amount)
            || !out.writeInt(static_cast<int>(a.segments.size()))) return false;
        for (const auto& s : a.segments)
            if (!out.writeInt64(static_cast<int64>(s.start)) || !out.writeInt64(static_cast<int64>(s.end))
                || !out.writeDouble(s.rmsDb) || !out.writeDouble(s.peakDb)) return false;
        return true;
    }
    static bool readArchive(ARAInputStream& in, syllable::Analysis& a, bool& enable) {
        enable = in.readBool(); a.sampleRate = in.readDouble();
        const auto count = in.readInt64(); a.settings.amount = in.readDouble();
        const auto size = in.readInt();
        if (count < 0 || count > 24000000 || size < 0 || size > 20000
            || !std::isfinite(a.sampleRate) || a.sampleRate < 0 || a.sampleRate > 384000
            || (size > 0 && a.sampleRate < 8000)
            || !std::isfinite(a.settings.amount) || a.settings.amount < 0 || a.settings.amount > 1) return false;
        a.sampleCount = static_cast<std::size_t>(count);
        for (int i = 0; i < size; ++i) {
            const auto start = in.readInt64(), end = in.readInt64();
            const auto rms = in.readDouble(), peak = in.readDouble();
            if (start < 0 || end <= start || end > count || (!a.segments.empty() && start < static_cast<int64>(a.segments.back().end))
                || !std::isfinite(rms) || !std::isfinite(peak) || rms < -120 || rms > 60 || peak < -120 || peak > 60) return false;
            a.segments.push_back({static_cast<std::size_t>(start), static_cast<std::size_t>(end), rms, peak, 0});
        }
        syllable::recalculate(a, a.settings); return !in.failed();
    }
    void restore(syllable::Analysis a, bool enable) {
        // Reject maps belonging to a source with a different layout.
        if (a.sampleRate != 0 && (a.sampleRate != getAudioSource()->getSampleRate()
            || a.sampleCount != static_cast<std::size_t>(getAudioSource()->getSampleCount()))) {
            invalidate(); return;
        }
        { const ScopedWriteLock l(mapLock); analysis = std::move(a); }
        setEnabled(enable); status = "Mapa restaurado"; notifyEdit(false);
    }
private:
    void invalidate() {
        setEnabled(false);
        { const ScopedWriteLock l(mapLock); analysis = {}; }
        status = "Audio mudou: analisar novamente";
    }
    void doUpdateAudioSourceContent(ARAAudioSource*, ARAContentUpdateScopes) override { invalidate(); }
    void didUpdateAudioSourceProperties(ARAAudioSource*) override {
        const auto a = snapshot();
        if (a.sampleRate != getAudioSource()->getSampleRate()
            || a.sampleCount != static_cast<std::size_t>(getAudioSource()->getSampleCount())) invalidate();
    }
    mutable ReadWriteLock mapLock;
    syllable::Analysis analysis;
    std::atomic<bool> enabled {false};
};
