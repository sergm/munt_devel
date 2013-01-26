#include "stdafx.h"

typedef _int8 Bit8s;
typedef _int16 Bit16s;
typedef _int32 Bit32s;

typedef unsigned _int8 Bit8u;
typedef unsigned _int16 Bit16u;
typedef unsigned _int32 Bit32u;

static const int MAX_SAMPLES = 64000;
static const float FLOAT_PI = 3.141592654f;
static const float FLOAT_LN_2 = logf(2.0f);
static const float FLOAT_LN_2_R = 1 / FLOAT_LN_2;

static const Bit32u MIDDLE_CUTOFF_VALUE = 128 << 18;
static const Bit32u RESONANCE_DECAY_THRESHOLD_CUTOFF_VALUE = 144 << 18;

static Bit16u exp9[512];
static Bit16u logsin9[512];

static inline float EXP2F(float x) {
	return expf(x * FLOAT_LN_2);
}

static inline float LOG2F(float x) {
	return logf(x) * FLOAT_LN_2_R;
}

static void init_tables() {
	// The LA32 chip contains an exponent table inside. The table contains 12-bit integer values.
	// The actual table size is 512 rows. The 9 higher bits of the fractional part of the argument are used as a lookup address.
	// To improve the precision of computations, the lower bits are supposed to be used for interpolation as the LA32 chip also
	// contains another 512-row table with inverted differences between the main table values.
	for (int i = 0; i < 512; i++) {
		exp9[i] = Bit16u(8191.5f - EXP2F(13.0f + ~i / 512.0f));
	}

	// There is a logarithmic sine table inside the LA32 chip. The table contains 13-bit integer values.
	for (int i = 1; i < 512; i++) {
		logsin9[i] = Bit16u(0.5f - LOG2F(sinf((i + 0.5f) / 1024.0f * FLOAT_PI)) * 1024.0f);
	}

	// The very first value is clamped to the maximum possible 13-bit integer
	logsin9[0] = 8191;
}

/**
 * LA32WaveGenerator is aimed to represent the exact model of LA32 wave generator.
 * The output square wave is created by adding high / low linear segments in-between
 * the rising and falling cosine segments. Basically, it’s very similar to the phase distortion synthesis.
 * Behaviour of a true resonance filter is emulated by adding decaying sine wave.
 * The beginning and the ending of the resonant sine is multiplied by a cosine window.
 * To synthesise sawtooth waves, the resulting square wave is multiplied by synchronous cosine wave.
 */
class LA32WaveGenerator {
	// True means the resulting square wave is to be multiplied by the synchronous cosine
	bool sawtoothWaveform;

	// Logarithmic amp of the wave generator
	Bit32u amp;

	// Logarithmic frequency of the resulting wave
	Bit16u pitch;

	// Values in range [1..31]
	// Value 1 correspong to the minimum resonance
	Bit8u resonance;

	// Processed value in range [0..255]
	// Values in range [0..128] have no effect and the resulting wave remains symmetrical
	// Value 255 corresponds to the maximum possible asymmetrical wave
	Bit8u pulseWidth;

	// Composed of the base cutoff in range [78..178] left-shifted by 18 bits and the TVF modifier
	Bit32u cutoffVal;

	// Relative position within a square wave phase:
	// 0             - start of the phase
	// 262144 (2^18) - corresponds to end of the sine phases. The length of linear phases may vary.
	Bit32u squareWavePosition;
	Bit32u highLen;
	Bit32u lowLen;

	// Relative position within a square wave phase:
	// 0 - start of the corresponding positive or negative segment of the square wave
	// The same increment sampleStep is used to indicate the current position
	// since the length of the resonance wave is always equal to four square wave sine segments.
	Bit32u resonanceSinePosition;

	// The amp of the resonance sine wave grows with the resonance value
	// As the resonance value cannot change while the partial is active, it is initialised once
	Bit32u resonanceAmpSubtraction;

	// Relative position within the sawtooth cosine wave
	// 0 - start of the positive rising segment of the square wave
	// The wave length corresponds to the current pitch
	Bit32u sawtoothCosinePosition;

	enum {
		POSITIVE_RISING_SINE_SEGMENT,
		POSITIVE_LINEAR_SEGMENT,
		POSITIVE_FALLING_SINE_SEGMENT,
		NEGATIVE_FALLING_SINE_SEGMENT,
		NEGATIVE_LINEAR_SEGMENT,
		NEGATIVE_RISING_SINE_SEGMENT
	} phase;

