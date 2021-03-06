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

/**
 * LA32 performs wave generation in the log-space that allows replacing multiplications by cheap additions
 * It's assumed that only low-bit multiplications occur in a few places which are unavoidable like these:
 * - interpolation of exponent table (obvious, a delta value has 4 bits)
 * - computation of resonance amp decay envelope (the table contains values with 1-2 "1" bits except the very first value 31 but this case can be found using inversion)
 * - interpolation of PCM samples (obvious, the wave position counter is in the linear space, there is no log() table in the chip)
 * and it seems to be implemented in the same way as in the Boss chip, i.e. right shifted additions which involved noticeable precision loss
 * Subtraction is supposed to be replaced by simple inversion
 * As the logarithmic sine is always negative, all the logarithmic values are treated as decrements
 */
struct LogSample {
	// 16-bit fixed point value, includes 12-bit fractional part
	// 4-bit integer part allows to present any 16-bit sample in the log-space
	// Obviously, the log value doesn't contain the sign of the resulting sample
	Bit16u logValue;
	enum {
		POSITIVE,
		NEGATIVE
	} sign;
};

class LA32Utilites {
public:
	static void init_tables();
	static Bit16u interpolateExp(Bit16u fract);
	static Bit16s unlog(LogSample logSample);
	static LogSample addLogSamples(LogSample sample1, LogSample sample2);
};

/**
 * LA32WaveGenerator is aimed to represent the exact model of LA32 wave generator.
 * The output square wave is created by adding high / low linear segments in-between
 * the rising and falling cosine segments. Basically, it�s very similar to the phase distortion synthesis.
 * Behaviour of a true resonance filter is emulated by adding decaying sine wave.
 * The beginning and the ending of the resonant sine is multiplied by a cosine window.
 * To synthesise sawtooth waves, the resulting square wave is multiplied by synchronous cosine wave.
 */
class LA32WaveGenerator {
	//***************************************************************************
	//  The local copy of partial parameters below
	//***************************************************************************

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
	// Value 255 corresponds to the maximum possible asymmetric of the resulting wave
	Bit8u pulseWidth;

	// Composed of the base cutoff in range [78..178] left-shifted by 18 bits and the TVF modifier
	Bit32u cutoffVal;

	//***************************************************************************
	// Internal variables below
	//***************************************************************************

	// Relative position within a square wave phase:
	// 0             - start of the phase
	// 262144 (2^18) - corresponds to end of the sine phases, the length of linear phases may vary
	Bit32u squareWavePosition;
	Bit32u highLen;
	Bit32u lowLen;

	// Relative position within the positive or negative wave segment:
	// 0 - start of the corresponding positive or negative segment of the square wave
	// 262144 (2^18) - corresponds to end of the first sine phase in the square wave
	// The same increment sampleStep is used to indicate the current position
	// since the length of the resonance wave is always equal to four square wave sine segments.
	Bit32u resonanceSinePosition;

	// The amp of the resonance sine wave grows with the resonance value
	// As the resonance value cannot change while the partial is active, it is initialised once
	Bit32u resonanceAmpSubtraction;

	// The decay speed of resonance sine wave, depends on the resonance value
	Bit32u resAmpDecayFactor;

	// Relative position within the cosine wave which is used to form the sawtooth wave
	// 0 - start of the positive rising segment of the square wave
	// The wave length corresponds to the current pitch
	Bit32u sawtoothCosinePosition;

	// Current phase of the square wave
	enum {
		POSITIVE_RISING_SINE_SEGMENT,
		POSITIVE_LINEAR_SEGMENT,
		POSITIVE_FALLING_SINE_SEGMENT,
		NEGATIVE_FALLING_SINE_SEGMENT,
		NEGATIVE_LINEAR_SEGMENT,
		NEGATIVE_RISING_SINE_SEGMENT
	} phase;

	// Current phase of the resonance wave
	enum {
		POSITIVE_RISING_RESONANCE_SINE_SEGMENT,
		POSITIVE_FALLING_RESONANCE_SINE_SEGMENT,
		NEGATIVE_FALLING_RESONANCE_SINE_SEGMENT,
		NEGATIVE_RISING_RESONANCE_SINE_SEGMENT
	} resonancePhase;

	// The increment of a wave position which is added when the current sample is completely processed
	// Derived from the current values of pitch and cutoff
	Bit32u sampleStep;

	// The increment of sawtoothCosinePosition, the same as the sampleStep but for different wave length
	// Depends on the current pitch value
	Bit32u sawtoothCosineStep;

	// Resulting log-space samples of the square and resonance waves
	LogSample squareLogSample;
	LogSample resonanceLogSample;

	//***************************************************************************
	// Internal methods below
	//***************************************************************************

	void updateWaveGeneratorState();
	void advancePosition();

