/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2008 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "RubberBandPitchShifter.h"

#include "RubberBandStretcher.h"

#include <iostream>
#include <cmath>

using namespace RubberBand;

const char *const
RubberBandPitchShifter::portNamesMono[PortCountMono] =
{
    "latency",
    "Cents",
    "Semitones",
    "Octaves",
    "Crispness",
    "Formant Preservation",
    "Input",
    "Output"
};

const char *const
RubberBandPitchShifter::portNamesStereo[PortCountStereo] =
{
    "latency",
    "Cents",
    "Semitones",
    "Octaves",
    "Crispness",
    "Formant Preservation",
    "Input L",
    "Output L",
    "Input R",
    "Output R"
};

const LADSPA_PortDescriptor 
RubberBandPitchShifter::portsMono[PortCountMono] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

const LADSPA_PortDescriptor 
RubberBandPitchShifter::portsStereo[PortCountStereo] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

const LADSPA_PortRangeHint 
RubberBandPitchShifter::hintsMono[PortCountMono] =
{
    { 0, 0, 0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE,
      -100.0, 100.0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -12.0, 12.0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -4.0, 4.0 },
    { LADSPA_HINT_DEFAULT_MAXIMUM |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
       0.0, 3.0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_TOGGLED,
       0.0, 1.0 },
    { 0, 0, 0 },
    { 0, 0, 0 }
};

const LADSPA_PortRangeHint 
RubberBandPitchShifter::hintsStereo[PortCountStereo] =
{
    { 0, 0, 0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE,
      -100.0, 100.0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -12.0, 12.0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -4.0, 4.0 },
    { LADSPA_HINT_DEFAULT_MAXIMUM |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
       0.0, 3.0 },
    { LADSPA_HINT_DEFAULT_0 |
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_TOGGLED,
       0.0, 1.0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 }
};

const LADSPA_Properties
RubberBandPitchShifter::properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;

const LADSPA_Descriptor 
RubberBandPitchShifter::ladspaDescriptorMono =
{
    2979, // "Unique" ID
    "rubberband-pitchshifter-mono", // Label
    properties,
    "Rubber Band Mono Pitch Shifter", // Name
    "Breakfast Quay",
    "GPL",
    PortCountMono,
    portsMono,
    portNamesMono,
    hintsMono,
    0, // Implementation data
    instantiate,
    connectPort,
    activate,
    run,
    0, // Run adding
    0, // Set run adding gain
    deactivate,
    cleanup
};

const LADSPA_Descriptor 
RubberBandPitchShifter::ladspaDescriptorStereo =
{
    9792, // "Unique" ID
    "rubberband-pitchshifter-stereo", // Label
    properties,
    "Rubber Band Stereo Pitch Shifter", // Name
    "Breakfast Quay",
    "GPL",
    PortCountStereo,
    portsStereo,
    portNamesStereo,
    hintsStereo,
    0, // Implementation data
    instantiate,
    connectPort,
    activate,
    run,
    0, // Run adding
    0, // Set run adding gain
    deactivate,
    cleanup
};

const LADSPA_Descriptor *
RubberBandPitchShifter::getDescriptor(unsigned long index)
{
    if (index == 0) return &ladspaDescriptorMono;
    if (index == 1) return &ladspaDescriptorStereo;
    else return 0;
}

RubberBandPitchShifter::RubberBandPitchShifter(int sampleRate, size_t channels) :
    m_latency(0),
    m_cents(0),
    m_semitones(0),
    m_octaves(0),
    m_crispness(0),
    m_formant(0),
    m_ratio(1.0),
    m_prevRatio(1.0),
    m_currentCrispness(-1),
    m_currentFormant(false),