	enum {
		POSITIVE_RISING_RESONANCE_SINE_SEGMENT,
		POSITIVE_FALLING_RESONANCE_SINE_SEGMENT,
		NEGATIVE_FALLING_RESONANCE_SINE_SEGMENT,
		NEGATIVE_RISING_RESONANCE_SINE_SEGMENT
	} resonancePhase;

	// The increment of wavePosition which is added when the current sample is completely processed and processing of the next sample begins
	// Derived from the current values of pitch and cutoff
	Bit32u sampleStep;

	// The increment of sawtoothCosinePosition, the same as the sampleStep but for different wave length
	// Depends on the current pitch value
	Bit32u sawtoothCosineStep;

	struct LogSample {
		Bit16u logValue;
		enum {
			POSITIVE,
			NEGATIVE
		} sign;
	};

	void updateWaveGeneratorState();
	void advancePosition();

	LogSample nextSquareWaveLogSample();
	LogSample nextResonanceWaveLogSample();
	LogSample nextSawtoothCosineLogSample();

	Bit16u interpolateExp(Bit16u fract);
	Bit16s unlog(LogSample logSample);
	LogSample addLogSamples(LogSample sample1, LogSample sample2);

public:
	void init(bool sawtoothWaveform, Bit32u amp, Bit16u pitch, Bit32u cutoff, Bit8u pulseWidth, Bit8u resonance);
	Bit16s nextSample();
};

void LA32WaveGenerator::init(bool sawtoothWaveform, Bit32u amp, Bit16u pitch, Bit32u cutoffVal, Bit8u pulseWidth, Bit8u resonance) {
	this->sawtoothWaveform = sawtoothWaveform;
	this->amp = amp;
	this->pitch = pitch;
	this->cutoffVal = cutoffVal;
	this->pulseWidth = pulseWidth;
	this->resonance = resonance;

	phase = POSITIVE_RISING_SINE_SEGMENT;
	squareWavePosition = 0;
	sawtoothCosinePosition = 1 << 18;
	resonancePhase = POSITIVE_RISING_RESONANCE_SINE_SEGMENT;
	resonanceSinePosition = 0;
	resonanceAmpSubtraction = (32 - resonance) << 10;
}

void LA32WaveGenerator::updateWaveGeneratorState() {
	// sawtoothCosineStep = EXP2F(pitch / 4096. + cosineLenFactor / 4096. + 4)
	if (sawtoothWaveform) {
		Bit32u expArgInt = (pitch >> 12);
		sawtoothCosineStep = interpolateExp(~pitch & 4095);
		if (expArgInt < 8) {
			sawtoothCosineStep >>= 8 - expArgInt;
		} else {
			sawtoothCosineStep <<= expArgInt - 8;
		}
	}

	Bit32u cosineLenFactor = 0;
	if (cutoffVal > MIDDLE_CUTOFF_VALUE) {
		cosineLenFactor = (cutoffVal - MIDDLE_CUTOFF_VALUE) >> 10;
	}

	// sampleStep = EXP2F(pitch / 4096. + cosineLenFactor / 4096. + 4)
	{
		Bit32u expArg = pitch + cosineLenFactor;
		Bit32u expArgInt = (expArg >> 12);
		sampleStep = interpolateExp(~expArg & 4095);
		if (expArgInt < 8) {
			sampleStep >>= 8 - expArgInt;
		} else {
			sampleStep <<= expArgInt - 8;
		}
	}

	// Ratio of positive segment to wave length
	Bit32u pulseLenFactor = 0;
	if (pulseWidth > 128) {
		pulseLenFactor = (pulseWidth - 128) << 6;
	}

	// highLen = EXP2F(19 - pulseLenFactor / 4096. + cosineLenFactor / 4096.) - (2 << 18);
	if (pulseLenFactor < cosineLenFactor) {
		Bit32u expArg = cosineLenFactor - pulseLenFactor;
		Bit32u expArgInt = (expArg >> 12);
		highLen = interpolateExp(~expArg & 4095);
		highLen <<= 7 + expArgInt;
		highLen -= (2 << 18);
	} else {
		highLen = 0;
	}

	// lowLen = EXP2F(20 + cosineLenFactor / 4096.) - (4 << 18) - highLen;
	lowLen = interpolateExp(~cosineLenFactor & 4095);
	lowLen <<= 8 + (cosineLenFactor >> 12);
	lowLen -= (4 << 18) + highLen;
}

