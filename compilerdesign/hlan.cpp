
#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#define Assert(x) \
    if(!(x)) { MessageBoxA(0, #x, "Failuree", MB_OK); __debugbreak(); }

extern "C" void assembly_test();

typedef uint32_t uint32;
typedef uint64_t uint64;

//Emitter

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
    R15 = 15,
};

typedef uint8_t Register;

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

enum ConditionCode
{
    O, 
    NO, 
    B,
    NB,
    E,
    NE,
    NA,
    A,
    S,
    NS,
    P,
    NP,
    L,
    NL,
    NG,
    G,
    NAE = B,
    C = B,
    AE = NB,
    NC = NB,
    Z = E,
    NZ = NE,
    BE = NA,
    NBE = A,
    PE = P,
    PO = NP,
    NGE = L,
    GE = NL,
    LE = NG,
    NLE = G
};


uint8_t code[MAX_CODE];
uint8_t* emit_pointer = code;

void Emit(uint8_t byte)
{
    *emit_pointer = byte;
    emit_pointer++;
}

void Emit4disp(uint32 w)
{
    Emit(w & 0xFF);
    Emit((w >> 8) & 0xFF);
    Emit((w >> 16) & 0xFF);
    Emit((w >> 24) & 0xFF);
}

void Emit8disp(uint64 w)
{
    Emit(w & 0xFF);
    Emit((w >> 8) & 0xFF);
    Emit((w >> 16) & 0xFF);
    Emit((w >> 24) & 0xFF);

    Emit((w >> 32) & 0xFF);
    Emit((w >> 40) & 0xFF);
    Emit((w >> 48) & 0xFF);
    Emit((w >> 56) & 0xFF);
}


void EmitRxMode(uint8_t mod, uint8_t rx, uint8_t rm)
{
    Assert(mod < 4);
    Assert(rx < 16);
    Assert(rm < 16);
    
    Emit((rm &= 7) | ((rx &= 7) << 3) | (mod << 6));
}


// add rax, rcx: rx = RAX, operand = RCX
void EmitDirect(uint8_t rx, Register operand)
{
    EmitRxMode(DIRECT, rx, operand);
}

//add rax, [rcx]: rx = RAX, operand = RCX
void EmitIndirect(uint8_t rx, Register base)
{
    Assert((base & 7) != RSP);
    Assert((base & 7) != RBP);
    EmitRxMode(INDIRECT, rx, base);
}

//add rax, [rip + 0x12345678]
//rax = RAX, disp = 0x12345678
void EmitIndirectDisplacedRip(uint8_t rx, uint32 disp)
{
    EmitRxMode(INDIRECT, rx, RBP);
    Emit4disp(disp);
}

void EmitDisplaced(uint8_t rx, uint32 disp)
{
    EmitRxMode(INDIRECT, rx, RBP);
    Emit4disp(disp);
}

void EmitDispla(uint8_t rx, uint32 disp)
{
    EmitRxMode(INDIRECT, rx, RSP);
    EmitRxMode(X1, RSP, RBP);
    Emit4disp(disp);
}

//add rax, [rcx + 0x12]: rx = RAX, operand = RCX, displacement = 0x12;
void EmitIndirectByteDisplaced(uint8_t rx, Register base, int displacement)
{
    Assert(base != RSP);
    EmitRxMode(BYTE_DISPALCED_INDIRECT, rx, base);
    Emit(displacement);
}

// add rax, [rcx + 0x12345678]: rx = RAX, operand = RCX, dispalcemnt = 0x12345678
void EmitIndirectDisplaced(uint8_t rx, Register base, int displacement)
{
    Assert((base & 7) != RSP);
    EmitRxMode(DISPLACED_INDIRECT, rx, base);
    Emit4disp(displacement);
}

//add rax, [rcx + 4 * rdx]: rx = RAX, operand = RCX, index = RDX, scale = X4;
void EmitIndirectIndex(uint8_t rx, Register base, Register index, Scale scale)
{
    Assert((base & 7) != RBP);
    EmitRxMode(INDIRECT, rx, RSP);
    EmitRxMode(scale, index, base);
}

//add rax, [rcx + 4 * rdx + 0x12]: rx = RAX, operand = RCX, index = RDX, scale = X4, displacement = 0x12
void EmitIndirectIndexedByteDisplaced(uint8_t rx, Register operand, Register index, Scale scale, uint32 disp)
{
    Assert(scale < 4);
    EmitRxMode(BYTE_DISPALCED_INDIRECT, rx, 4);
    EmitRxMode(scale, operand, index);
    Emit(disp);
}

//add rax, [rcx + 4 * rdx + 0x12345678]: rx = RAX, operand = RCX, index = RDX, scale = X4, displacement = 0x12345678
void EmitIndirectIndexedDisplaced(uint8_t rx, Register operand, Register index, Scale scale, uint32 disp)
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

#define EMIT_D_R(operation, destination, source) \
    EmitREX2(source, (Register)0); \
    Emit_##operation##_M(); \
    EmitDisplaced(source, dest)


#define EMIT_D_I(operation, destination, source_immediate) \
    EmitREX2((Register)0, (Register)0); \
    Emit_##operation##_I(); \
    EmitDisplaced(extension_##operation##_I, destination); \
    Emit4disp(source_immediate)


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

#define EMIT_RIPD_R(operation, destination_disp, source) \
    EmitREX2(source, (Register)0); \
    Emit_##operation##_M(); \
    EmitIndirectDisplacedRip(source, destination_disp);


#define EMIT_RIPD_I(operation, destination_disp, source_immediate) \
    EmitREX2((Register)0, (Register)0); \
    Emit_##operation##_I(); \
    EmitIndirectDisplacedRip(extension_##operation##_I, destination_disp); \
    Emit4disp(source_immediate)


#define EMIT_X_R(operation, source) \
    EmitREX2((Register)0, source); \
    Emit_##operation##_X(); \
    EmitDirect(extension_##operation##_X, source)

#define EMIT_X_M(operation, source) \
    EmitREX2((Register)0, source); \
    Emit_##operation##_X(); \
    EmitIndirect(extension_##operation##_X, source)

#define EMIT_X_RIPD(operation, source_disp) \
    EmitREX2((Register)0, (Register)0); \
    Emit_##operation##_X(); \
    EmitIndirectDisplacedRip(extension_##operation##_, source_disp)

#define EMIT_X_D(operation, source) \
    EmitREX2((Register)0, (Register)0); \
    Emit_##operation##_X(); \
    EmitDisplaced(extension_##opeartion##_X, source)


#define EMIT_X_MD1(operation, source, disp) \
    EmitREX2((Register)0, source); \
    Emit_##operation##_X(); \
    EmitIndirectByteDisplaced(extension_##operation##_X, source, disp)


#define EMIT_X_MD(operation, source, disp) \
    EmitREX2((Register)0, source); \
    Emit_##operation##_X(); \
    EmitIndirectDisplaced(extension_##operation##_X, source, disp)

#define EMIT_X_SIB(operation, source_base, source_scale, source_index) \
    EmitREX((Register)0, source_base, source_index); \
    Emit_##operation##_X(); \
    EmitIndirectIndex(extension_##operation##_X, source_base, source_index, source_scale)

#define EMIT_X_SIBD1(operation, source_base, source_scale, source_index, displacement) \
    EmitREX((Register)0, source_base, source_index); \
    Emit_##operation##_X(); \
    EmitIndirectIndexedByteDisplaced(extension_##operation##_X, source_base, source_index, source_scale, displacement)

#define EMIT_X_SIBD(operation, source_base, source_scale, source_index, displacement) \
    EmitREX((Register)0, source_base, source_index); \
    Emit_##operation##_X(); \
    EmitIndirectIndexedDisplaced(extension_##operation##_X, source_base, source_index, source_scale, displacement)

#define EMIT_I(operation, source_immediate) \
    EmitREX2((Register)0, (Register)0); \
    Emit_##operation##_I(); \
    Emit4disp(source_immediate);

#define EMIT_C_I(operation, condition_code, source_immediate) \
    EmitREX2((Register)0, (Register)0); \
    Emit_##operation##_C_I(condition_code); \
    Emit4disp(source_immediate);

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


#define OP1X(operation, opcode, extension) \
    void Emit_##operation##_X() { \
        Emit(opcode); \
    } \
    enum { extension_##operation##_X = extension };

#define OP1CI(operation, opcode, extension) \
    void Emit_##operation##_C_I(ConditionCode code) { \
        Emit(opcode + code); \
    } 

#define OP2CI(operation, opcode) \
    void Emit_##operation##_C_I(ConditionCode code) { \
        Emit(0x0F); \
        Emit(opcode + code); \
    } 

#define EMIT_MOV_R_I(destination_register, source_intermeddiate) \
    EmitREX2(destination_register, (Register)0); \
    Emit(0xB8); \
    Emit8disp(source_intermeddiate)

#define EMIT_MOV_RAX_MOFF(source_offset) \
    EmitREX2((Register)0, (Register)0); \
    Emit(0xA1); \
    Emit8disp(source_offset)

#define EMIT_MOV_MOFF_RAX(destination_offset) \
    EmitREX2((Register)0, (Register)0); \
    Emit(0xA3); \
    Emit8disp(destination_offset)


OP1M(MOV, 0x8B)
OP1R(MOV, 0x89)

OP1R(ADD, 0x03)
OP1M(ADD, 0x01)
OP1I(ADD, 0x81, 0x00)

OP1R(AND, 0x23)
OP1M(AND, 0x21)
OP1I(AND, 0x81, 0x04)

OP1X(MUL, 0xF7, 0x04)

OP1I(JMP, 0xE9, 0x00)

//byte immediate : OP1CI(J, 0x70)

OP2CI(J, 0x80)  

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

// Reader


char cur_character;
char *remaining_characters;

enum Token
{
    TOKEN_EOF = 0,
    LAST_LITERAL_TOKEN = 127,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER

};

Token cur_token;
uint64 token_integer;

void ReadCharacters()
{
    Assert(cur_character != 0);
    cur_character = *remaining_characters;
    remaining_characters++;
}

void ReadToken()
{
retry:
    switch (cur_character)
    {
    case 0:
        cur_token = TOKEN_EOF;
        break;
    case ' ':
    case '\n':
    case '\r':
    case '\t':
    case '\v':
        ReadCharacters();
        goto retry;
        break;
    case '+':
    case '-':
    case '*':
    case '/':
    case '(':
    case ')':
    case '^':
        cur_token = (Token)cur_character;
        ReadCharacters();
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
        cur_token = TOKEN_INTEGER;
        token_integer = cur_character - '0';
        ReadCharacters();
        while (isdigit(cur_character))
        {
            token_integer *= 10;
            token_integer += cur_character - '0';
            ReadCharacters();
        }
        break;
    }

    }
}

void ExpectToken(Token expected)
{
    Assert(cur_token == expected);
    ReadToken();
}

Register next_register;

Register AllocateRegister()
{
    Assert(next_register <= R15);
    Register cur_reg = next_register;
    next_register++;
    return cur_reg;
}

void Free()
{
    Assert(next_register >= RAX);
    next_register--;
}

Register ParseExpr();

Register ParsePOW()
{
    Register target = AllocateRegister();
    if (cur_token == TOKEN_INTEGER)
    {
        ReadToken();
        EMIT_MOV_R_I(target, token_integer);
        return target;
    }
    else if (cur_token == '(')
    {
        ReadToken();
        uint64 value = ParseExpr();
        ExpectToken((Token)')');
        return value;
    }
    Assert(0);
    return(0);
}

Register ParseFactor()
{
    Register reg = ParsePOW();
   
#if 0
    if (cur_token == '^')
    {
        ReadToken();
        uint64 pow = ParseFactor();
        uint64 base = token;
        token = 1;
        for (size_t i = 0; i < pow; i++)
        {
            token *= base;
        }
    }
#endif

    return reg;
}

Register ParseTerm()
{
    Register val = ParseFactor();
    while (cur_token == '*' || cur_token == '/')
    {
        Token operator_t = cur_token;
        ReadToken();
        if (operator_t == '*')
        {
            val *= ParseFactor();
        }
        else
        {
            val /= ParseFactor();
        }

    }
    return (val);
}

Register ParseExpr()
{
    uint64 val = ParseTerm();
//r:
//    switch (cur_token)
//    {
//        case '+':
//        {
//            ReadToken();
//            val += ParseTerm();
//            goto r;
//        }
//        case '-':
//        {
//            ReadToken();
//            val -= ParseTerm();
//            goto r;
//        }
//    }

    while (cur_token == '+' || cur_token == '-')
    {
        Token operator_t = cur_token;
        ReadToken();
        if (operator_t == '+')
        {
            val += ParseTerm();
        }
        else
        {
            val -= ParseTerm();
        }
        
    }
    return (val);
}


void ParsingFile(const char *filename)
{
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    Assert(file != INVALID_HANDLE_VALUE);
    DWORD file_size = GetFileSize(file, 0);
    Assert(file_size > 0);
    HANDLE file_mapping = CreateFileMappingA(file, 0, PAGE_WRITECOPY, 0, 0, 0);
    Assert(file_mapping);
    char *file_memory = (char*)MapViewOfFileEx(file_mapping, FILE_MAP_COPY, 0, 0, 0, 0);
    Assert(file_memory);
    Assert(file_memory[file_size - 1] == '\n');
    file_memory[file_size - 1] = 0;
    cur_character = *file_memory;
    remaining_characters = file_memory + 1;
    ReadToken();
}


void Test()
{
    EMIT_MOV_R_I(RBX, 0x12345678deadbeefull);
    EMIT_MOV_RAX_MOFF(0x12345678deadbeefull);
    EMIT_MOV_MOFF_RAX(0x12345678deadbeefull);
    EMIT_R_R(MOV, RAX, R10);
    EMIT_M_R(MOV, RAX, R10);
    EMIT_I(JMP, -0x1234);
    EMIT_C_I(J, NZ, 0x1234);
    EMIT_D_I(ADD, 0x12345678, 0xDEADBEEF);
    EMIT_RIPD_I(ADD, 0x12345678, 0xDEADBEEF);

    for (uint8_t d = RAX; d < R15; d++)
    {
        Register dest = (Register)d;
        EMIT_X_R(MUL, dest);
        if ((dest & 7) != RSP)
        {
            EMIT_X_MD1(MUL, dest, 0x12);
            EMIT_X_MD(MUL, dest, 0x12345678);
            if ((dest & 7) != RBP)
            {
                EMIT_X_M(MUL, dest);
                EMIT_X_SIB(MUL, dest, X4, R8);
                EMIT_X_SIBD1(MUL, dest, X4, R8, 0x12);
                EMIT_X_SIBD(MUL, dest, X4, R8, 0x12345678);
            }
        }

        if ((dest & 7) != RSP)
        {
            if ((dest & 7) != RBP)
            {
                //EMIT_X_M(MUL);
            }
        }
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
        EMIT_RIPD_R(ADD, 0x12345678, dest);
        EMIT_D_R(ADD, 0x12345678, dest);


        for (uint8_t source = RAX; source < R15; source++)
        {
            Register s = (Register)source;

            EMIT_R_R(ADD, dest, s);

            if ((s & 7) != RBP)
            {
                EMIT_R_SIB(ADD, dest, s, X4, dest);
            }

            /*		if ((s & 7) != RBP)
            {
            EmitIndirectIndex(dest, s, dest, X4);
            }
            else
            {
            EmitIndirectIndexedByteDisplaced(dest, s, dest, X4, 0);
            }*/
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
                EmitREX2(dest, s);
                EmitIndirectIndex(dest, s, RSP, X1);
                EmitAddToRegister();
#endif
            }
        }
    }

    FILE *f = fopen("test.bin", "wb");
    fwrite(f, emit_pointer - code, 1, f);
    fclose(f);

}


int main(int argc, char **argv)
{
    ParsingFile("m.hlan");
    uint64 res = ParseExpr();
    int i = 0x0001;
    return (0);
}
