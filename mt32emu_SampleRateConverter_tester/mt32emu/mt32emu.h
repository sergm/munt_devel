#ifndef MT32EMU_H
#define MT32EMU_H

#define MT32EMU_USE_FLOAT_SAMPLES 1

namespace MT32Emu {
	typedef short Bit16s;
	typedef int Bit32s;
	typedef unsigned int Bit32u;

#if MT32EMU_USE_FLOAT_SAMPLES
	typedef float Sample;
	typedef float SampleEx;
#else
	typedef Bit16s Sample;
	typedef Bit32s SampleEx;
#endif

	const Bit32u MAX_SAMPLES_PER_RUN = 4096;
	const Bit32u SAMPLE_RATE = 32000;

	class Synth {
	public:
		static inline void muteSampleBuffer(Sample *buffer, Bit32u len) {
			if (buffer == NULL) return;

#if MT32EMU_USE_FLOAT_SAMPLES
			// FIXME: Use memset() where compatibility is guaranteed (if this turns out to be a win)
			while (len--) {
				*(buffer++) = 0.0f;
			}
#else
			memset(buffer, 0, len * sizeof(Sample));
#endif
		}

		Bit32u getStereoOutputSampleRate();
		void render(Sample *inBuffer, Bit32u inLength);
	};
}

#endif