void LA32WaveGenerator::advancePosition() {
	squareWavePosition += sampleStep;
	resonanceSinePosition += sampleStep;
	if (sawtoothWaveform) {
		sawtoothCosinePosition = (sawtoothCosinePosition + sawtoothCosineStep) & ((1 << 20) - 1);
	}
	if (phase == POSITIVE_LINEAR_SEGMENT) {
		if (squareWavePosition >= highLen) {
			squareWavePosition -= highLen;
			phase = POSITIVE_FALLING_SINE_SEGMENT;
		}
	} else if (phase == NEGATIVE_LINEAR_SEGMENT) {
		if (squareWavePosition >= lowLen) {
			squareWavePosition -= lowLen;
			phase = NEGATIVE_RISING_SINE_SEGMENT;
		}
	} else if (squareWavePosition >= (1 << 18)) {
		squareWavePosition -= 1 << 18;
		if (phase == NEGATIVE_RISING_SINE_SEGMENT) {
			phase = POSITIVE_RISING_SINE_SEGMENT;
			resonancePhase = POSITIVE_RISING_RESONANCE_SINE_SEGMENT;
			resonanceSinePosition = squareWavePosition;
		} else {
			// phase incrementing hack
			++(*(int*)&phase);
			if (phase == NEGATIVE_FALLING_SINE_SEGMENT) {
				resonancePhase = NEGATIVE_FALLING_RESONANCE_SINE_SEGMENT;
				resonanceSinePosition = squareWavePosition;
			}
		}
	}
	// resonancePhase computation hack
	*(int*)&resonancePhase = ((resonanceSinePosition >> 18) + (phase > POSITIVE_FALLING_SINE_SEGMENT ? 2 : 0)) & 3;
}

