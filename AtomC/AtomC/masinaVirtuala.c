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
	if (instructions == start || lastInstruction == start)
	{
		return;
	}

	for (Instr* i = instructions->next; i != lastInstruction; i = i->next)
	{
		if (i == start)
		{
			while (lastInstruction != start)
			{
				i = lastInstruction;
				lastInstruction = i->last;
				free(i);
			}
			return;
		}
	}
	//start->next = NULL;
	//lastInstruction = start;
}

/* ------------------------------ MAIN FUNCTION FOR MV ------------------------------ */

void run(Instr* IP)
{
	int iVal1, iVal2;
	char cVal1, cVal2;
	double dVal1, dVal2;
	char* aVal1, aVal2;
	char* FP = 0, * oldSP;
	SP = stack;
	stackAfter = stack + STACK_SIZE;
	while (1) {
		printf("%p/%d\t", IP, SP - stack);
		switch (IP->opcode) {
		case O_ADD_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("ADD_C\t(%c+%c -> %c)\n", cVal2, cVal1, cVal2 + cVal1);
			pushc(cVal2 + cVal1);
			IP = IP->next;
			break;
		case O_ADD_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("ADD_D\t(%g+%g -> %g)\n", dVal2, dVal1, dVal2 + dVal1);
			pushd(dVal2 + dVal1);
			IP = IP->next;
			break;
		case O_ADD_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("ADD_I\t(%d+%d -> %d)\n", iVal2, iVal1, iVal2 + iVal1);
			pushi(iVal2 + iVal1);
			IP = IP->next;
			break;
		case O_AND_A:
			aVal1 = popa();
			aVal2 = popa();
			printf("AND_A\t(%p&&%p -> %d)\n", aVal2, aVal1, aVal2 && aVal1);
			pushi(aVal2 && aVal1);
			IP = IP->next;
			break;
		case O_AND_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("AND_C\t(%c&&%c -> %d)\n", cVal2, cVal1, cVal2 && cVal1);
			pushi(cVal2 && cVal1);
			IP = IP->next;
			break;
		case O_AND_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("AND_D\t(%g&&%g -> %d)\n", dVal2, dVal1, dVal2 && dVal1);
			pushi(dVal2 && dVal1);
			IP = IP->next;
			break;
		case O_AND_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("AND_I\t(%d&&%d -> %d)\n", iVal2, iVal1, iVal2 && iVal1);
			pushi(iVal2 && iVal1);
			IP = IP->next;
			break;
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
		case O_CAST_C_D:
			cVal1 = popc();
			dVal1 = (double)cVal1;
			printf("CAST_C_D\t(%c -> %g)\n", cVal1, dVal1);
			pushd(dVal1);
			IP = IP->next;
			break;
		case O_CAST_C_I:
			cVal1 = popc();
			iVal1 = (int)cVal1;
			printf("CAST_C_I\t(%c -> %d)\n", cVal1, iVal1);
			pushi(iVal1);
			IP = IP->next;
			break;
		case O_CAST_D_C:
			dVal1 = popd();
			cVal1 = (char)dVal1;
			printf("CAST_D_C\t(%g -> %c)\n", dVal1, cVal1);
			pushc(cVal1);
			IP = IP->next;
			break;
		case O_CAST_D_I:
			dVal1 = popd();
			iVal1 = (int)dVal1;
			printf("CAST_D_I\t(%g -> %d)\n", dVal1, iVal1);
			pushi(iVal1);
			IP = IP->next;
			break;
		case O_CAST_I_C:
			iVal1 = popi();
			cVal1 = (char)iVal1;
			printf("CAST_I_C\t(%d -> %c)\n", iVal1, cVal1);
			pushc(cVal1);
			IP = IP->next;
			break;
		case O_CAST_I_D:
			iVal1 = popi();
			dVal1 = (double)iVal1;
			printf("CAST_I_D\t(%d -> %g)\n", iVal1, dVal1);
			pushd(dVal1);
			IP = IP->next;
			break;
		case O_DIV_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("DIV_C\t(%c/%c -> %c)\n", cVal2, cVal1, cVal2 / cVal1);
			pushc(cVal2 / cVal1);
			IP = IP->next;
			break;
		case O_DIV_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("DIV_D\t(%g/%g -> %g)\n", dVal2, dVal1, dVal2 / dVal1);
			pushd(dVal2 / dVal1);
			IP = IP->next;
			break;
		case O_DIV_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("DIV_I\t(%d/%d -> %d)\n", iVal2, iVal1, iVal2 / iVal1);
			pushi(iVal2 / iVal1);
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
		case O_EQ_A:
			aVal1 = popa();
			aVal2 = popa();
			printf("EQ_A\t(%p==%p -> %d)\n", aVal2, aVal1, aVal2 == aVal1);
			pushi(aVal2 == aVal1);
			IP = IP->next;
			break;
		case O_EQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("EQ_C\t(%c==%c -> %d)\n", cVal2, cVal1, cVal2 == cVal1);
			pushi(cVal2 == cVal1);
			IP = IP->next;
			break;
		case O_EQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("EQ_D\t(%g==%g -> %d)\n", dVal2, dVal1, dVal2 == dVal1);
			pushi(dVal2 == dVal1);
			IP = IP->next;
			break;
		case O_EQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("EQ_I\t(%d==%d -> %d)\n", iVal2, iVal1, iVal2 == iVal1);
			pushi(iVal2 == iVal1);
			IP = IP->next;
			break;
		case O_GREATER_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("GREATER_C\t(%c>%c -> %d)\n", cVal2, cVal1, cVal2 > cVal1);
			pushi(cVal2 > cVal1);
			IP = IP->next;
			break;
		case O_GREATER_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("GREATER_D\t(%g>%g -> %d)\n", dVal2, dVal1, dVal2 > dVal1);
			pushi(dVal2 > dVal1);
			IP = IP->next;
			break;
		case O_GREATER_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("GREATER_I\t(%d>%d -> %d)\n", iVal2, iVal1, iVal2 > iVal1);
			pushi(iVal2 > iVal1);
			IP = IP->next;
			break;
		case O_GREATEREQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("GREATEREQ_C\t(%c>=%c -> %d)\n", cVal2, cVal1, cVal2 >= cVal1);
			pushi(cVal2 >= cVal1);
			IP = IP->next;
			break;
		case O_GREATEREQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("GREATEREQ_D\t(%g>=%g -> %d)\n", dVal2, dVal1, dVal2 >= dVal1);
			pushi(dVal2 >= dVal1);
			IP = IP->next;
			break;
		case O_GREATEREQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("GREATEREQ_I\t(%d>=%d -> %d)\n", iVal2, iVal1, iVal2 >= iVal1);
			pushi(iVal2 >= iVal1);
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
		case O_JF_A:
			aVal1 = popa();
			printf("JF_A\t%p\t(%p)\n", IP->args[0].addr, aVal1);
			IP = aVal1 ? IP->next : IP->args[0].addr;
			break;
		case O_JF_C:
			cVal1 = popc();
			printf("JF_C\t%p\t(%c)\n", IP->args[0].addr, cVal1);
			IP = cVal1 ? IP->next : IP->args[0].addr;
			break;
		case O_JF_D:
			dVal1 = popd();
			printf("JF_D\t%p\t(%g)\n", IP->args[0].addr, dVal1);
			IP = dVal1 ? IP->next : IP->args[0].addr;
			break;
		case O_JF_I:
			iVal1 = popi();
			printf("JF_I\t%p\t(%d)\n", IP->args[0].addr, iVal1);
			IP = iVal1 ? IP->next : IP->args[0].addr;
			break;
		case O_JMP:
			printf("JMP\t%p\n", IP->args[0].addr);
			IP = IP->args[0].addr;
			break;
		case O_JT_A:
			aVal1 = popa();
			printf("JT_A\t%p\t(%p)\n", IP->args[0].addr, aVal1);
			IP = aVal1 ? IP->args[0].addr : IP->next;
			break;
		case O_JT_C:
			cVal1 = popc();
			printf("JT_C\t%p\t(%c)\n", IP->args[0].addr, cVal1);
			IP = cVal1 ? IP->args[0].addr : IP->next;
			break;
		case O_JT_D:
			dVal1 = popd();
			printf("JT_D\t%p\t(%g)\n", IP->args[0].addr, dVal1);
			IP = dVal1 ? IP->args[0].addr : IP->next;
			break;
		case O_JT_I:
			iVal1 = popi();
			printf("JT_I\t%p\t(%d)\n", IP->args[0].addr, iVal1);
			IP = iVal1 ? IP->args[0].addr : IP->next;
			break;
		case O_LESS_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("LESS_C\t(%c<%c -> %d)\n", cVal2, cVal1, cVal2 < cVal1);
			pushi(cVal2 < cVal1);
			IP = IP->next;
			break;
		case O_LESS_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("LESS_D\t(%g<%g -> %d)\n", dVal2, dVal1, dVal2 < dVal1);
			pushi(dVal2 < dVal1);
			IP = IP->next;
			break;
		case O_LESS_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("LESS_I\t(%d<%d -> %d)\n", iVal2, iVal1, iVal2 < iVal1);
			pushi(iVal2 < iVal1);
			IP = IP->next;
			break;
		case O_LESSEQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("LESSEQ_C\t(%c<=%c -> %d)\n", cVal2, cVal1, cVal2 <= cVal1);
			pushi(cVal2 <= cVal1);
			IP = IP->next;
			break;
		case O_LESSEQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("LESSEQ_D\t(%g<=%g -> %d)\n", dVal2, dVal1, dVal2 <= dVal1);
			pushi(dVal2 <= dVal1);
			IP = IP->next;
			break;
		case O_LESSEQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("LESSEQ_I\t(%d<=%d -> %d)\n", iVal2, iVal1, iVal2 <= iVal1);
			pushi(iVal2 <= iVal1);
			IP = IP->next;
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
		case O_MUL_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("DIV_C\t(%c*%c -> %c)\n", cVal2, cVal1, cVal2 * cVal1);
			pushc(cVal2 * cVal1);
			IP = IP->next;
			break;
		case O_MUL_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("MUL_D\t(%g*%g -> %g)\n", dVal2, dVal1, dVal2 * dVal1);
			pushd(dVal2 * dVal1);
			IP = IP->next;
			break;
		case O_MUL_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("MUL_I\t(%d*%d -> %d)\n", iVal2, iVal1, iVal2 * iVal1);
			pushi(iVal2 * iVal1);
			IP = IP->next;
			break;
		case O_NEG_C:
			cVal1 = popc();
			printf("NOT_C\t(-%c)\n", cVal1);
			pushi(-cVal1);
			IP = IP->next;
			break;
		case O_NEG_D:
			dVal1 = popd();
			printf("NEG_D\t(-%g)\n", dVal1);
			pushd(-dVal1);
			IP = IP->next;
			break;
		case O_NEG_I:
			iVal1 = popi();
			printf("NEG_I\t(-%d)\n", iVal1);
			pushi(-iVal1);
			IP = IP->next;
			break;
		case O_NOP:
			printf("NOP\n");
			IP = IP->next;
			break;
		case O_NOT_A:
			aVal1 = popa();
			printf("NOT_A\t(!%p)\n", aVal1);
			pusha(!aVal1);
			IP = IP->next;
			break;
		case O_NOT_C:
			cVal1 = popc();
			printf("NOT_C\t(!%c)\n", cVal1);
			pushc(!cVal1);
			IP = IP->next;
			break;
		case O_NOT_D:
			dVal1 = popd();
			printf("NOT_D\t(!%g)\n", dVal1);
			pushi(!dVal1);
			IP = IP->next;
			break;
		case O_NOT_I:
			iVal1 = popi();
			printf("NOT_I\t(!%d)\n", iVal1);
			pushi(!iVal1);
			IP = IP->next;
			break;
		case O_NOTEQ_A:
			aVal1 = popa();
			aVal2 = popa();
			printf("NOTEQ_A\t(%p!=%p -> %d)\n", aVal2, aVal1, aVal2 != aVal1);
			pushi(aVal2 != aVal1);
			IP = IP->next;
			break;
		case O_NOTEQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("NOTEQ_C\t(%c!=%c -> %d)\n", cVal2, cVal1, cVal2 != cVal1);
			pushi(cVal2 != cVal1);
			IP = IP->next;
			break;
		case O_NOTEQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("NOTEQ_D\t(%g!=%g -> %d)\n", dVal2, dVal1, dVal2 != dVal1);
			pushi(dVal2 != dVal1);
			IP = IP->next;
			break;
		case O_NOTEQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("NOTEQ_I\t(%d!=%d -> %d)\n", iVal2, iVal1, iVal2 != iVal1);
			pushi(iVal2 != iVal1);
			IP = IP->next;
			break;
		case O_OFFSET:
			iVal1 = popi();
			aVal1 = popa();
			printf("OFFSET\t(%p+%d -> %p)\n", aVal1, iVal1, aVal1 + iVal1);
			pusha(aVal1 + iVal1);
			IP = IP->next;
			break;
		case O_OR_A:
			aVal1 = popa();
			aVal2 = popa();
			printf("OR_A\t(%p||%p -> %d)\n", aVal2, aVal1, aVal2 || aVal1);
			pushi(aVal2 || aVal1);
			IP = IP->next;
			break;
		case O_OR_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("OR_C\t(%c||%c -> %d)\n", cVal2, cVal1, cVal2 || cVal1);
			pushi(cVal2 || cVal1);
			IP = IP->next;
			break;
		case O_OR_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("OR_D\t(%g||%g -> %d)\n", dVal2, dVal1, dVal2 || dVal1);
			pushi(dVal2 || dVal1);
			IP = IP->next;
			break;
		case O_OR_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("OR_I\t(%d||%d -> %d)\n", iVal2, iVal1, iVal2 || iVal1);
			pushi(iVal2 || iVal1);
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
		case O_PUSHCT_C:
			cVal1 = IP->args[0].i;
			printf("PUSHCT_C\t%c\n", cVal1);
			pushc(cVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_D:
			dVal1 = IP->args[0].d;
			printf("PUSHCT_D\t%g\n", dVal1);
			pushd(dVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_I:
			iVal1 = IP->args[0].i;
			printf("PUSHCT_I\t%d\n", iVal1);
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
		case O_SUB_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("SUB_I\t(%c-%c -> %c)\n", cVal2, cVal1, cVal2 - cVal1);
			pushc(cVal2 - cVal1);
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
			printf("SUB_I\t(%d-%d -> %d)\n", iVal2, iVal1, iVal2 - iVal1);
			pushi(iVal2 - iVal1);
			IP = IP->next;
			break;
		default:
			err("invalid opcode: %d", IP->opcode);
		}
	}
}