//    m_extraLatency(8192), //!!! this should be at least the maximum possible displacement from linear at input rates, divided by the pitch scale factor.  It could be very large
//    m_extraLatency(512), //!!! this should be at least the maximum possible displacement from linear at input rates, divided by the pitch scale factor.  It could be very large
    m_extraLatency(128), //!!! this should be at least the maximum possible displacement from linear at input rates, divided by the pitch scale factor.  It could be very large
    m_stretcher(new RubberBandStretcher
                (sampleRate, channels,
                 RubberBandStretcher::OptionProcessRealTime)),
    m_sampleRate(sampleRate),
    m_channels(channels)
{
    for (size_t c = 0; c < m_channels; ++c) {
        m_input[c] = 0;
        m_output[c] = 0;
        //!!! size must be at least max process size plus m_extraLatency:
        m_outputBuffer[c] = new RingBuffer<float>(8092); //!!!
        m_outputBuffer[c]->zero(m_extraLatency);
        //!!! size must be at least max process size:
        m_scratch[c] = new float[16384];//!!!
        for (int i = 0; i < 16384; ++i) {
            m_scratch[c][i] = 0.f;
        }
    }
    int reqd = m_stretcher->getSamplesRequired();
    m_stretcher->process(m_scratch, reqd, false);
    int avail = m_stretcher->available();
    std::cerr << "construction: reqd = " << reqd <<  ", available = " << avail << std::endl;
    if (avail > 0) {
        m_stretcher->retrieve(m_scratch, avail);
    }
}

RubberBandPitchShifter::~RubberBandPitchShifter()
{
    delete m_stretcher;
    for (size_t c = 0; c < m_channels; ++c) {
        delete m_outputBuffer[c];
        delete[] m_scratch[c];
    }
}
    
LADSPA_Handle
RubberBandPitchShifter::instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
    if (desc->PortCount == ladspaDescriptorMono.PortCount) {
        return new RubberBandPitchShifter(rate, 1);
    } else if (desc->PortCount == ladspaDescriptorStereo.PortCount) {
        return new RubberBandPitchShifter(rate, 2);
    }
    return 0;
}

void
RubberBandPitchShifter::connectPort(LADSPA_Handle handle,
				    unsigned long port, LADSPA_Data *location)
{
    RubberBandPitchShifter *shifter = (RubberBandPitchShifter *)handle;

    float **ports[PortCountStereo] = {
        &shifter->m_latency,
	&shifter->m_cents,
	&shifter->m_semitones,
	&shifter->m_octaves,
        &shifter->m_crispness,
	&shifter->m_formant,
    	&shifter->m_input[0],
	&shifter->m_output[0],
	&shifter->m_input[1],
	&shifter->m_output[1]
    };

    *ports[port] = (float *)location;
}

void
RubberBandPitchShifter::activate(LADSPA_Handle handle)
{
    RubberBandPitchShifter *shifter = (RubberBandPitchShifter *)handle;
    shifter->updateRatio();
    shifter->m_prevRatio = shifter->m_ratio;
    shifter->m_stretcher->reset();
    shifter->m_stretcher->setPitchScale(shifter->m_ratio);
}

void
RubberBandPitchShifter::run(LADSPA_Handle handle, unsigned long samples)
{
    RubberBandPitchShifter *shifter = (RubberBandPitchShifter *)handle;
    shifter->runImpl(samples);
}

void
RubberBandPitchShifter::updateRatio()
{
    double oct = *m_octaves;
    oct += *m_semitones / 12;
    oct += *m_cents / 1200;
    m_ratio = pow(2.0, oct);
}

void
RubberBandPitchShifter::updateCrispness()
{
    if (!m_crispness) return;
    
    int c = lrintf(*m_crispness);
    if (c == m_currentCrispness) return;
    if (c < 0 || c > 3) return;
    RubberBandStretcher *s = m_stretcher;

    switch (c) {
    case 0:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseIndependent);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsSmooth);
        break;
    case 1:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseAdaptive);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsSmooth);
        break;
    case 2:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseAdaptive);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsMixed);
        break;
    case 3:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseAdaptive);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsCrisp);
        break;
    }

    m_currentCrispness = c;
}