LA32WaveGenerator::LogSample LA32WaveGenerator::nextSquareWaveLogSample() {
	Bit32u logSampleValue;
	switch (phase) {
		case POSITIVE_RISING_SINE_SEGMENT:
			logSampleValue = logsin9[(squareWavePosition >> 9) & 511];
			break;
		case POSITIVE_LINEAR_SEGMENT:
			logSampleValue = 0;
			break;
		case POSITIVE_FALLING_SINE_SEGMENT:
			logSampleValue = logsin9[~(squareWavePosition >> 9) & 511];
			break;
		case NEGATIVE_FALLING_SINE_SEGMENT:
			logSampleValue = logsin9[(squareWavePosition >> 9) & 511];
			break;
		case NEGATIVE_LINEAR_SEGMENT:
			logSampleValue = 0;
			break;
		case NEGATIVE_RISING_SINE_SEGMENT:
			logSampleValue = logsin9[~(squareWavePosition >> 9) & 511];
			break;
	}
	logSampleValue <<= 2;
	logSampleValue += amp >> 10;
	if (cutoffVal < MIDDLE_CUTOFF_VALUE) {
		logSampleValue += (MIDDLE_CUTOFF_VALUE - cutoffVal) >> 9;
	}

	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = phase < NEGATIVE_FALLING_SINE_SEGMENT ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

LA32WaveGenerator::LogSample LA32WaveGenerator::nextResonanceWaveLogSample() {
	Bit32u logSampleValue;
	if (resonancePhase == POSITIVE_FALLING_RESONANCE_SINE_SEGMENT || resonancePhase == NEGATIVE_RISING_RESONANCE_SINE_SEGMENT) {
		logSampleValue = logsin9[~(resonanceSinePosition >> 9) & 511];
	} else {
		logSampleValue = logsin9[(resonanceSinePosition >> 9) & 511];
	}
	logSampleValue <<= 2;
	logSampleValue += amp >> 10;

	// The decaying speed of the resonance sine is found a bit different for the positive and the negative segments
	static const Bit32u resAmpDecrementFactorTable[] = {31, 16, 12, 8, 5, 3, 2, 1};
	Bit32u resAmpDecrementFactor = resAmpDecrementFactorTable[resonance >> 2] << 2;
	resAmpDecrementFactor = phase < NEGATIVE_FALLING_SINE_SEGMENT ? resAmpDecrementFactor : resAmpDecrementFactor + 1;
	// Unsure about resonanceSinePosition here. It's possible that dedicated counter & decrement are used. Although, cutoff is finely ramped, so maybe not.
	logSampleValue += resonanceAmpSubtraction + ((resonanceSinePosition * resAmpDecrementFactor) >> 12);

	// To ensure the output wave has no breaks, two different windows are appied to the beginning and the ending of the resonance sine segment
	if (phase == POSITIVE_RISING_SINE_SEGMENT || phase == NEGATIVE_FALLING_SINE_SEGMENT) {
		// The window is synchronous sine here
		logSampleValue += logsin9[(squareWavePosition >> 9) & 511] << 2;
	} else if (phase == POSITIVE_FALLING_SINE_SEGMENT || phase == NEGATIVE_RISING_SINE_SEGMENT) {
		// The window is synchronous square sine here
		logSampleValue += logsin9[~(squareWavePosition >> 9) & 511] << 3;
	}

	if (cutoffVal < MIDDLE_CUTOFF_VALUE) {
		// For the cutoff values below the cutoff middle point, it seems the amp of the resonance wave is expotentially decayed
		logSampleValue += 31743 + ((MIDDLE_CUTOFF_VALUE - cutoffVal) >> 9);
	} else if (cutoffVal < RESONANCE_DECAY_THRESHOLD_CUTOFF_VALUE) {
		// For the cutoff values below this point, the amp of the resonance wave is sinusoidally decayed
		Bit32u sineIx = (cutoffVal - MIDDLE_CUTOFF_VALUE) >> 13;
		logSampleValue += logsin9[sineIx] << 2;
	}

	// After all the amp decrements are added, it should be safe now to adjust the amp of the resonance wave to what we see on captures
	logSampleValue -= 1 << 12;

	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = resonancePhase < NEGATIVE_FALLING_RESONANCE_SINE_SEGMENT ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

LA32WaveGenerator::LogSample LA32WaveGenerator::nextSawtoothCosineLogSample() {
	Bit32u logSampleValue;
	if ((sawtoothCosinePosition & (1 << 18)) > 0) {
		logSampleValue = logsin9[~(sawtoothCosinePosition >> 9) & 511];
	} else {
		logSampleValue = logsin9[(sawtoothCosinePosition >> 9) & 511];
	}
	logSampleValue <<= 2;

	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = ((sawtoothCosinePosition & (1 << 19)) == 0) ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

Bit16u LA32WaveGenerator::interpolateExp(Bit16u fract) {
	Bit16u expTabIndex = fract >> 3;
	Bit16u extraBits = fract & 7;
	Bit16u expTabEntry2 = 8191 - exp9[expTabIndex];
	Bit16u expTabEntry1 = expTabIndex == 0 ? 8191 : (8191 - exp9[expTabIndex - 1]);
	return expTabEntry1 + (((expTabEntry2 - expTabEntry1) * extraBits) >> 3);
}

Bit16s LA32WaveGenerator::unlog(LogSample logSample) {
	//Bit16s sample = (Bit16s)EXP2F(13.0f - logSample.logValue / 1024.0f);
	Bit32u intLogValue = logSample.logValue >> 12;
	Bit32u fracLogValue = logSample.logValue & 4095;
	Bit16s sample = interpolateExp(fracLogValue) >> intLogValue;
	return logSample.sign == LogSample::POSITIVE ? sample : -sample;
}

LA32WaveGenerator::LogSample LA32WaveGenerator::addLogSamples(LogSample sample1, LogSample sample2) {
	Bit32u logSampleValue = sample1.logValue + sample2.logValue;
	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = sample1.sign == sample2.sign ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

Bit16s LA32WaveGenerator::nextSample() {
	updateWaveGeneratorState();
	LogSample squareLogSample = nextSquareWaveLogSample();
	LogSample resonanceLogSample = nextResonanceWaveLogSample();
	LogSample cosineLogSample;
	if (sawtoothWaveform) {
		cosineLogSample = nextSawtoothCosineLogSample();
	}
	advancePosition();
	std::cout << unlog(squareLogSample) << "; " << unlog(resonanceLogSample) << "; " << unlog(cosineLogSample) << "; ";
	if (sawtoothWaveform) {
		return unlog(addLogSamples(squareLogSample, cosineLogSample)) + unlog(addLogSamples(resonanceLogSample, cosineLogSample));
	} else {
		return unlog(squareLogSample) + unlog(resonanceLogSample);
	}
}

int main() {
	init_tables();

	int cutoff = 70;
	cutoff = (78 + cutoff) << 18;
	int pw = 75;
	pw = pw * 255 / 100;
	int resonance = 20;
	resonance++;

	LA32WaveGenerator la32wg;
	la32wg.init(true, (264 + ((resonance >> 1) << 8)) << 10, 24835 - 4096, cutoff, pw, resonance);
	for (int i = 0; i < MAX_SAMPLES; i++) {
		std::cout << la32wg.nextSample() << std::endl;
	}
}
