#ifndef MT32EMU_H
#define MT32EMU_H

namespace MT32Emu {
	typedef short Bit16s;
	typedef unsigned int Bit32u;
	typedef Bit16s Sample;

	const Bit32u MAX_SAMPLES_PER_RUN = 4096;
	const Bit32u SAMPLE_RATE = 32000;

	class Synth {
	public:
		Bit32u getStereoOutputSampleRate();
		void render(Sample *inBuffer, Bit32u inLength);
	};
}

#endif