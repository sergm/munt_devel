#ifndef MT32EMU_H
#define MT32EMU_H

namespace MT32Emu {
	typedef short Bit16s;
	typedef int Bit32s;
	typedef unsigned int Bit32u;

	enum AnalogOutputMode { AnalogOutputMode_DIGITAL_ONLY, AnalogOutputMode_COARSE, AnalogOutputMode_ACCURATE, AnalogOutputMode_OVERSAMPLED };

	const Bit32u MAX_SAMPLES_PER_RUN = 4096;
	const Bit32u SAMPLE_RATE = 32000;

	class Synth {
	public:
		template <class S>
		static inline void muteSampleBuffer(S *buffer, Bit32u len) {
			if (buffer == NULL) return;
			memset(buffer, 0, len * sizeof(S));
		}

		static inline void muteSampleBuffer(float *buffer, Bit32u len) {
			if (buffer == NULL) return;
			// FIXME: Use memset() where compatibility is guaranteed (if this turns out to be a win)
			while (len--) {
				*(buffer++) = 0.0f;
			}
		}

		static Bit32u getStereoOutputSampleRate(AnalogOutputMode);

		Bit32u getStereoOutputSampleRate();
		void render(Bit16s *inBuffer, Bit32u inLength);
		void render(float *inBuffer, Bit32u inLength);
	};
}

#endif
