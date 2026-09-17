#include "LevelingEngine.h"
#include <iostream>
#include <limits>
#include <random>
#include <string>

static int assertions = 0;
void check(bool ok, const char* message) {
    ++assertions; if (!ok) throw std::runtime_error(message);
}
void tone(std::vector<float>& out, double sr, double start, double duration, double amplitude) {
    for (auto n=static_cast<std::size_t>(start*sr); n<static_cast<std::size_t>((start+duration)*sr); ++n) {
        const double elapsed = static_cast<double>(n)/sr-start;
        const double fade = std::clamp(std::min(elapsed, duration-elapsed)/.01, 0.0, 1.0);
        out[n] = static_cast<float>(amplitude*fade*std::sin(2*3.141592653589793*200*n/sr));
    }
}
syllable::Analysis analyse(const std::vector<float>& x, double sr, syllable::Settings settings={}) {
    const float* channels[]{x.data()}; return syllable::analyse(channels,1,x.size(),sr,settings);
}
double rms(const std::vector<float>& x, std::size_t a, std::size_t b) {
    double sum=0; for(auto i=a;i<b;++i) sum+=x[i]*x[i]; return syllable::db(std::sqrt(sum/(b-a)));
}
int main() {
    try {
        for (const double sr : {44100.0,48000.0,96000.0}) {
            std::vector<float> voice(static_cast<std::size_t>(3*sr));
            tone(voice,sr,.1,.6,.08); tone(voice,sr,1.1,.6,.32); tone(voice,sr,2.1,.6,.16);
            auto a=analyse(voice,sr);
            check(a.segments.size()==3,"three separated vocal-like events expected");
            check(a.segments[0].gainDb>0 && a.segments[1].gainDb<0,"quiet boosted / loud attenuated");
            check(std::abs(a.segments[2].gainDb)<.05,"median event unchanged");
            auto out=voice; float* channel[]{out.data()}; syllable::apply(a,channel,1,out.size());
            double before=0,after=0;
            before=rms(voice,static_cast<std::size_t>(1.2*sr),static_cast<std::size_t>(1.6*sr))
                -rms(voice,static_cast<std::size_t>(.2*sr),static_cast<std::size_t>(.6*sr));
            after=rms(out,static_cast<std::size_t>(1.2*sr),static_cast<std::size_t>(1.6*sr))
                -rms(out,static_cast<std::size_t>(.2*sr),static_cast<std::size_t>(.6*sr));
            check(after<before*.55,"50% amount halves interior level spread");
            std::cout<<"sr="<<sr<<" segments="<<a.segments.size()<<" spread="<<before<<" -> "<<after<<" dB\n";
            for (auto n=static_cast<std::size_t>(.8*sr);n<static_cast<std::size_t>(sr);++n)
                check(voice[n]==out[n],"silence unchanged");
            auto blocks=voice;
            for(std::size_t n=0;n<blocks.size();n+=127) {
                float* p[]{blocks.data()+n}; syllable::apply(a,p,1,std::min<std::size_t>(127,blocks.size()-n),n);
            }
            check(blocks==out,"chunked playback exactly matches whole-source render");
            auto zero=a.settings; zero.amount=0; syllable::recalculate(a,zero);
            out=voice; channel[0]=out.data(); syllable::apply(a,channel,1,out.size());
            check(out==voice,"amount zero is bit-identical");
            zero.amount=1; zero.maxBoostDb=2; zero.maxCutDb=3; syllable::recalculate(a,zero);
            for(const auto& s:a.segments) check(s.gainDb<=2 && s.gainDb>=-3,"gain bounds respected");
            std::vector<float> inverse=voice; for(auto& x:inverse) x=-x;
            const float* stereo[]{voice.data(),inverse.data()};
            auto st=syllable::analyse(stereo,2,voice.size(),sr);
            check(st.segments.size()==3,"anti-phase stereo cannot disappear from detector");
            check(std::abs(st.segments[0].rmsDb-analyse(voice,sr).segments[0].rmsDb)<1e-8,"channel energy measurement");
        }
        std::vector<float> silent(48000), quiet(48000,1e-5f);
        check(analyse(silent,48000).segments.empty(),"silence has no segments");
        check(analyse(quiet,48000).segments.empty(),"noise under threshold has no segments");
        std::vector<float> vowel(96000); tone(vowel,48000,0,2,.2);
        check(analyse(vowel,48000).segments.size()==1,"sustained vowel is not split on periodicity");
        // Synthetic connected envelopes with a deep valley but no actual silence.
        for(std::size_t n=0;n<vowel.size();++n) {
            const double t=static_cast<double>(n)/48000;
            const double dip=1-.85*std::exp(-std::pow((t-1)/.05,2)); vowel[n]*=static_cast<float>(dip);
        }
        check(analyse(vowel,48000).segments.size()>=2,"deep connected valley is split");
        auto a=analyse(vowel,48000);
        for (const auto& s:a.segments) {
            check(syllable::gainAt(a,s.start)==1 && syllable::gainAt(a,s.end-1)==1,"unity at boundaries");
            check(std::abs(syllable::gainAt(a,s.start+1)-1)<.0001,"smooth first transition sample");
        }
        syllable::Analysis peaks; peaks.sampleRate=48000; peaks.sampleCount=96000;
        peaks.segments={{0,48000,-35,-2,0},{48000,96000,-15,-1,0}};
        auto full=syllable::Settings{}; full.amount=1;
        syllable::recalculate(peaks,full);
        check(peaks.segments[0].gainDb<=1.00001,"peak headroom limits boost");
        std::vector<float> invalid(100, std::numeric_limits<float>::quiet_NaN());
        bool rejected=false; try { analyse(invalid,48000); } catch(const std::invalid_argument&) {rejected=true;}
        check(rejected,"NaN audio rejected");
        rejected=false; full.amount=std::numeric_limits<double>::quiet_NaN();
        try { syllable::recalculate(a,full); } catch(const std::invalid_argument&) {rejected=true;}
        check(rejected,"NaN settings rejected");
        for(std::size_t length:{1u,7u,479u,480u,481u,901u}) {
            std::vector<float> tiny(length,.1f); const auto shortMap=analyse(tiny,48000);
            for(const auto& s:shortMap.segments) check(s.end<=length && s.start<s.end,"short buffer bounds");
        }
        std::cout<<"PASS: "<<assertions<<" assertions, synthetic audio only.\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
