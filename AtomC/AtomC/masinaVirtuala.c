#include "main.h"

// This file contains VIRTUAL MACHINE

/* ----------------------------------------- STACK ----------------------------------------- */

char stack[STACK_SIZE];
char* SP; // Stack Pointer
char* stackAfter; // first byte after stack; used for stack limit tests

/* ------------------------------------ GLOBAL VARIABLES ------------------------------------ */

char globals[GLOBAL_SIZE];
int nGlobals;

void* allocGlobal(int size)
{
	void* p = globals + nGlobals;
	if (nGlobals + size > GLOBAL_SIZE) err("insufficient globals space");
	nGlobals += size;
	return p;
}

/* --------------------------------- STANDARD OPERATIONS --------------------------------- */

// The suffix d represents type: double
void pushd(double d)
{
	if (SP + sizeof(double) > stackAfter) err("out of stack");
	*(double*)SP = d;
	SP += sizeof(double);
}

double popd()
{
	SP -= sizeof(double);
	if (SP < stack)err("not enough stack bytes for popd");
	return *(double*)SP;
}

// The suffix i represents type: int
void pushi(int i)
{
	if (SP + sizeof(int) > stackAfter) err("out of stack");
	*(int*)SP = i;
	SP += sizeof(int);
}

int popi()
{
	SP -= sizeof(int);
	if (SP < stack) err("not enough stack bytes for popi");
	return *(int*)SP;
}

// The suffix c represents type: char
void pushc(char c)
{
	if (SP + sizeof(char) > stackAfter) err("out of stack");
	*(char*)SP = c;
	SP += sizeof(char);
}

char popc()
{
	SP -= sizeof(char);
	if (SP < stack) err("not enough stack bytes for popc");
	return *(char*)SP;
}

// The suffix a represents type: address
void pusha(void* a)
{
	if (SP + sizeof(void*) > stackAfter) err("out of stack");
	*(void**)SP = a;
	SP += sizeof(void*);
}

void* popa()
{
	SP -= sizeof(void*);
	if (SP < stack) err("not enough stack bytes for popa");
	return *(void**)SP;
}

/* ------------------------------------ FUNCTIONS ------------------------------------ */

Instr* instructions, * lastInstruction; // double linked list

// for operating with Instr structure the following functions are used  

Instr* createInstr(int opcode)
{
	Instr* i;
	SAFEALLOC(i, Instr)
	i->opcode = opcode;
	return i;
}

void insertInstrAfter(Instr* after, Instr* i)
{
	i->next = after->next;
	i->last = after;
	after->next = i;
	if (i->next == NULL)
		lastInstruction = i;
}

Instr* addInstr(int opcode)
{
	Instr* i = createInstr(opcode);
	i->next = NULL;
	i->last = lastInstruction;
	if (lastInstruction) {
		lastInstruction->next = i;
	}
	else {
		instructions = i;
	}
	lastInstruction = i;
	return i;
}

Instr* addInstrAfter(Instr* after, int opcode)
{
	Instr* i = createInstr(opcode);
	insertInstrAfter(after, i);
	return i;
}

Instr* addInstrA(int opcode, void* addr)
{
	Instr* i = createInstr(opcode);
	i->args[0].addr = addr;
	i->next = NULL;
	i->last = lastInstruction;
	if (lastInstruction) {
		lastInstruction->next = i;
	}
	else {
		instructions = i;
	}
	lastInstruction = i;
	return i;
}

Instr* addInstrI(int opcode, int val)
{
	Instr* i = createInstr(opcode);
	i->args[0].i = val;
	i->next = NULL;
	i->last = lastInstruction;
	if (lastInstruction) {
		lastInstruction->next = i;
	}
	else {
		instructions = i;
	}
	lastInstruction = i;
	return i;
}

Instr* addInstrII(int opcode, int val1, int val2)
{
	Instr* i = createInstr(opcode);
	i->args[0].i = val1;
	i->args[1].i = val2;
	i->next = NULL;
	i->last = lastInstruction;
	if (lastInstruction) {
		lastInstruction->next = i;
	}
	else {
		instructions = i;
	}
	lastInstruction = i;
	return i;
}

void deleteInstructionsAfter(Instr* start)
{
	Instr* currentInstr = instructions;
	while(currentInstr)
	{
		if (currentInstr == start)	// find start in instructions
		{			
			currentInstr = start->next;	
			while (currentInstr->next)
			{
				currentInstr = currentInstr->next;
				free(currentInstr->last);
			}
		}
		currentInstr = currentInstr->next;
	}
}

Symbol* requireSymbol(Symbols* symbols, const char* name)  // Find a symbol with given name in specified Symbols Table
{
	int index = (symbols->end) - (symbols->begin) - 1;
	for (int i = index; i >= 0; i--)
	{
		if (strcmp(symbols->begin[i]->name, name) == 0)
			return symbols->begin[i];
	}
	err("Undefined symbol!");
}

/* ------------------------------ MAIN FUNCTION FOR MV ------------------------------ */