	LogSample nextSquareWaveLogSample();
	LogSample nextResonanceWaveLogSample();
	LogSample nextSawtoothCosineLogSample();

public:
	void init(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance);
	void generateNextSample(Bit32u amp, Bit16u pitch, Bit32u cutoff);
	LogSample getSquareLogSample();
	LogSample getResonanceLogSample();
};

// LA32PartialPair contains a structure of two partials being mixed / ring modulated
class LA32PartialPair {
	LA32WaveGenerator master;
	LA32WaveGenerator slave;
	bool ringModulated;
	bool mixed;

public:
	void init(bool ringModulated, bool mixed);
	void initMaster(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance);
	void initSlave(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance);
	void generateNextMasterSample(Bit32u amp, Bit16u pitch, Bit32u cutoff);
	void generateNextSlaveSample(Bit32u amp, Bit16u pitch, Bit32u cutoff);
	Bit16s nextOutSample();
};

void LA32Utilites::init_tables() {
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

Bit16u LA32Utilites::interpolateExp(Bit16u fract) {
	Bit16u expTabIndex = fract >> 3;
	Bit16u extraBits = fract & 7;
	Bit16u expTabEntry2 = 8191 - exp9[expTabIndex];
	Bit16u expTabEntry1 = expTabIndex == 0 ? 8191 : (8191 - exp9[expTabIndex - 1]);
	return expTabEntry1 + (((expTabEntry2 - expTabEntry1) * extraBits) >> 3);
}

Bit16s LA32Utilites::unlog(LogSample logSample) {
	//Bit16s sample = (Bit16s)EXP2F(13.0f - logSample.logValue / 1024.0f);
	Bit32u intLogValue = logSample.logValue >> 12;
	Bit32u fracLogValue = logSample.logValue & 4095;
	Bit16s sample = interpolateExp(fracLogValue) >> intLogValue;
	return logSample.sign == LogSample::POSITIVE ? sample : -sample;
}

LogSample LA32Utilites::addLogSamples(LogSample sample1, LogSample sample2) {
	Bit32u logSampleValue = sample1.logValue + sample2.logValue;
	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = sample1.sign == sample2.sign ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

void LA32WaveGenerator::updateWaveGeneratorState() {
	// sawtoothCosineStep = EXP2F(pitch / 4096. + cosineLenFactor / 4096. + 4)
	if (sawtoothWaveform) {
		Bit32u expArgInt = (pitch >> 12);
		sawtoothCosineStep = LA32Utilites::interpolateExp(~pitch & 4095);
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
		sampleStep = LA32Utilites::interpolateExp(~expArg & 4095);
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
		highLen = LA32Utilites::interpolateExp(~expArg & 4095);
		highLen <<= 7 + expArgInt;
		highLen -= (2 << 18);
	} else {
		highLen = 0;
	}

	// lowLen = EXP2F(20 + cosineLenFactor / 4096.) - (4 << 18) - highLen;
	lowLen = LA32Utilites::interpolateExp(~cosineLenFactor & 4095);
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

LogSample LA32WaveGenerator::nextSquareWaveLogSample() {
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

LogSample LA32WaveGenerator::nextResonanceWaveLogSample() {
	Bit32u logSampleValue;
	if (resonancePhase == POSITIVE_FALLING_RESONANCE_SINE_SEGMENT || resonancePhase == NEGATIVE_RISING_RESONANCE_SINE_SEGMENT) {
		logSampleValue = logsin9[~(resonanceSinePosition >> 9) & 511];
	} else {
		logSampleValue = logsin9[(resonanceSinePosition >> 9) & 511];
	}
	logSampleValue <<= 2;
	logSampleValue += amp >> 10;

	// From the digital captures, the decaying speed of the resonance sine is found a bit different for the positive and the negative segments
	Bit32u decayFactor = phase < NEGATIVE_FALLING_SINE_SEGMENT ? resAmpDecayFactor : resAmpDecayFactor + 1;
	// Unsure about resonanceSinePosition here. It's possible that dedicated counter & decrement are used. Although, cutoff is finely ramped, so maybe not.
	logSampleValue += resonanceAmpSubtraction + ((resonanceSinePosition * decayFactor) >> 12);

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

LogSample LA32WaveGenerator::nextSawtoothCosineLogSample() {
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

// Initialisation of the WG engine and set up the invariant parameters
void LA32WaveGenerator::init(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance) {
	this->sawtoothWaveform = sawtoothWaveform;
	this->pulseWidth = pulseWidth;
	this->resonance = resonance;

	phase = POSITIVE_RISING_SINE_SEGMENT;
	squareWavePosition = 0;
	sawtoothCosinePosition = 1 << 18;

	resonancePhase = POSITIVE_RISING_RESONANCE_SINE_SEGMENT;
	resonanceSinePosition = 0;
	resonanceAmpSubtraction = (32 - resonance) << 10;

	static const Bit32u resAmpDecayFactorTable[] = {31, 16, 12, 8, 5, 3, 2, 1};
	resAmpDecayFactor = resAmpDecayFactorTable[resonance >> 2] << 2;
}

// Update parameters with respect to TVP, TVA and TVF, and generate next sample
void LA32WaveGenerator::generateNextSample(Bit32u amp, Bit16u pitch, Bit32u cutoffVal) {
	this->amp = amp;
	this->pitch = pitch;
	this->cutoffVal = cutoffVal;

	updateWaveGeneratorState();
	squareLogSample = nextSquareWaveLogSample();
	resonanceLogSample = nextResonanceWaveLogSample();
	LogSample cosineLogSample;
	if (sawtoothWaveform) {
		cosineLogSample = nextSawtoothCosineLogSample();
	}
	advancePosition();
	//std::cout << LA32Utilites::unlog(squareLogSample) << "; " << LA32Utilites::unlog(resonanceLogSample) << "; ";
	if (sawtoothWaveform) {
		//std::cout << LA32Utilites::unlog(cosineLogSample) << "; ";
		this->squareLogSample = LA32Utilites::addLogSamples(squareLogSample, cosineLogSample);
		this->resonanceLogSample = LA32Utilites::addLogSamples(resonanceLogSample, cosineLogSample);
	}
}

LogSample LA32WaveGenerator::getSquareLogSample() {
	return squareLogSample;
}

LogSample LA32WaveGenerator::getResonanceLogSample() {
	return resonanceLogSample;
}

void LA32PartialPair::init(bool ringModulated, bool mixed) {
	this->ringModulated = ringModulated;
	this->mixed = mixed;
}

void LA32PartialPair::initMaster(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance) {
	master.init(sawtoothWaveform, pulseWidth, resonance);
}

void LA32PartialPair::initSlave(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance) {
	slave.init(sawtoothWaveform, pulseWidth, resonance);
}

void LA32PartialPair::generateNextMasterSample(Bit32u amp, Bit16u pitch, Bit32u cutoff) {
	master.generateNextSample(amp, pitch, cutoff);
}

void LA32PartialPair::generateNextSlaveSample(Bit32u amp, Bit16u pitch, Bit32u cutoff) {
	slave.generateNextSample(amp, pitch, cutoff);
}

Bit16s LA32PartialPair::nextOutSample() {
	LogSample masterSquareLogSample = master.getSquareLogSample();
	LogSample masterResonanceLogSample = master.getResonanceLogSample();
	LogSample slaveSquareLogSample = slave.getSquareLogSample();
	LogSample slaveResonanceLogSample = slave.getResonanceLogSample();
	if (ringModulated) {
		Bit16s sample = LA32Utilites::unlog(LA32Utilites::addLogSamples(masterSquareLogSample, slaveSquareLogSample));
		sample += LA32Utilites::unlog(LA32Utilites::addLogSamples(masterSquareLogSample, slaveResonanceLogSample));
		sample += LA32Utilites::unlog(LA32Utilites::addLogSamples(slaveSquareLogSample, masterResonanceLogSample));
		sample += LA32Utilites::unlog(LA32Utilites::addLogSamples(masterResonanceLogSample, slaveResonanceLogSample));
		if (mixed) {
			sample += LA32Utilites::unlog(masterSquareLogSample) + LA32Utilites::unlog(masterResonanceLogSample);
		}
		return sample;
	}
	Bit16s sample = LA32Utilites::unlog(masterSquareLogSample) + LA32Utilites::unlog(masterResonanceLogSample);
	sample += LA32Utilites::unlog(slaveSquareLogSample) + LA32Utilites::unlog(slaveResonanceLogSample);
	return sample;
}

int main() {
	LA32Utilites::init_tables();

	bool sawtooth = false;
	int pw = 0;
	pw = pw * 255 / 100;
	int resonance = 0;
	resonance++;

	LA32PartialPair la32pair;
	la32pair.init(true, true);
	la32pair.initMaster(sawtooth, pw, resonance);
	la32pair.initSlave(sawtooth, pw, resonance);

	Bit32u amp = (264 + ((resonance >> 1) << 8)) << 10;
	Bit16u pitch = 24835;
	if (sawtooth) pitch -= 4096;
	int cutoff = 50;
	cutoff = (78 + cutoff) << 18;

	for (int i = 0; i < MAX_SAMPLES; i++) {
		la32pair.generateNextMasterSample(amp, pitch, cutoff);
		la32pair.generateNextSlaveSample(amp, pitch, cutoff);
		std::cout << la32pair.nextOutSample() << std::endl;
	}
}
