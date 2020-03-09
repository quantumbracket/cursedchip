#pragma once


#include<string.h>


#include "hid.h"

#define TIMER_SPEED_HZ 60
#define USECS_PER_TIMER 16667


long CPU_SPEED_HZ = 500;
long USECS_PER_CPU = 2000; // 1e6/CPU_SPEED_HZ




unsigned char CHIP8_FONTSET[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};



typedef struct {
	uint8_t V[0x10];
	uint16_t I;
	uint16_t PC;
	uint8_t SP;
	uint8_t DT;
	uint8_t ST;
} registers_t;


class Runtime{
	public:
		registers_t registers = {
			.V = {0},
			.I = 0,
			.PC = 0x200,
			.SP = 0,
			.DT = 0,
			.ST = 0
		};
		uint8_t mem[0x1000]={0};
            	uint16_t stack[0x10]={0};
        	ScreenHandler* screen;
        	KeyboardHandler* keyboard;


		Runtime(uint8_t* rom, size_t romsize){
			memcpy(&mem,CHIP8_FONTSET,sizeof(CHIP8_FONTSET));
			memcpy(&mem[0x200],rom,romsize);
		}
};


typedef struct{
    uint16_t opcode;
    uint8_t _unused;
    int8_t error_num;
} ERRORCODE;

//error codes
#define CLEAN_EXIT 0 //exit with no error
#define EXEC_CONTINUE 1    //no error

#define ERR_OUT_OF_MEM -1
#define ERR_INVALID_OP -2
#define ERR_STACK_UNDERFLOW -3
#define ERR_STACK_OVERFLOW -4


//print error codes
const char* ERR_CODES_HR[]={"ERR_STACK_OVERFLOW","ERR_STACK_UNDERFLOW","ERR_INVALID_OP","ERR_OUT_OF_MEM","CLEAN_EXIT","EXEC_CONTINUE (suddenly stopped?)"};

#define err2string(errcode) ERR_CODES_HR[(int)(4+errcode)]

#define pack_error(errnum,erropcode) ERRORCODE {.opcode=erropcode,.error_num=errnum}
