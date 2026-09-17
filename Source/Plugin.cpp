#include <JuceHeader.h>
using namespace juce;
#include "ARAPlugin.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SyllableAudioProcessor();
}
const ARA::ARAFactory* JUCE_CALLTYPE createARAFactory()
{
    return juce::ARADocumentControllerSpecialisation::createARAFactory<SyllableDocumentControllerSpecialisation>();
}
