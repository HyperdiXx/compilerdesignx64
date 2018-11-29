
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
void EmitIndirectDisplacedRip(uint8 rx, uint32 disp)
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
void EmitIndirectDisplaced(uint8 rx, Register base, int displacement)
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
void EmitIndirectIndexedByteDisplaced(uint8 rx, Register operand, Register index, Scale scale, uint32 disp)
{
	Assert(scale < 4);
	EmitRxMode(BYTE_DISPALCED_INDIRECT, rx, 4);
	EmitRxMode(scale, operand, index);
	Emit(disp);
}

//add rax, [rcx + 4 * rdx + 0x12345678]: rx = RAX, operand = RCX, index = RDX, scale = X4, displacement = 0x12345678
void EmitIndirectIndexedDisplaced(uint8 rx, Register operand, Register index, Scale scale, uint32 disp)
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



#define EMIT_R_R(operation, dest, source) \
	EmitREX2(dest, source); \
	Emit_##operation##_R(); \
	EmitDirect(dest, source)

#define EMIT_R_M(operation, destination, source) \
	EmitREX2(dest, source); \
	Emit_##operation##_R(); \
	EmitIndirect(dest, source)

#define EMIT_R_RIPD(operation, destination, rip_disp) \
	EmitREX2(destination, (Register)0); \
	Emit_##operation##_R(); \
	EmitIndirectDisplacedRip(dest, rip_disp)

#define EMIT_R_D(operation, destination, source) \
	EmitREX2(dest, (Register)0); \
	Emit_##operation##_R(); \
	EmitDisplaced(dest, source)

#define EMIT_R_MD1(operation, dest, source, disp) \
	EmitREX2(dest, source); \
	Emit_##operation##_R(); \
	EmitIndirectByteDisplaced(dest, source, disp)


#define EMIT_R_MD(operation, dest, source, disp) \
	EmitREX2(dest, source); \
	Emit_##operation##_R(); \
	EmitIndirectDisplaced(dest, source, disp)

#define EMIT_R_SIB(operation, destination, source_base, source_scale, source_index) \
	EmitREX(destination, source_base, source_index); \
	Emit_##operation##_R(); \
	EmitIndirectIndex(destination, source_base, source_index, source_scale)

#define EMIT_R_SIBD1(operation, destination, source_base, source_scale, source_index, displacement) \
	EmitREX(destination, source_base, source_index); \
	Emit_##operation##_R(); \
	EmitIndirectIndexedByteDisplaced(destination, source_base, source_index, source_scale, displacement)

#define EMIT_R_SIBD(operation, destination, source_base, source_scale, source_index, displacement) \
	EmitREX(destination, source_base, source_index); \
	Emit_##operation##_R(); \
	EmitIndirectIndexedDisplaced(destination, source_base, source_index, source_scale, displacement)

#define EMIT_M_R(operation, destination, source) \
	EmitREX2(source, destination); \
	Emit_##operation##_M(); \
	EmitIndirect(source, destination);

#define EMIT_MD1_R(operation, destination, displacement, source) \
	EmitREX2(source, destination); \
	Emit_##operation##_M(); \
	EmitIndirectByteDisplaced(source, destination, displacement);

#define EMIT_MD_R(operation, destination, displacement, source) \
	EmitREX2(source, destination); \
	Emit_##operation##_M(); \
	EmitIndirectDisplaced(source, destination, displacement);

#define EMIT_SIB_R(operation, destination_base, destination_scale, destination_index, source) \
	EmitREX(source, destination_base, destination_index); \
	Emit_##opeartion##_M(); \
	EmitIndirectIndex(source, destination_base, destination_index, destination_scale)

#define EMIT_SIBD1_R(operation, destination_base, destination_scale, destination_index, destination_displacement, source) \
	EmitREX(source, destination_base, destination_index); \
	Emit_##opeartion##_M(); \
	EmitIndirectIndexedByteDisplaced(source, destination_base, destination_index, destination_scale, destination_displacement)


#define EMIT_SIBD_R(operation, destination_base, destination_scale, destination_index, destination_displacement, source) \
	EmitREX(source, destination_base, destination_index); \
	Emit_##opeartion##_M(); \
	EmitIndirectIndexedDisplaced(source, destination_base, destination_index, destination_scale, destination_displacement)