void
RubberBandPitchShifter::updateFormant()
{
    if (!m_formant) return;

    bool f = (*m_formant > 0.5f);
    if (f == m_currentFormant) return;
    
    RubberBandStretcher *s = m_stretcher;
    
    s->setFormantOption(f ?
                        RubberBandStretcher::OptionFormantPreserved :
                        RubberBandStretcher::OptionFormantShifted);

    m_currentFormant = f;
}

void
RubberBandPitchShifter::runImpl(unsigned long insamples)
{
//    std::cerr << "RubberBandPitchShifter::runImpl(" << insamples << ")" << std::endl;

    static int incount = 0, outcount = 0;

    updateRatio();
    if (m_ratio != m_prevRatio) {
        m_stretcher->setPitchScale(m_ratio);
        m_prevRatio = m_ratio;
    }

    if (m_latency) {
        *m_latency = float(m_stretcher->getLatency() + m_extraLatency);
//        std::cerr << "latency = " << *m_latency << std::endl;
    }

    updateCrispness();
    updateFormant();

    int samples = insamples;
    int processed = 0;
    size_t outTotal = 0;

    float *ptrs[2];

    // We have to break up the input into chunks like this because
    // insamples could be arbitrarily large 

    while (processed < samples) {

        //!!! size_t:
        int toCauseProcessing = m_stretcher->getSamplesRequired();
//        std::cout << "to-cause: " << toCauseProcessing << ", remain = " << samples - processed;
        int inchunk = std::min(samples - processed, toCauseProcessing);
        for (size_t c = 0; c < m_channels; ++c) {
            ptrs[c] = &(m_input[c][processed]);
        }
        m_stretcher->process(ptrs, inchunk, false);
        processed += inchunk;
        incount += inchunk; //!!!

        int avail = m_stretcher->available();
        int writable = m_outputBuffer[0]->getWriteSpace();
        int outchunk = std::min(avail, writable);
        size_t actual = m_stretcher->retrieve(m_scratch, outchunk);
        outTotal += actual;
        outcount += actual;

//        std::cout << ", avail: " << avail << ", outchunk = " << outchunk;
//        if (actual != outchunk) std::cout << " (" << actual << ")";
//        std::cout << std::endl;

        outchunk = actual;

        for (size_t c = 0; c < m_channels; ++c) {
            if (int(m_outputBuffer[c]->getWriteSpace()) < outchunk) {
                std::cerr << "RubberBandPitchShifter::runImpl: buffer overrun: chunk = " << outchunk << ", space = " << m_outputBuffer[c]->getWriteSpace() << std::endl;
            }                
            m_outputBuffer[c]->write(m_scratch[c], outchunk);
        }
    }

    std::cout << "in: " << incount << ", out: " << outcount << ", loss: " << incount - outcount << std::endl;
    
//    std::cout << "processed = " << processed << " in, " << outTotal << " out" << ", fill = " << m_outputBuffer[0]->getReadSpace() << " of " << m_outputBuffer[0]->getSize() << std::endl;
    
    for (size_t c = 0; c < m_channels; ++c) {
        int avail = m_outputBuffer[c]->getReadSpace();
//        std::cout << "avail: " << avail << std::endl;
        if (avail < samples && c == 0) {
            std::cerr << "RubberBandPitchShifter::runImpl: buffer underrun: required = " << samples << ", available = " << avail << std::endl;
        }
        int chunk = std::min(avail, samples);
//        std::cout << "out chunk: " << chunk << std::endl;
        m_outputBuffer[c]->read(m_output[c], chunk);
    }

    static int minr = -1;
    int avail = m_outputBuffer[0]->getReadSpace();
    if (minr == -1 || (avail >= 0 && avail < minr)) {
        std::cerr << "RubberBandPitchShifter::runImpl: new min remaining " << avail << " from " << minr << std::endl;
        minr = avail;
    }
}

void
RubberBandPitchShifter::deactivate(LADSPA_Handle handle)
{
    activate(handle); // both functions just reset the plugin
}

void
RubberBandPitchShifter::cleanup(LADSPA_Handle handle)
{
    delete (RubberBandPitchShifter *)handle;
}

