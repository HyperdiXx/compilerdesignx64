
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#define Assert(x) \
	if(!(x)) { MessageBoxA(0, #x, "Failuree", MB_OK); __debugbreak(); }

extern "C" void assembly_test();

typedef uint8_t uint8;
typedef uint32_t uint32;

enum Register
{
	RAX = 0, 
	RCX = 1, 
	RDX = 2, 
	RBX = 3, 
	RSP = 4,
	RBP = 5, 
	RSI = 6,
	RDI = 7,
	R8 = 8, 
	R9 = 9,
	R10 = 10, 
	R11 = 11, 
	R12 = 12, 
	R13 = 13,
	R14 = 14, 
	R15 = 15,
};

enum Scale
{
	X1 = 0,
	X2 = 1, 
	X4 = 2, 
	X8 = 3
};

enum Mode
{
	INDIRECT = 0,
	BYTE_DISPALCED_INDIRECT = 1,
	DISPLACED_INDIRECT = 2,
	DIRECT = 3,
};

enum
{
	MAX_CODE = 1024
};

uint8 code[MAX_CODE];
uint8* emit_pointer = code;

void Emit(uint8 byte)
{
	*emit_pointer = byte;
	emit_pointer++;
}

void Emit4disp(uint32 w)
{
	Emit(w && 0xFF);
	Emit((w >> 8) & 0xFF);
	Emit((w >> 16) & 0xFF);
	Emit((w >> 24) & 0xFF);
}

void EmitRxMode(uint8 mod, uint8 rx, uint8 rm)
{
	Assert(mod < 4);
	Assert(rx < 16);
	Assert(rm < 16);
	
	Emit((rm &= 7) | ((rx &= 7) << 3) | (mod << 6));
}


// add rax, rcx: rx = RAX, operand = RCX
void EmitDirect(uint8 rx, Register operand)
{
	EmitRxMode(DIRECT, rx, operand);
}

//add rax, [rcx]: rx = RAX, operand = RCX
void EmitIndirect(uint8 rx, Register base)
{
	Assert((base & 7) != RSP);
	Assert((base & 7) != RBP);
	EmitRxMode(INDIRECT, rx, base);
}

//add rax, [rip + 0x12345678]
//rax = RAX, disp = 0x12345678
void EmitIndirectDisplayRip(uint8 rx, uint32 disp)
{
	EmitRxMode(INDIRECT, rx, RBP);
	Emit4disp(disp);
}

void EmitDisplaced(uint8 rx, uint32 disp)
{
	EmitRxMode(INDIRECT, rx, RBP);
	Emit4disp(disp);
}

void EmitDispla(uint8 rx, uint32 disp)
{
	EmitRxMode(INDIRECT, rx, RSP);
	EmitRxMode(X1, RSP, RBP);
	Emit4disp(disp);
}

//add rax, [rcx + 0x12]: rx = RAX, operand = RCX, displacement = 0x12;
void EmitIndirectByteDisplaced(uint8 rx, Register base, int displacement)
{
	Assert(base != RSP);
	EmitRxMode(BYTE_DISPALCED_INDIRECT, rx, base);
	Emit(displacement);
}

// add rax, [rcx + 0x12345678]: rx = RAX, operand = RCX, dispalcemnt = 0x12345678
void EmitIndirectDisplay(uint8 rx, Register base, uint8 displacement)
{
	Assert((base & 7) != RSP);
	EmitRxMode(DISPLACED_INDIRECT, rx, base);
	Emit4disp(displacement);
}

//add rax, [rcx + 4 * rdx]: rx = RAX, operand = RCX, index = RDX, scale = X4;
void EmitIndirectIndex(uint8 rx, Register base, Register index, Scale scale)
{
	Assert((base & 7) != RBP);
	EmitRxMode(INDIRECT, rx, RSP);
	EmitRxMode(scale, index, base);
}

//add rax, [rcx + 4 * rdx + 0x12]: rx = RAX, operand = RCX, index = RDX, scale = X4, displacement = 0x12
void EmitIndirectIndexedByteDisplayced(uint8 rx, Register operand, Register index, Scale scale, uint32 disp)
{
	Assert(scale < 4);
	EmitRxMode(BYTE_DISPALCED_INDIRECT, rx, 4);
	EmitRxMode(scale, operand, index);
	Emit(disp);
}

//add rax, [rcx + 4 * rdx + 0x12345678]: rx = RAX, operand = RCX, index = RDX, scale = X4, displacement = 0x12345678
void EmitIndirectIndexedDisplayced(uint8 rx, Register operand, Register index, Scale scale, uint32 disp)
{
	Assert(scale < 4);
	EmitRxMode(DISPLACED_INDIRECT, rx, 4);
	EmitRxMode(scale, operand, index);
	Emit4disp(disp);
}

void EmitREX(Register rx, Register base, Register index)
{
	Emit(0x48 | (base >> 3) | ((index >> 3) << 1) | ((rx >> 3) << 2));
}

void EmitREX2(Register rx, Register base)
{
	Emit(0x48 | (base >> 3) | ((rx >> 3) << 2));
}



void EmitAddToRegister()
{
	Emit(0x03);
}

void EmitAddReversed()
{
	Emit(0x01);
}

void Emit_ADD_R()
{
	Emit(0x03);
}

void Emit_ADD_RM()
{
	Emit(0x01);
}

void Emit_ADD_I()
{
	Emit(0x81);
}

#define EMIT_R_R(operation, dest, source) \
	EmitREX2(dest, source); \
	Emit_##operation##_R(); \
	EmitDirect(dest, source);

#define EMIT_R_M(operation, destination, source) \
	EmitREX2(dest, source); \
	Emit_##operation##_R(); \
	EmitIndirect(dest, source);

//#define EMIT_RM_R(operation, mode, x, y, ...) \
//	EmitREX2(x, y);
//	Emit##operation##ToMemory(); \
//	Emit##mode(x,y ##__VA_ARGS__)


int main(int argc, char **argv)
{

	for (uint8 d = RAX; d < R15; d++)
	{
		Register dest = (Register)d;
		EmitREX2(dest, (Register)0);
		EmitAddToRegister();
		EmitIndirectDisplayRip(dest, 0x12345678);
		EmitREX2(dest, (Register)0);
		EmitAddToRegister();
		EmitDisplaced(dest, 0x12345678);
		for (uint8 source = RAX; source < R15; source++)
		{
			Register s = (Register)source;
			
			EMIT_R_R(ADD, dest, s);
			
			//EmitREX2(dest, s);
			//EmitAddToRegister();
			//EmitDirect(dest, s);
			EmitREX(dest, s, dest);
			EmitAddToRegister();
			EmitIndirectIndex(dest, s, dest, X4);

			if ((s & 7) != RBP)
			{
				EmitIndirectIndex(dest, s, dest, X4);
			}
			else
			{
				EmitIndirectIndexedByteDisplayced(dest, s, dest, X4, 0);
			}
			if ((s & 7) != RSP && (s & 7) != RBP)
			{
				EMIT_R_M(ADD, dest, s);
				//EmitREX2(dest, s);
				//EmitAddToRegister();
				//EmitIndirect(dest, s);
				EmitAddToRegister();
				EmitIndirectByteDisplaced(dest, s, 0x12);
				EmitAddToRegister();
				EmitIndirectByteDisplaced(dest, s, 0x12345678);

			}

			if ((s & 7) == RSP)
			{
				EmitREX2(dest,s);
				EmitIndirectIndex(dest, s, RSP, X1);
				EmitAddToRegister();
			}
		}
	}

	FILE *f = fopen("test.bin", "wb");
	fwrite(f, emit_pointer - code, 1, f);
	fclose(f);


	//assembly_test();

	return (0);
}
