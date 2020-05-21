#include "main.h"

/*
void mvTest()
{
	int v = 3; 
	do 
	{ 
		put_i(v); 
		v = v - 1; 
	} while (v); 
}
*/

void mvTest()	
{
	Instr* L1;
	int* v = allocGlobal(sizeof(int));
	addInstrA(O_PUSHCT_A, v);
	addInstrI(O_PUSHCT_I, 3);
	addInstrI(O_STORE, sizeof(int));
	L1 = addInstrA(O_PUSHCT_A, v);
	addInstrI(O_LOAD, sizeof(int));
	addInstrA(O_CALLEXT, requireSymbol(&symbols, "put_i")->addr);
	addInstrA(O_PUSHCT_A, v);
	addInstrA(O_PUSHCT_A, v);
	addInstrI(O_LOAD, sizeof(int));
	addInstrI(O_PUSHCT_I, 1);
	addInstr(O_SUB_I);
	addInstrI(O_STORE, sizeof(int));
	addInstrA(O_PUSHCT_A, v);
	addInstrI(O_LOAD, sizeof(int));
	addInstrA(O_JT_I, L1);		
	addInstr(O_HALT);
}
