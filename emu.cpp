#include "emu.h"

#include <iostream>
#include <fstream>
#include <time.h>
#include <stdlib.h> 
#include <iomanip>

#include <Windows.h>
#pragma comment(lib, "winmm.lib")

unsigned short combineNibbles(unsigned char nibbles[4]) {
	unsigned short result = 0;

	for (unsigned char i = 0; i < 3; i++) {
		result |= (unsigned short)nibbles[i] << (4 * i);
	}

	return result;
}

void emu::xorByte(unsigned char x, unsigned char y, unsigned char byte) {
	unsigned short index;

	for (unsigned char i = 0; i < 8; i++) {
		index = ((x + i) % 64) + (64 * (y % 32));
		if (((byte << i) & 0x80) == 0x80) {
			if (display[index] == 0) { //1,0
				display[index] = 255;
			}
			else { //1,1
				display[index] = 0;
				registers[0xF] = 1;
			}
		}
		else {
			if (display[index] == 0) { //0,0
				display[index] = 0;
			}
			else { //0,1
				display[index] = 255;
			}
		}
	}
}

emu::emu(unsigned char disp[32 * 64], unsigned short entry) {
	memset(this->registers, 0, 16);
	this->sound = 0;
	this->delay = 0;

	this->I = 0;
	this->PC = entry;
	this->entry = entry;
	this->SP = 0;

	this->halt = 0;

	this->display = disp;

	unsigned char digits[80] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, //0
		0x20, 0x60, 0x20, 0x20, 0x70, //1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
		0xF0 ,0x10, 0xF0, 0x10, 0xF0, //3
		0x90, 0x90, 0xF0, 0x10, 0x10, //4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
		0xF0, 0x10, 0x20, 0x40, 0x40, //7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
		0xF0, 0x90, 0xF0, 0x90, 0x90, //A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
		0xF0, 0x80, 0x80, 0x80, 0xF0, //C
		0xE0, 0x90, 0x90, 0x90, 0xE0, //D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
		0xF0, 0x80, 0xF0, 0x80, 0x80  //F
	};

	memcpy(memory + 0x100, digits, 80); //dump digits into 0x100
}

void emu::load(unsigned char* data, unsigned short len, unsigned short entry) {
	for (unsigned short i = 0; i < len; i++) {
		this->memory[entry + i] = data[i];
	}
}

unsigned char* emu::getMemory() {
	return this->memory;
}

void emu::reset() {
	memset(this->registers, 0, 16);
	memset(this->stack, 0, 64);
	memset(this->display, 0, 32 * 64);
	this->sound = 0;
	this->delay = 0;

	this->I = 0;
	this->PC = entry;
	this->SP = 0;

	this->halt = 0;
}

