
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#define Assert(x) \
	if(!(x)) { MessageBoxA(0, #x, "Faiuree", MB_OK); __debugbreak(); }

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
	R15 = 15
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
	Assert(rx < 8);
	Assert(rm < 8);
	Emit(rm | (rx << 3) | (mod << 6));
}


// add rax, rcx: rx = RAX, operand = RCX
void EmitDirect(uint8 rx, Register operand)
{
	EmitRxMode(DIRECT, rx, operand);
}

//add rax, [rcx]: rx = RAX, operand = RCX
void EmitIndirect(uint8 rx, Register operand)
{
	Assert(operand != RSP);
	Assert(operand != RBP);
	EmitRxMode(INDIRECT, rx, operand);
}

void EmitDisplaced(uint8 rx, uint32 disp)
{
	EmitRxMode(INDIRECT, rx, RBP);
	Emit4disp(disp);
}

//add rax, [rcx + 0x12]: rx = RAX, operand = RCX, displacement = 0x12;
void EmitIndirectOperandWithSmallDisp(uint8 rx, Register operand, uint8 displacement)
{
	Assert(operand != RSP);
	EmitRxMode(BYTE_DISPALCED_INDIRECT, rx, operand);
	Emit(displacement);
}

// add rax, [rcx + 0x12345678]: rx = RAX, operand = RCX, dispalcemnt = 0x12345678
void EmitIndirectOperandWithBigDisp(uint8 rx, Register operand, uint8 displacement)
{
	Assert(operand != RSP);
	EmitRxMode(DISPLACED_INDIRECT, rx, operand);
	Emit4disp(displacement);
}

//add rax, [rcx + 4 * rdx]: rx = RAX, operand = RCX, index = RDX, scale = X4;
void EmitIndirectIndex(uint8 rx, Register operand, Register index, Scale scale)
{
	Assert(scale < 4);
	EmitRxMode(INDIRECT, rx, RSP);
	EmitRxMode(scale, operand, index);
}

//add rax, [rcx + 4 * rdx + 0x12]: rx = RAX, operand = RCX, index = RDX, scale = X4, displacement = 0x12
void EmitByteIndirectOperand(uint8 rx, Register operand, Register index, Scale scale, uint32 disp)
{
	Assert(scale < 4);
	EmitRxMode(BYTE_DISPALCED_INDIRECT, rx, 4);
	EmitRxMode(scale, operand, index);
	Emit(disp);
}

//add rax, [rcx + 4 * rdx + 0x12345678]: rx = RAX, operand = RCX, index = RDX, scale = X4, displacement = 0x12345678
void EmitIndirectOperand(uint8 rx, Register operand, Register index, Scale scale, uint32 disp)
{
	Assert(scale < 4);
	EmitRxMode(DISPLACED_INDIRECT, rx, 4);
	EmitRxMode(scale, operand, index);
	Emit4disp(disp);
}

void EmitAdd()
{
	Emit(0x48);
	Emit(0x03);
}

int main(int argc, char **argv)
{
	for (uint8 dest = RAX; dest < R8; dest++)
	{
		for (uint8 s = RAX; s < R8; s++)
		{
			EmitAdd();
			EmitDirect((Register)dest, (Register)s);
			if (s != RSP && s != RBP)
			{
				EmitAdd();
				EmitIndirect((Register)dest, (Register)s);
			}
			if (s == RBP)
			{

			}
		}
	}

	FILE *f = fopen("test.bin", "wb");
	fwrite(f, emit_pointer - code, 1, f);
	fclose(f);

	//assembly_test();

	return (0);
}
