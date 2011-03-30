Test programs for research of filtered square wave generation

Munt-IIR-Synth:
---------------

Filtered square wave generation using 4-order IIR filter (from Munt)

+ real physical / mathematical model
+ clear sequential computation with resonance at once
- many pre-computed coefficients in a large array (up to 15 Megs for now), there is no place to store them in the hardware
- it seems this model doesn�t correspond real hardware behavior, e.g. sound over-attenuated for values of sound frequency below the cutoff frequency value, and
- always produces resonance sine wave, without the resonance (e.g. for resonanceFactor 0.1) gives non-symmetrical waves


SubSynth:
---------

Filtered square wave generation using 4-stage bi-directional single-pole IIR filter with resonance emulated

+ real physical / mathematical model
+ there is no need of that huge array of coefficients
+ can produce pure almost symmetrical waves without resonance at all
- complications in bi-directional filtering algorithm, buffering and overlap-add must be used
- similar problem with sound over-attenuation
- very rough resonance emulation, need to be researched, but may be closer to the real hardware behavior


WTSynth:
--------

Filtered square wave generation using a wavetable for various cutoff points

+ much faster and simpler than subtractive algorithms above
+ modest memory requirements, wavetable can be very small but it depends on sound quality and number of indexes in the wavetable
+ wavetable can be filled with waveforms which exactly correspond to real hardware output either sampled or generated via slow and exact algorithms (e.g. window-sinc filter)
+ there is a way for producing sound for parameter values in-between neighbor indexes by mixing corresponding waves at proportional volumes (or any other interpolation method can be used)
- aliasing introduced when playing wave samples large enough at highest pitches, for producing superior sound quality it�s necessary either sound decimation or adding wavetables for several octaves


WTRSynth:
---------

Filtered squarewave generation using cosine waves on slopes
Waves produced are very similar to Casio's phase distortion but in another way

+ can surely be implemented in hardware using cosine LUT for speedup computations
+ LUT is the only memory needed for WG
+ waves produced are perfect and unique
+ this makes mindful use of the cosine LUT to form sawtooth waves
- can be claimed as Casio's patent violation when implemented in a electronic musical instrument
- to be checked for consistency in terms of accordance of waveform shape to filterval values
- for now, resonance emulation isn't functional since there are too many "How to?"s