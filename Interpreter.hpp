#pragma once
#include <vector>
#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <functional>
#include <algorithm>
#include <setjmp.h>
//#include "src/message/buffer.h"


#define STACK_OVERFLOW 1
#define STACK_UNDERFLOW 2
#define RSTACK_OVERFLOW 3
#define RSTACK_UNDERFLOW 4
#define CSTACK_OVERFLOW 5
#define CSTACK_UNDERFLOW 6
#define STACK_REQ_INT 7
#define STACK_REQ_DBL 8
#define STACK_REQ_ADR 9
#define STACK_WRONG_VALUE 10
#define INPUT_UNMATCH_PAREN 11
#define INPUT_UNMATCH_QUOT  21

#define WORD_NOT_FOUND 12
#define NOT_COLON_WORD 13
#define CFA_IS_NULL 14
#define CFA_NOT_FOUND 15
#define MEMORY_ALLOCATION_ERROR 16 
#define MEMORY_OVERFLOW 17
#define WORD_COMPILATION_ONLY 18
#define WORD_INTERPRETER_ONLY 19
#define WORD_MISSING_NAME 20

class Component;

typedef long long stack_int;
typedef double stack_dbl;

// Data & return stack cell
typedef struct {
	enum Type { INT = 1, DBL = 2, PTR = 4, STR = 8, ADR = 16 } type;
	union {
		stack_int int_val;
		stack_dbl dbl_val;
		uintptr_t ptr_val;
	};
} dst_cell_t;

// Memory cell
typedef unsigned char mem_cell_t;
// Pointer cell
typedef uintptr_t ptr_cell_t;

// The forth main memory (for dictionary).
#define MEMORY_SIZE 65536                
// Data stack (DS) size.
#define STACK_DEPTH 10           
// Control flow stack size
#define CFS_DEPTH 32    

#define F_IMMEDIATE 0x01
#define F_BUILTIN   0x02
#define F_VARIABLE  0x04
#define F_COLONWORD 0x08

struct state_t
{
	// Forth memory array
	mem_cell_t memory[MEMORY_SIZE];
	// Data Stack
	dst_cell_t* data_stack = (dst_cell_t*)memory;
	// Return Stack
	ptr_cell_t* return_stack = (ptr_cell_t*)(data_stack + STACK_DEPTH);
	// Control FLow Stack
	ptr_cell_t* control_flow_stack = (ptr_cell_t*)(return_stack + STACK_DEPTH);
	// Dictionary Memory
	mem_cell_t* dict_memory = (mem_cell_t*)(control_flow_stack + CFS_DEPTH);

	dst_cell_t* dsp = data_stack;
	ptr_cell_t* rsp = return_stack;
	ptr_cell_t* cfsp = control_flow_stack;

	// Pointers to manage the memory.
	mem_cell_t* rgx_here_ptr = dict_memory;
	mem_cell_t* rgx_last_nfa = NULL;
	mem_cell_t* rgx_next_nfa = rgx_here_ptr;

	// Execution pointer for colon defined words.
	ptr_cell_t* ip = NULL;

	// Input and output streams.
	FILE* input;
	FILE* output;

	bool compiling = false;
	char error_msg[256];

	unsigned char BASE = 10;

	Component* compObj;

};

void create_dictionary(state_t* state);
state_t* create_state();
void interpret(state_t* s);
void push(state_t* s, dst_cell_t value);
void execute_word(state_t* s, char* token);