int emu::tick(long long time, unsigned short keys, std::ofstream& debug) {
	static unsigned char MSB, LSB;
	static unsigned char nibbles[4];
	
	static unsigned long cycle = 0;
	static long long last = 0;

	if (halt == 0) {
		MSB = memory[PC];
		PC++;
		LSB = memory[PC];
		PC++;

		//3210
		nibbles[3] = (MSB & 0xF0) >> 4;
		nibbles[2] = (MSB & 0x0F);
		nibbles[1] = (LSB & 0xF0) >> 4;
		nibbles[0] = (LSB & 0x0F);
	}

	/* counter decrementing */
	static unsigned char soundprior = 0;
	if (time - last >= 17) { //~17 ms period
		last = time;

		if (sound > 0 && (soundprior < sound)) {
			PlaySound(TEXT("audio/square150.wav"), NULL, SND_FILENAME | SND_ASYNC);
		}
		if (sound == 0 && (soundprior != 0)) {
			PlaySound(NULL, 0, 0);
		}

		soundprior = sound;
		sound = (sound > 0) ? sound - 1 : 0;
		delay = (delay > 0) ? delay - 1 : 0;
	}

	switch (nibbles[3]) { //highest nibble
		case 0x0: //MISC
			switch (LSB) {
				case 0xE0: //CLS (clear display)
					memset(this->display, 0, 32 * 64);
					break;
				case 0xEE: //RET (return from subroutine
					PC = stack[SP];
					if (SP > 0) {
						SP--;
					}
					else {
						printf("ERROR: stack underflow violation, attempting to pop empty stack at cycle:%lu\n", cycle);
						exit(1);
					}
					break;
				default:
					printf("ERROR: undefined opcode at cycle:%lu\n", cycle);
					break;
			}
			break;
		case 0x1: //JP addr (jump to address)
			PC = combineNibbles(nibbles);
			break;
		case 0x2: //CALL addr (call subroutine at address)
			if (SP < 63) {
				SP++;
			}
			else {
				printf("ERROR: stack overflow violation, attempting to push full stack at cycle:%lu\n", cycle);
				exit(1);
			}
			stack[SP] = PC;
			PC = combineNibbles(nibbles);
			break;
		case 0x3: //SE Vx, byte (skip next instruction if Vx == LSB)
			if (registers[nibbles[2]] == LSB) {
				PC += 2;
			}
			break;
		case 0x4: //SNE Vx, byte (skip next instruction if Vx != LSB)
			if (registers[nibbles[2]] != LSB) {
				PC += 2;
			}
			break;
		case 0x5: //SE Vx, Vy (skip next instruction if Vx == Vy)
			switch (nibbles[0]) {
				case 0x0:
					if (registers[nibbles[2]] == registers[nibbles[1]]) {
						PC += 2;
					}
					break;
				default:
					printf("ERROR: undefined opcode at cycle:%lu\n", cycle);
					break;
			}
			break;
		case 0x6: //LD Vx, byte (load register with LSB)
			registers[nibbles[2]] = LSB;
			break;
		case 0x7: //ADD Vx, byte (add LSB to Vx and store in Vx)
			registers[nibbles[2]] += LSB;
			break;
		case 0x8: //ALU and register operations
			switch (nibbles[0]) {
				case 0x0: //LD Vx, Vy (move Vy to Vx)
					registers[nibbles[2]] = registers[nibbles[1]];
					break;
				case 0x1: //OR Vx, Vy (or Vx and Vy and store in Vx)
					registers[nibbles[2]] |= registers[nibbles[1]];
					break;
				case 0x2: //AND Vx, Vy (and Vx and Vy and store in Vx)
					registers[nibbles[2]] &= registers[nibbles[1]];
					break;
				case 0x3: //XOR Vx, Vy (xor Vx and Vy and store in Vx)
					registers[nibbles[2]] ^= registers[nibbles[1]];
					break;
				case 0x4: //ADD Vx, Vy (add Vx and Vy and store in Vx, store carry in VF)
					registers[0xF] = (((unsigned short)registers[nibbles[2]] + (unsigned short)registers[nibbles[1]]) > 255);
					registers[nibbles[2]] += registers[nibbles[1]];
					break;
				case 0x5: //SUB Vx, Vy (sub Vx and Vy and store in Vx, store borrow in VF)
					registers[0xF] = (registers[nibbles[2]] > registers[nibbles[1]]);
					registers[nibbles[2]] -= registers[nibbles[1]];
					break;
				case 0x6: //SHR Vx (shift Vx right by 1)
					registers[0xF] = ((registers[nibbles[2]] & 0x01) == 0x01);
					registers[nibbles[2]] = registers[nibbles[2]] >> 1;
					break;
				case 0x7: //SUBN Vx, Vy (subtract Vy - Vx, store borrow in VF)
					registers[0xF] = (registers[nibbles[1]] > registers[nibbles[2]]);
					registers[nibbles[2]] = (registers[nibbles[1]] - registers[nibbles[2]]);
					break;
				case 0xE: //SHL Vx (shift Vx left by 1)
					registers[0xF] = ((registers[nibbles[2]] & 0x80) == 0x80);
					registers[nibbles[2]] = registers[nibbles[2]] << 1;
					break;
				default:
					printf("ERROR: undefined opcode at cycle:%lu\n", cycle);
					break;
			}
			break;
		case 0x9: //SNE Vx, Vy (skip next instruction if Vx != Vy)
			switch (nibbles[0]) {
				case 0x0:
					if (registers[nibbles[2]] != registers[nibbles[1]]) {
						PC += 2;
					}
					break;
				default:
					printf("ERROR: undefined opcode at cycle:%lu\n", cycle);
					break;
			}
			break;
		case 0xA: //LD I, addr (load index register)
			I = combineNibbles(nibbles);
			break;
		case 0xB: //JP V0, addr (jump to nnn + V0)
			PC = (unsigned short)registers[0] + combineNibbles(nibbles);
			break;
		case 0xC: //RND Vx, byte (generate random number and AND with immediate value, store in Vx)
			registers[nibbles[2]] = ((rand() % 256) & LSB);
			break;
		case 0xD: //DRW Vx, Vy, nibble (draw sprite pointed to by I with nibble bytes at location Vx, Vy)
			registers[0xF] = 0;
			for (unsigned char i = 0; i < nibbles[0]; i++) {
				xorByte(registers[nibbles[2]], 31 - (registers[nibbles[1]] + i), memory[I + i]);
			}
			break;
		case 0xE: //key skips
			switch (LSB) {
				case 0x9E: //SKP Vx (skip next instruction if key with value of Vx is pressed)
					if (((keys >> registers[nibbles[2]]) & 0x01) == 0x01) {
						PC += 2;
					}
					break;
				case 0xA1: //SKNP Vx (skip next instruction if key with valye of Vx is NOT pressed)
					if (((keys >> registers[nibbles[2]]) & 0x01) != 0x01) {
						PC += 2;
					}
					break;
				default:
					printf("ERROR: undefined opcode at cycle:%lu\n", cycle);
					break;
			}
			break;
		case 0xF: //time, index and key instructions
			switch (LSB) {
				case 0x07: //LD Vx, DT (load Vx with delay timer value)
					registers[nibbles[2]] = delay;
					break;
				case 0x0A: //LD Vx, K (halt until key is pressed and store key value in Vx)
					if (keys == 0) {
						halt = 1;
					}
					else {
						halt = 0;
						for (unsigned char i = 0; i < 16; i++) {
							if (((keys >> i) & 0x01) == 0x01) { //assign first key to Vx
								registers[nibbles[2]] = i;
								break;
							}
						}
					}
					break;
				case 0x15: //LD DT, Vx (load delay timer with Vx)
					delay = registers[nibbles[2]];
					break;
				case 0x18: //LD ST, Vx (set sound timer = Vx)
					sound = registers[nibbles[2]];
					break;
				case 0x1E: //ADD I, Vx (add Vx to I)
					I += registers[nibbles[2]];
					break;
				case 0x29: //LD F, Vx (set I to the hex digit sprite location stored in Vx)
					I = 0x100 + (5 * registers[nibbles[2]]);
					break;
				case 0x33: //LD B, Vx (store BCD of Vx in I, I+2, I+3) BROKEN
					memory[I] = registers[nibbles[2]] / 100;
					memory[I + 1] = (registers[nibbles[2]] / 10) % 10;
					memory[I + 2] = registers[nibbles[2]] % 10;
					break;
				case 0x55: //LD [I], Vx (store all registers in memory starting at I) BROKEN
					for (unsigned char i = 0; i <= nibbles[2]; i++) {
						memory[I + i] = registers[i];
					}
					break;
				case 0x65: //LD Vx, [I] (read memory starting at I until Vx into registers)
					for (unsigned char i = 0; i <= nibbles[2]; i++) {
						registers[i] = memory[I + i];
					}
					break;
				default:
					printf("ERROR: undefined opcode at cycle:%lu\n", cycle);
					break;
			}
			break;
	}

	if (halt == 0) {
		debug << std::dec << std::setw(10) << std::setfill('0') << cycle << ",";

		debug << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(MSB);
		debug << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(LSB) << ",";

		for (unsigned char i = 0; i < 16; i++) {
			debug << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(registers[i]) << ",";
		}

		debug << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(sound) << ",";
		debug << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(delay) << ",";
		debug << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(SP) << ",";
		debug << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(I) << ",";
		debug << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(PC) << std::endl;

		cycle++;
	}

	return 0;
}