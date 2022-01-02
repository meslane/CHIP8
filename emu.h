#ifndef __emu_h__
#define __emu_h__

#include <time.h>
#include <chrono>
#include <fstream>

class emu {
	private:
		unsigned char registers[16];
		unsigned char sound;
		unsigned char delay;

		unsigned short I;
		unsigned short PC;
		unsigned char SP;
		unsigned short stack[64];

		unsigned char* display;
		unsigned char memory[4096];

		char halt;

		int xorByte(unsigned char x, unsigned char y, unsigned char byte, unsigned long cycle);

	public:
		emu(unsigned char display[32 * 64], unsigned short entry);
		int tick(long long time, unsigned short keys, std::ofstream& debug);
		void load(unsigned char* data, unsigned short len, unsigned short entry);
		unsigned char* getMemory();
};

#endif