#pragma once

//#define _CRT_SECURE_NO_DEPRECATE

#include <vector>
#include <utility>
#include <string>
#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <cstring>

class Audio {
public:
    // Constructors
    Audio(std::string str);
    Audio() {};

    // Read / Write files
    void load_wav(std::string str);
    void write_wav(std::string str);

    // Get functions
    // Subchunk2Size = NumSamples * NumChannels * BitsPerSample / 8 <==> NumSamples = Subchunk2Size / (NumChannels*(BitsPerSample / 8))
    unsigned get_size() { return data.size(); }
    unsigned get_sample_rate() { return SampleRate; }
    unsigned get_n_channels() { return NumChannels; }

    // Set Functions
    void set_sample_rate(int n) { SampleRate = n; }
    void set_n_channels(int n) { if (n != 1 && n != 2)throw std::invalid_argument("n can only be 1 (MONO) or 2 (STEREO)!"); NumChannels = n; }

    // Overloaded Operators
    std::pair<short, short> &operator[](unsigned i) { return data[i]; }
protected:
    char type[5];
    char format[5];
    char Subchunk1ID[5];
    char Subchunk2ID[5];

    int ChunkSize;
    int Subchunk1Size;
    int SampleRate;
    int ByteRate;
    int Subchunk2Size;

    short AudioFormat;
    short NumChannels;
    short BlockAlign;
    short BitsPerSample;

    // utility
    unsigned NumSamples;

    std::vector<std::pair<short, short> > data;
};