void run(Instr* IP)
{
	int iVal1, iVal2;
	double dVal1, dVal2;
	char* aVal1;
	char* FP = 0, * oldSP;
	SP = stack;
	stackAfter = stack + STACK_SIZE;
	while (1) {
		printf("%p/%d\t", IP, SP - stack);
		switch (IP->opcode) {
		case O_CALL:
			aVal1 = IP->args[0].addr;
			printf("CALL\t%p\n", aVal1);
			pusha(IP->next);
			IP = (Instr*)aVal1;
			break;
		case O_CALLEXT:
			printf("CALLEXT\t%p\n", IP->args[0].addr);
			(*(void(*)())IP->args[0].addr)();
			IP = IP->next;
			break;
		case O_CAST_I_D:
			iVal1 = popi();
			dVal1 = (double)iVal1;
			printf("CAST_I_D\t(%d -> %g)\n", iVal1, dVal1);
			pushd(dVal1);
			IP = IP->next;
			break;
		case O_DROP:
			iVal1 = IP->args[0].i;
			printf("DROP\t%d\n", iVal1);
			if (SP - iVal1 < stack) err("not enough stack bytes");
			SP -= iVal1;
			IP = IP->next;
			break;
		case O_ENTER:
			iVal1 = IP->args[0].i;
			printf("ENTER\t%d\n", iVal1);
			pusha(FP);
			FP = SP;
			SP += iVal1;
			IP = IP->next;
			break;
		case O_EQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("EQ_D\t(%g==%g -> %d)\n", dVal2, dVal1, dVal2 == dVal1);
			pushi(dVal2 == dVal1);
			IP = IP->next;
			break;
		case O_HALT:
			printf("HALT\n");
			return;
		case O_INSERT:
			iVal1 = IP->args[0].i; // iDst
			iVal2 = IP->args[1].i; // nBytes
			printf("INSERT\t%d,%d\n", iVal1, iVal2);
			if (SP + iVal2 > stackAfter) err("out of stack");
			memmove(SP - iVal1 + iVal2, SP - iVal1, iVal1); //make room
			memmove(SP - iVal1, SP + iVal2, iVal2); //dup
			SP += iVal2;
			IP = IP->next;
			break;
		case O_JT_I:
			iVal1 = popi();
			printf("JT_I\t%p\t(%d)\n", IP->args[0].addr, iVal1);
			IP = iVal1 ? IP->args[0].addr : IP->next;
			break;
		case O_LOAD:
			iVal1 = IP->args[0].i;
			aVal1 = popa();
			printf("LOAD\t%d\t(%p)\n", iVal1, aVal1);
			if (SP + iVal1 > stackAfter) err("out of stack");
			memcpy(SP, aVal1, iVal1);
			SP += iVal1;
			IP = IP->next;
			break;
		case O_OFFSET:
			iVal1 = popi();
			aVal1 = popa();
			printf("OFFSET\t(%p+%d -> %p)\n", aVal1, iVal1, aVal1 + iVal1);
			pusha(aVal1 + iVal1);
			IP = IP->next;
			break;
		case O_PUSHFPADDR:
			iVal1 = IP->args[0].i;
			printf("PUSHFPADDR\t%d\t(%p)\n", iVal1, FP + iVal1);
			pusha(FP + iVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_A:
			aVal1 = IP->args[0].addr;
			printf("PUSHCT_A\t%p\n", aVal1);
			pusha(aVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_I:
			iVal1 = IP->args[0].i;
			printf("PUSHCT_I\t%p\n", iVal1);
			pushi(iVal1);
			IP = IP->next;
			break;
		case O_RET:
			iVal1 = IP->args[0].i; // sizeArgs
			iVal2 = IP->args[1].i; // sizeof(retType)
			printf("RET\t%d,%d\n", iVal1, iVal2);
			oldSP = SP;
			SP = FP;
			FP = popa();
			IP = popa();
			if (SP - iVal1 < stack) err("not enough stack bytes");
			SP -= iVal1;
			memmove(SP, oldSP - iVal2, iVal2);
			SP += iVal2;
			break;
		case O_STORE:
			iVal1 = IP->args[0].i;
			if (SP - (sizeof(void*) + iVal1) < stack) err("not enough stack bytes for SET");
			aVal1 = *(void**)(SP - ((sizeof(void*) + iVal1)));
			printf("STORE\t%d\t(%p)\n", iVal1, aVal1);
			memcpy(aVal1, SP - iVal1, iVal1);
			SP -= sizeof(void*) + iVal1;
			IP = IP->next;
			break;
		case O_SUB_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("SUB_D\t(%g-%g -> %g)\n", dVal2, dVal1, dVal2 - dVal1);
			pushd(dVal2 - dVal1);
			IP = IP->next;
			break;
		case O_SUB_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("SUB_i\t(%d-%d -> %d)\n", iVal2, iVal1, iVal2 - iVal1);
			pushi(iVal2 - iVal1);
			IP = IP->next;
			break;
		default:
			err("invalid opcode: %d", IP->opcode);
		}
	}
}