#define EMIT_R_I(operation, destination, source_immediate) \
	EmitREX2((Register)0, destination); \
	Emit_##operation##_I(); \
	EmitDirect(extension_##operation##_I, destination); \
	Emit4disp(source_immediate)


#define EMIT_M_I(operation, destination, source_immediate) \
	EmitREX2((Register)0, destination); \
	Emit_##operation##_I(); \
	EmitIndirect(extension_##operation##_I, destination); \
	Emit4disp(source_immediate)

#define EMIT_MD1_I(operation, destination, destination_displacement, source_immediate) \
	EmitREX2((Register)0, destination); \
	Emit_##operation##_I(); \
	EmitIndirectByteDisplaced(extension_##operation##_I, destination, destination_displacement); \
	Emit4disp(source_immediate)

#define EMIT_MD_I(operation, destination, destination_displacement, source_immediate) \
	EmitREX2((Register)0, destination); \
	Emit_##operation##_I(); \
	EmitIndirectDisplaced(extension_##operation##_I, destination, destination_displacement); \
	Emit4disp(source_immediate)


#define EMIT_SIB_I(operation, destination_base, destination_scale, destination_index, source_immediate) \
	EmitREX((Register)0, destination_base, destination_index); \
	Emit_##operation##_I(); \
	EmitIndirectIndex(extension_##operation##_I, destination_base, destination_index, destination_scale); \
	Emit4disp(source_immediate)


#define EMIT_SIBD1_I(operation, destination_base, destination_scale, destination_index, destination_displacement, source_immediate) \
	EmitREX((Register)0, destination_base, destination_index); \
	Emit_##operation##_I(); \
	EmitIndirectIndexedByteDisplaced(extension_##operation##_I, destination_base, destination_index, destination_scale, destination_displacement); \
	Emit4disp(source_immediate)

#define EMIT_SIBD_I(operation, destination_base, destination_scale, destination_index, destination_displacement, source_immediate) \
	EmitREX((Register)0, destination_base, destination_index); \
	Emit_##operation##_I(); \
	EmitIndirectIndexedDisplaced(extension_##operation##_I, destination_base, destination_index, destination_scale, destination_displacement); \
	Emit4disp(source_immediate)


#define OP1R(operation, opcode) \
	void Emit_##operation##_R() { \
		Emit(opcode); \
	}

#define OP1M(operation, opcode) \
	void Emit_##operation##_M() { \
		Emit(opcode); \
	}

#define OP1I(operation, opcode, extension) \
	void Emit_##operation##_I() { \
		Emit(opcode); \
	} \
	enum { extension_##operation##_I = extension };

OP1R(ADD, 0x03)
OP1M(ADD, 0x01)
OP1I(ADD, 0x81, 0x00)


//Instructions
#if 0
void Emit_ADD_R()
{
	Emit(0x03);
}

void Emit_ADD_M()
{
	Emit(0x01);
}

void Emit_ADD_I()
{
	Emit(0x81);
}

enum { extension_ADD_I = 0 };

#endif // 



int main(int argc, char **argv)
{

	for (uint8 d = RAX; d < R15; d++)
	{
		Register dest = (Register)d;
		EMIT_R_I(ADD, dest, 0x12345678);
		if ((dest & 7) != RSP)
		{
			EMIT_MD1_I(ADD, dest, 0x12, 0xDEADBEEF);
			EMIT_MD_I(ADD, dest, 0x12345678, 0xDEAFBEEF);
			if ((dest & 7) != RBP)
			{
				EMIT_SIB_I(ADD, dest, X4, R8, 0xDEADBEEF);
			}
		}
		EMIT_R_RIPD(ADD, dest, 0x12345678);
		EMIT_R_D(ADD, dest, 0x12345678);

		/*EmitREX2(dest, (Register)0);
		EmitAddToRegister();
		EmitIndirectDisplayRip(dest, 0x12345678);
		EmitREX2(dest, (Register)0);
		EmitAddToRegister();
		EmitDisplaced(dest, 0x12345678);*/

		for (uint8 source = RAX; source < R15; source++)
		{
			Register s = (Register)source;
			
			EMIT_R_R(ADD, dest, s);
			
			if ((s & 7) != RBP)
			{
				EMIT_R_SIB(ADD, dest, s, X4, dest);
			}

			//EmitREX2(dest, s);
			//EmitAddToRegister();
			//EmitDirect(dest, s);
			//EmitREX(dest, s, dest);
			//EmitAddToRegister();
			//EmitIndirectIndex(dest, s, dest, X4);

			if ((s & 7) != RBP)
			{
				EmitIndirectIndex(dest, s, dest, X4);
			}
			else
			{
				EmitIndirectIndexedByteDisplaced(dest, s, dest, X4, 0);
			}
			if ((s & 7) != RSP && (s & 7) != RBP)
			{
				EMIT_R_M(ADD, dest, s);
				EMIT_R_MD1(ADD, dest, s, 0x12);
				EMIT_R_MD(ADD, dest, s, 0x12345678);
			}

			if ((dest & 7) != RSP && (dest & 7) != RBP)
			{
				EMIT_M_R(ADD, dest, s);
				EMIT_MD1_R(ADD, dest, 0x12, s);
				EMIT_MD_R(ADD, dest, 0x12345678, s);
			}

			if ((s & 7) == RSP)
			{
				EMIT_R_SIB(ADD, dest, s, X1, RSP);

				#if 0
				EmitREX2(dest,s);
				EmitIndirectIndex(dest, s, RSP, X1);
				EmitAddToRegister();
#endif
			}
		}
	}

	FILE *f = fopen("test.bin", "wb");
	fwrite(f, emit_pointer - code, 1, f);
	fclose(f);


	//assembly_test();

	return (0);
}
