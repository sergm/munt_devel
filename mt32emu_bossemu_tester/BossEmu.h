#ifndef BOSS_EMU_H
#define BOSS_EMU_H

class BossEmu {
public:
	BossEmu(const unsigned char rom[], int length);
	~BossEmu();
	void setParameters(int mode, int time, int level);
	bool isActive();
	void process(const short *inLeft, const short *inRight, short *outLeft, short *outRight, int length);

private:
	const unsigned char *rom;
	bool extendedModesEnabled;
	short *ram;

	int romBaseIx;
	int sawBits;

	int currentPosition;
	short accumulator;
	short shifter;
	short carry;

	void processingCycle(int cycleIx, short inputData);
	void updateAccumulator(unsigned char controlBits, int shifterMask);
};

#endif // BOSS_EMU_H
