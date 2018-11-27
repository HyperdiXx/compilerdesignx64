#include <windows.h>
#include <stdint.h>

#define Assert(x) \
	if(!(x)) { MessageBoxA(0, #x, "Faiuree", MB_OK); __debugbreak(); }

extern "C" void assembly_test();

typedef uint8_t uint8;

enum
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

void EmitRxMode(uint8 mod, uint8 rx, uint8 rm)
{
	Emit(rm | (rx << 3) | (mod << 6));
}

void EmitDirect(uint8 rx, uint8 rm)
{
	EmitRxMode(3, rx, rm);
}

void EmitIndirect(uint8 rx, uint8 rm)
{
	Assert(rm != RSP);
	EmitRxMode(0, rx, rm);
}

void EmitIndirectOperandWithSmallDisp(uint8 rx, uint8 rm, uint8 displacement)
{
	Assert(rm != RSP);
	EmitRxMode(1, rx, rm);
	Emit(displacement);
}

void EmitIndirectOperandWithBigDisp(uint8 rx, uint8 rm, uint8 displacement)
{
	Assert(rm != RSP);
	EmitRxMode(2, rx, rm);
	Emit(displacement && 0xFF);
	Emit(displacement >> 8 && 0xFF);
	Emit(displacement >> 16 && 0xFF);
	Emit(displacement >> 24 && 0xFF);
}


int main(int argc, char **argv)
{
	EmitRxMode(0x3, 0x4, 0x1);

	assembly_test();

	return (0);
}
