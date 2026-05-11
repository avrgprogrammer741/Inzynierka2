#include <assert.h>
#include "Interpreter.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <cmath>

//#include "src/component/component.h"
//#include "src/hardware/hardware.h"

#define XT
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) > (b)) ? (b) : (a))


jmp_buf jump;
//Hardware hardware;

// Pointer to the execution routine (stored in CFA).
typedef void(*FuncPtr)(state_t*);

mem_cell_t* find_word(state_t* s, const char* word);
FuncPtr get_func(state_t* s, mem_cell_t* current);
size_t read_word(FILE* stream, char* outptr, size_t max_len);
size_t get_padding(mem_cell_t* current);
void create_builtin(state_t* s, const char* name, unsigned char flag, FuncPtr xt);
//size_t read_word_case_sensitive(FILE* stream, char* outptr, size_t max_len);

void execute_lit_primitive(state_t* s);
void execute_0branch_primitive(state_t* s);
void execute_branch_primitive(state_t* s);
void execute_loop_primitive(state_t* s);
void execute_exit_primitive(state_t* s);

void execute_add_primitive(state_t* s);
void execute_sub_primitive(state_t* s);
void execute_div_primitive(state_t* s);
void execute_mul_primitive(state_t* s);
void execute_dot_primitive(state_t* s);
void execute_cr_primitive(state_t* s);

void execute_drop_primitive(state_t* s);
void execute_swap_primitive(state_t* s);
void execute_rot_primitive(state_t* s);
void execute_mrot_primitive(state_t* s);
void execute_pick_primitive(state_t* s);
void execute_dup_primitive(state_t* s);

void execute_and_primitive(state_t* s);
void execute_or_primitive(state_t* s);
void execute_not_primitive(state_t* s);

void execute_store_primitive(state_t* s);
void execute_fetch_primitive(state_t* s);
void execute_Cstore_primitive(state_t* s);
void execute_Cfetch_primitive(state_t* s);

void execute_emit_primitive(state_t* s);
void execute_cmove_primitive(state_t* s);
void execute_allot_primitive(state_t* s);
void execute_I_primitive(state_t* s);
void execute_count_primitive(state_t* s);
void execute_type_primitive(state_t* s);
void execute_cells_primitive(state_t* s);
void execute_do_primitive(state_t* s);
void execute_quit_primitive(state_t* s);
void execute_backslash_primitive(state_t* s);
void execute_paren_primitive(state_t* s);
void execute_nip_primitive(state_t* s);
void execute_coma_primitive(state_t* s);
void execute_free_primitive(state_t* s);
void execute_allocate_primitive(state_t* s);
void execute_Squot_primitive(state_t* s);
void execute_hex_primitive(state_t* s);
void execute_dec_primitive(state_t* s);
void execute_oct_primitive(state_t* s);
void execute_dotS_primitive(state_t* s);
void execute_dotW_primitive(state_t* s);
void execute_dotV_primitive(state_t* s);
void execute_dotD_primitive(state_t* s);
void execute_here_primitive(state_t* s);
//void execute_serialize_primitive(state_t *s);
//void execute_eserialize_primitive(state_t *s);

//void execute_deserialize_primitive(state_t *s);
//void execute_edeserialize_primitive(state_t *s);

void execute_gt_primitive(state_t* s);
void execute_gtz_primitive(state_t* s);
void execute_eq_primitive(state_t* s);
void execute_lt_primitive(state_t* s);
void execute_ltz_primitive(state_t* s);
void execute_gte_primitive(state_t* s);
void execute_lte_primitive(state_t* s);
void execute_1plus_primitive(state_t* s);
void execute_1minus_primitive(state_t* s);
void execute_neq_primitive(state_t* s);
void execute_eqz_primitive(state_t* s);

void execute_immediate_primitive(state_t* s);
void execute_variable_primitive(state_t* s);
#ifdef XT
void execute_colon_primitive(state_t* s);
#endif
void execute_adc_primitive(state_t* s);
void execute_bin_primitive(state_t* s);
void execute_send_primitive(state_t* s);

void compile_do(state_t* s);
void compile_loop(state_t* s);
void compile_if(state_t* s);
void compile_else(state_t* s);
void compile_then(state_t* s);
void compile_begin(state_t* s);
void compile_while(state_t* s);
void compile_until(state_t* s);
void compile_variable(state_t* s);
void compile_colon(state_t* s);
void compile_create(state_t* s);
void compile_semicolon(state_t* s);


// Function creates forth state object.
state_t* create_state()
{
	state_t* s = (state_t*)malloc(sizeof(state_t));
	memset(s->memory, 0, MEMORY_SIZE);

	s->data_stack = (dst_cell_t*)s->memory;
	s->return_stack = (ptr_cell_t*)(s->data_stack + STACK_DEPTH);
	s->control_flow_stack = (ptr_cell_t*)(s->return_stack + STACK_DEPTH);
	s->dict_memory = (mem_cell_t*)(s->control_flow_stack + CFS_DEPTH);

	s->rgx_here_ptr = s->dict_memory;
	//s->rgx_next_nfa = s->rgx_last_nfa;
	s->rgx_last_nfa = NULL;

	s->dsp = s->data_stack;
	s->rsp = s->return_stack;
	s->cfsp = s->control_flow_stack;
	s->ip = NULL;
	s->compiling = false;

	s->output = stdout;

	s->BASE = 10;
	return s;
}

// Functions checks if the flag field contains given flag.
int check_flag(mem_cell_t* entry_addr, unsigned char flag) {
	return ((unsigned char)*(entry_addr + sizeof(ptr_cell_t)) & flag) != 0;
}

// Function reads the flag field.
unsigned char get_flag(mem_cell_t* current)
{
	return *(current + sizeof(ptr_cell_t));
}

// Function reads the length field.
unsigned char get_length(mem_cell_t* current)
{
	return *(current + sizeof(ptr_cell_t) + sizeof(mem_cell_t));
}

char* get_name_ptr(mem_cell_t* current)
{
	return (char*)(current + sizeof(ptr_cell_t) + 2 * sizeof(mem_cell_t));
}

// Function reads the name field.
void get_name(mem_cell_t* current, char name[])
{
	unsigned char lenght = get_length(current);
	memcpy(name, current + sizeof(ptr_cell_t) + 2 * sizeof(mem_cell_t), lenght);
	name[lenght] = '\0';
}

// Function reads the code field.
FuncPtr get_func(state_t* s, mem_cell_t* current)
{
	size_t name_len = get_length(current);
	current += sizeof(ptr_cell_t) + 2 * sizeof(mem_cell_t) + name_len;
	current += get_padding(current);

	FuncPtr func;
	memcpy(&func, current, sizeof(FuncPtr));
	return func;
}

// Function moves the nfa address to the pfa (code) address.
mem_cell_t* get_pfa_addr(mem_cell_t* addr)
{
	size_t name_len = get_length(addr);
	addr += sizeof(ptr_cell_t) + 2 * sizeof(mem_cell_t) + name_len;
	addr += get_padding(addr);
	return addr;
}

char* get_funct_name(FuncPtr func)
{
	if (func == execute_quit_primitive)
		return (char*)"QUIT";
	else if (func == execute_exit_primitive)
		return (char*)"EXIT";
	else if (func == execute_backslash_primitive)
		return (char*)"\\";
	else if (func == execute_paren_primitive)
		return (char*)"(";

	else if (func == execute_add_primitive)
		return (char*)"+";
	else if (func == execute_sub_primitive)
		return (char*)"-";
	else if (func == execute_div_primitive)
		return (char*)"/";
	else if (func == execute_mul_primitive)
		return (char*)"*";
	else if (func == execute_1plus_primitive)
		return (char*)"1+";
	else if (func == execute_1minus_primitive)
		return (char*)"1-";

	else if (func == execute_eq_primitive)
		return (char*)"=";
	else if (func == execute_gt_primitive)
		return (char*)">";
	else if (func == execute_lt_primitive)
		return (char*)"<";
	else if (func == execute_gte_primitive)
		return (char*)">=";
	else if (func == execute_lte_primitive)
		return (char*)"<=";
	else if (func == execute_neq_primitive)
		return (char*)"<>";
	else if (func == execute_eqz_primitive)
		return (char*)"0=";
	else if (func == execute_gtz_primitive)
		return (char*)"0>";
	else if (func == execute_ltz_primitive)
		return (char*)"0<";

	else if (func == execute_and_primitive)
		return (char*)"AND";
	else if (func == execute_or_primitive)
		return (char*)"OR";
	else if (func == execute_not_primitive)
		return (char*)"NOT";

	else if (func == execute_do_primitive)
		return (char*)"DO";
	else if (func == execute_loop_primitive)
		return (char*)"LOOP";
	else if (func == execute_I_primitive)
		return (char*)"I";

	else if (func == execute_0branch_primitive)
		return (char*)")BRANCH";
	else if (func == execute_branch_primitive)
		return (char*)"BRANCH";

	else if (func == execute_allot_primitive)
		return (char*)"ALLOT";

	else if (func == execute_dot_primitive)
		return (char*)".";
	else if (func == execute_pick_primitive)
		return (char*)"PICK";
	else if (func == execute_cr_primitive)
		return (char*)"CR";
	else if (func == execute_emit_primitive)
		return (char*)"EMIT";
	else if (func == execute_dup_primitive)
		return (char*)"DUP";
	else if (func == execute_drop_primitive)
		return (char*)"DROP";
	else if (func == execute_nip_primitive)
		return (char*)"NIP";
	else if (func == execute_swap_primitive)
		return (char*)"SWAP";
	else if (func == execute_rot_primitive)
		return (char*)"ROT";
	else if (func == execute_mrot_primitive)
		return (char*)"-ROT";

	else if (func == execute_fetch_primitive)
		return (char*)"@";
	else if (func == execute_store_primitive)
		return (char*)"!";
	else if (func == execute_Cfetch_primitive)
		return (char*)"C@";
	else if (func == execute_Cstore_primitive)
		return (char*)"C!";

	else if (func == execute_count_primitive)
		return (char*)"COUNT";
	else if (func == execute_type_primitive)
		return (char*)"TYPE";
	else if (func == execute_cmove_primitive)
		return (char*)"CMOVE";
	else if (func == execute_cells_primitive)
		return (char*)"CELLS";

	else if (func == execute_allocate_primitive)
		return (char*)"ALLOCATE";
	else if (func == execute_free_primitive)
		return (char*)"FREE";

	else if (func == execute_lit_primitive)
		return (char*)"LIT";
	else if (func == execute_store_primitive)
		return (char*)"!";
	else if (func == execute_fetch_primitive)
		return (char*)"@";
	else if (func == execute_Cstore_primitive)
		return (char*)"C!";
	else if (func == execute_Cfetch_primitive)
		return (char*)"C@";
	else if (func == execute_coma_primitive)
		return (char*)",";
	else if (func == execute_Squot_primitive)
		return (char*)"S\"";

	else if (func == execute_dec_primitive)
		return (char*)"DECIMAL";
	else if (func == execute_hex_primitive)
		return (char*)"HEX";
	else if (func == execute_oct_primitive)
		return (char*)"OCTAL";

	else if (func == execute_here_primitive)
		return (char*)"HERE";

	/*else if (func == execute_serialize_primitive)
		return (char*)">>";
	else if (func == execute_eserialize_primitive)
		return (char*)"e>>";
	else if (func == execute_deserialize_primitive)
		return (char*)"<<";
	else if (func == execute_edeserialize_primitive)
		return (char*)"e<<";*/

	else if (func == execute_adc_primitive)
		return (char*)"ADC";
	else if (func == execute_bin_primitive)
		return (char*)"BIN";
	else if (func == execute_send_primitive)
		return (char*)"SEND";

	else if (func == execute_immediate_primitive)
		return (char*)"IMMEDIATE";

	return (char*)"";
}

// Function stores pointer value in memory
void store_cell(state_t* s, ptr_cell_t value)
{
	*(ptr_cell_t*)(s->rgx_here_ptr) = value;
	s->rgx_here_ptr += sizeof(ptr_cell_t);
}

// Function stores byte value in memory
void store_cell(state_t* s, unsigned char value)
{
	*(s->rgx_here_ptr) = (mem_cell_t)value;
	s->rgx_here_ptr += sizeof(mem_cell_t);
}

// Function stores data cell value in memory
void store_cell(state_t* s, dst_cell_t value)
{
	*(dst_cell_t*)(s->rgx_here_ptr) = value;
	s->rgx_here_ptr += sizeof(dst_cell_t);
}

// Function stores function pointer in memory
void store_cell(state_t* s, FuncPtr value)
{
	*(FuncPtr*)(s->rgx_here_ptr) = value;
	s->rgx_here_ptr += sizeof(FuncPtr);
}

// Function creates buildin words in the dictionary.
void create_dictionary(state_t* state)
{
	create_builtin(state, "QUIT", F_BUILTIN | F_IMMEDIATE, execute_quit_primitive);
	create_builtin(state, "EXIT", F_BUILTIN, execute_exit_primitive);
	create_builtin(state, "\\", F_BUILTIN | F_IMMEDIATE, execute_backslash_primitive);
	create_builtin(state, "(", F_BUILTIN | F_IMMEDIATE, execute_paren_primitive);
	create_builtin(state, "+", F_BUILTIN, execute_add_primitive);
	create_builtin(state, "-", F_BUILTIN, execute_sub_primitive);
	create_builtin(state, "*", F_BUILTIN, execute_mul_primitive);
	create_builtin(state, "/", F_BUILTIN, execute_div_primitive);
	create_builtin(state, "1+", F_BUILTIN, execute_1plus_primitive);
	create_builtin(state, "1-", F_BUILTIN, execute_1minus_primitive);

	create_builtin(state, "=", F_BUILTIN, execute_eq_primitive);
	create_builtin(state, ">", F_BUILTIN, execute_gt_primitive);
	create_builtin(state, "<", F_BUILTIN, execute_lt_primitive);
	create_builtin(state, ">=", F_BUILTIN, execute_gte_primitive);
	create_builtin(state, "<=", F_BUILTIN, execute_lte_primitive);
	create_builtin(state, "<>", F_BUILTIN, execute_neq_primitive);
	create_builtin(state, "=0", F_BUILTIN, execute_eqz_primitive);
	create_builtin(state, ">0", F_BUILTIN, execute_gtz_primitive);
	create_builtin(state, "<0", F_BUILTIN, execute_ltz_primitive);

	create_builtin(state, "AND", F_BUILTIN, execute_and_primitive);
	create_builtin(state, "OR", F_BUILTIN, execute_or_primitive);
	create_builtin(state, "NOT", F_BUILTIN, execute_not_primitive);

	create_builtin(state, "_DO", F_BUILTIN, execute_do_primitive);
	create_builtin(state, "_LOOP", F_BUILTIN, execute_loop_primitive);
	create_builtin(state, "DO", F_BUILTIN | F_IMMEDIATE, compile_do);
	create_builtin(state, "LOOP", F_BUILTIN | F_IMMEDIATE, compile_loop);
	create_builtin(state, "I", F_BUILTIN, execute_I_primitive);

	create_builtin(state, "IF", F_BUILTIN | F_IMMEDIATE, compile_if);
	create_builtin(state, "ELSE", F_BUILTIN | F_IMMEDIATE, compile_else);
	create_builtin(state, "THEN", F_BUILTIN | F_IMMEDIATE, compile_then);
	create_builtin(state, "0BRANCH", F_BUILTIN, execute_0branch_primitive);
	create_builtin(state, "BRANCH", F_BUILTIN, execute_branch_primitive);

	create_builtin(state, "CREATE", F_BUILTIN, compile_create);
	create_builtin(state, "ALLOT", F_BUILTIN, execute_allot_primitive);
	create_builtin(state, "VARIABLE", F_BUILTIN, compile_variable);

	create_builtin(state, ".", F_BUILTIN, execute_dot_primitive);
	create_builtin(state, "PICK", F_BUILTIN, execute_pick_primitive);
	create_builtin(state, "CR", F_BUILTIN, execute_cr_primitive);
	create_builtin(state, "EMIT", F_BUILTIN, execute_emit_primitive);
	create_builtin(state, "DUP", F_BUILTIN, execute_dup_primitive);
	create_builtin(state, "DROP", F_BUILTIN, execute_drop_primitive);
	create_builtin(state, "NIP", F_BUILTIN, execute_nip_primitive);
	create_builtin(state, "SWAP", F_BUILTIN, execute_swap_primitive);
	create_builtin(state, "ROT", F_BUILTIN, execute_rot_primitive);
	create_builtin(state, "-ROT", F_BUILTIN, execute_mrot_primitive);
	create_builtin(state, "COUNT", F_BUILTIN, execute_count_primitive);
	create_builtin(state, "TYPE", F_BUILTIN, execute_type_primitive);
	create_builtin(state, "CMOVE", F_BUILTIN, execute_cmove_primitive);
	create_builtin(state, "CELLS", F_BUILTIN, execute_cells_primitive);

	create_builtin(state, "ALLOCATE", F_BUILTIN, execute_allocate_primitive);
	create_builtin(state, "FREE", F_BUILTIN, execute_free_primitive);

	create_builtin(state, "LIT", F_BUILTIN, execute_lit_primitive);
	create_builtin(state, "!", F_BUILTIN, execute_store_primitive);
	create_builtin(state, "@", F_BUILTIN, execute_fetch_primitive);
	create_builtin(state, "C!", F_BUILTIN, execute_Cstore_primitive);
	create_builtin(state, "C@", F_BUILTIN, execute_Cfetch_primitive);
	create_builtin(state, ",", F_BUILTIN, execute_coma_primitive);

	create_builtin(state, "S\"", F_BUILTIN | F_IMMEDIATE, execute_Squot_primitive);

	create_builtin(state, ":", F_BUILTIN, compile_colon);
	create_builtin(state, ";", F_BUILTIN | F_IMMEDIATE, compile_semicolon);
	create_builtin(state, "DECIMAL", F_BUILTIN, execute_dec_primitive);
	create_builtin(state, "HEX", F_BUILTIN, execute_hex_primitive);
	create_builtin(state, "OCTAL", F_BUILTIN, execute_oct_primitive);
	create_builtin(state, ".S", F_BUILTIN, execute_dotS_primitive);
	create_builtin(state, ".D", F_BUILTIN, execute_dotD_primitive);
	create_builtin(state, ".W", F_BUILTIN, execute_dotW_primitive);
	create_builtin(state, ".V", F_BUILTIN, execute_dotV_primitive);
	create_builtin(state, "HERE", F_BUILTIN, execute_here_primitive);
	//create_builtin(state, ">>", F_BUILTIN | F_IMMEDIATE, execute_serialize_primitive);
	//create_builtin(state, "e>>", F_BUILTIN, execute_eserialize_primitive);
	//create_builtin(state, "<<", F_BUILTIN | F_IMMEDIATE, execute_deserialize_primitive);
	//create_builtin(state, "e<<", F_BUILTIN, execute_edeserialize_primitive);

	create_builtin(state, "ADC", F_BUILTIN, execute_adc_primitive);
	create_builtin(state, "BIN", F_BUILTIN, execute_bin_primitive);
	create_builtin(state, "SEND", F_BUILTIN, execute_send_primitive);
}
/*
void serialize(state_t* s, char* name, BinaryBuffer& buffer)
{
	mem_cell_t* entry_addr = s->rgx_last_nfa;
	mem_cell_t* prev_entry = s->rgx_here_ptr;
	mem_cell_t* end_addr = s->rgx_here_ptr;
	mem_cell_t* start_addr = s->rgx_here_ptr;

	while (entry_addr != NULL)
	{
		if (!check_flag(entry_addr, F_VARIABLE | F_COLONWORD))
		{
			prev_entry = entry_addr;
			entry_addr = (mem_cell_t*)*(ptr_cell_t*)entry_addr;
			continue;
		}

		unsigned char flag = get_flag(entry_addr);
		unsigned char length = get_length(entry_addr);
		char name[256];
		get_name(entry_addr, name);

		if (!check_flag(entry_addr, F_BUILTIN))
		{
			start_addr = entry_addr;
			size_t start_pos = buffer.getSize();
			buffer.put("", (uint16_t)0);
			buffer.put("", (uint8_t)flag);
			buffer.put("", (uint8_t)length);
			buffer.put("", name);

			mem_cell_t* code_addr = get_pfa_addr(entry_addr);

			while (code_addr < prev_entry)
			{
				FuncPtr func = (FuncPtr)*(ptr_cell_t*)code_addr;
				uint16_t index = (uint16_t)((code_addr - s->dict_memory) / sizeof(ptr_cell_t));
				buffer.put("", index);

				if (func == execute_lit_primitive)
				{
					code_addr += sizeof(ptr_cell_t);
					dst_cell_t value = *(dst_cell_t*)code_addr;

					buffer.put("", (uint8_t)value.type);
					if (value.type == dst_cell_t::INT)
					{
						buffer.put("", (int)value.int_val);
					}
					else if (value.type == dst_cell_t::DBL)
					{
						buffer.put("", value.dbl_val);
					}
					else if (value.type == dst_cell_t::ADR)
					{
						buffer.put("", (uint16_t)(((mem_cell_t*)value.ptr_val - entry_addr) / sizeof(ptr_cell_t)));
					}
					else if (value.type == dst_cell_t::PTR)
					{
						buffer.put("", (uint16_t)0);
					}
					else if (value.type == dst_cell_t::STR)
					{
						buffer.put("", (char*)value.ptr_val);
					}

					code_addr += sizeof(dst_cell_t);
				}
				else if (func == execute_0branch_primitive)
				{
					code_addr += sizeof(ptr_cell_t);

					int64_t branch_offset = *(int64_t*)code_addr;
					buffer.put("", (int)(branch_offset / (int64_t)sizeof(ptr_cell_t)));

					code_addr += sizeof(ptr_cell_t);
				}
				else if (func == execute_branch_primitive)
				{
					code_addr += sizeof(ptr_cell_t);

					int64_t branch_offset = *(int64_t*)code_addr;
					buffer.put("", (int)(branch_offset / (int64_t)sizeof(ptr_cell_t)));

					code_addr += sizeof(ptr_cell_t);
				}
				else if (func == execute_loop_primitive)
				{
					code_addr += sizeof(ptr_cell_t);

					int64_t branch_offset = *(int64_t*)code_addr;

					buffer.put("", (int)(branch_offset / (int64_t)sizeof(ptr_cell_t)));

					code_addr += sizeof(ptr_cell_t);
				}
				else if (func == execute_variable_primitive)
				{
					code_addr += sizeof(ptr_cell_t);

					int64_t offset = *(int64_t*)code_addr;
					buffer.put("", (int)(offset / (int64_t)sizeof(ptr_cell_t)));

					code_addr += sizeof(ptr_cell_t);
				}
#ifdef XT
				else if (func == execute_colon_primitive)
				{
					code_addr += sizeof(ptr_cell_t);

					int64_t offset = *(int64_t*)code_addr;
					buffer.put("", (int)(offset / (int64_t)sizeof(ptr_cell_t)));

					code_addr += sizeof(ptr_cell_t);
				}
#endif
				else
				{
					//char * name = get_funct_name(func);
					//printf("    %s", name);

					code_addr += sizeof(ptr_cell_t);
				}
			}

			size_t end_pos = buffer.getSize();
			buffer.write(start_pos, "", (uint16_t)(end_pos - start_pos));
		}

		prev_entry = entry_addr;
		entry_addr = (mem_cell_t*)*(ptr_cell_t*)entry_addr;
	}

	printf("Number of bytes serialized: %lld\n", end_addr - start_addr);
}*/

/*
void deserialize(state_t* s, BinaryBuffer& buffer)
{
	while (buffer.dataAvailable())
	{
		uint16_t start_pos;
		uint16_t end_pos;
		uint8_t flags;
		uint8_t name_len;
		char* name;

		start_pos = (uint16_t)buffer.offset;
		buffer.get("", end_pos);
		end_pos = start_pos + end_pos;
		buffer.get("", flags);
		buffer.get("", name_len);
		buffer.get("", name);

		// Store the Link Field (LFA).
		// Remember current 'rgx_here_ptr' pointer. It is the NFA of the new word.
		mem_cell_t* word_addr = s->rgx_here_ptr;
		store_cell(s, (ptr_cell_t)s->rgx_last_nfa);
		s->rgx_last_nfa = word_addr;

		// Store the flag field.
		store_cell(s, (unsigned char)flags);

		// Store the name length.
		store_cell(s, (unsigned char)name_len);

		// Store word name.
		memcpy(s->rgx_here_ptr, name, name_len);
		s->rgx_here_ptr += name_len;

		// Align memory.
		size_t padding = get_padding(s->rgx_here_ptr);
		memset(s->rgx_here_ptr, 0, padding);
		s->rgx_here_ptr += padding;

		while (buffer.offset < end_pos)
		{

			uint16_t offset;
			mem_cell_t* code_addr;

			// Get code offset from the dictionary beginning
			buffer.get("", offset);

			// Convert offset to address
			code_addr = s->dict_memory + offset * sizeof(ptr_cell_t);
			FuncPtr func = (FuncPtr)*(ptr_cell_t*)code_addr;

#ifdef XT
			store_cell(s, (ptr_cell_t)func);
#else
			store_cell(s, (ptr_cell_t)word_addr);

#endif
			if (func == execute_lit_primitive)
			{
				dst_cell_t value;
				uint8_t type;
				buffer.get("", type);
				value.type = (dst_cell_t::Type)type;
				if (value.type == dst_cell_t::INT)
				{
					int v;
					buffer.get("", v);
					value.int_val = v;
				}
				else if (value.type == dst_cell_t::DBL)
				{
					double v;
					buffer.get("", v);
					value.dbl_val = v;
				}
				else if (value.type == dst_cell_t::ADR)
				{
					uint16_t v;
					buffer.get("", v);
					value.int_val = (ptr_cell_t)(word_addr + v * sizeof(ptr_cell_t));
				}
				else if (value.type == dst_cell_t::PTR)
				{
					uint16_t v;
					buffer.get("", v);
					value.int_val = 0;
				}
				else if (value.type == dst_cell_t::STR)
				{
					char* v;
					buffer.get("", v);
					value.ptr_val = (ptr_cell_t)v;
				}

				store_cell(s, value);
			}
			else if (func == execute_0branch_primitive)
			{
				int offset;
				buffer.get("", offset);
				store_cell(s, (ptr_cell_t)(offset * sizeof(ptr_cell_t)));
			}
			else if (func == execute_branch_primitive)
			{
				int offset;
				buffer.get("", offset);
				store_cell(s, (ptr_cell_t)(offset * sizeof(ptr_cell_t)));
			}
			else if (func == execute_loop_primitive)
			{
				int offset;
				buffer.get("", offset);
				store_cell(s, (ptr_cell_t)(offset * sizeof(ptr_cell_t)));
			}
			else if (func == execute_variable_primitive)
			{
				int offset;
				buffer.get("", offset);
				store_cell(s, (ptr_cell_t)(offset * sizeof(ptr_cell_t)));

			}
			else if (func == execute_colon_primitive)
			{
				int offset;
				buffer.get("", offset);
				store_cell(s, (ptr_cell_t)(offset * sizeof(ptr_cell_t)));
			}
		}
	}
}*/

// Function prints the content of the dictionary.
void debug_dump_dict(state_t* s, unsigned char flag)
{
	mem_cell_t* entry_addr = s->rgx_last_nfa;
	mem_cell_t* next_addr = s->rgx_here_ptr;

	// Dump all dictionary entries, followed the linked list of link addrs
	// in the header.
	while (entry_addr != NULL)
	{
		if (!check_flag(entry_addr, flag))
		{
			next_addr = entry_addr;
			entry_addr = (mem_cell_t*)*(ptr_cell_t*)entry_addr;
			continue;
		}

		char name[256];
		get_name(entry_addr, name);

		if (s->BASE == 16)
			printf("Entry at 0x%llx: name='%s'", (ptr_cell_t)entry_addr, name);
		else
			printf("Entry at %lld: name='%s'", (entry_addr - s->dict_memory) / sizeof(ptr_cell_t), name);

		if (check_flag(entry_addr, F_BUILTIN)) {
			printf(" (builtin)");
		}
		if (check_flag(entry_addr, F_IMMEDIATE)) {
			printf(" (immediate)");
		}
		printf("\n");

		if (!check_flag(entry_addr, F_BUILTIN))
		{
			mem_cell_t* code_addr = get_pfa_addr(entry_addr);

			while (code_addr < next_addr)
			{
				FuncPtr func = (FuncPtr) * (ptr_cell_t*)code_addr;

				if (s->BASE == 16)
					printf("  %04llx:    %04llx ", (ptr_cell_t)code_addr, (ptr_cell_t)func);
				else
					printf("  %lld:    %04llx ", (code_addr - s->dict_memory) / sizeof(ptr_cell_t), (ptr_cell_t)func);

				if (func == execute_lit_primitive)
				{
					// LIT is special, it has a number following it.
					printf("    %s", "LIT");

					code_addr += sizeof(ptr_cell_t);
					dst_cell_t value = *(dst_cell_t*)code_addr;

					if (value.type == dst_cell_t::INT)
						printf(" %lld", value.int_val);
					else if (value.type == dst_cell_t::DBL)
						printf(" %f", value.dbl_val);
					else if (value.type == dst_cell_t::ADR)
					{
						if (s->BASE == 16)
							printf(" 0x%llx", value.int_val);
						else
							printf(" %lld", ((mem_cell_t*)value.int_val - s->dict_memory) / sizeof(ptr_cell_t));
					}
					else if (value.type == dst_cell_t::PTR)
					{
						if (s->BASE == 16)
							printf(" 0x%llx", value.int_val);
						else
							printf(" %lld", value.int_val);
					}
					else if (value.type == dst_cell_t::STR)
						printf(" %s", (char*)value.ptr_val);

					code_addr += sizeof(dst_cell_t);
				}
				else if (func == execute_0branch_primitive)
				{
					// 0BRANCH is special, it has a offset following it.
					printf("    %s", "0BRANCH");

					code_addr += sizeof(ptr_cell_t);
					int64_t branch_offset = *(int64_t*)code_addr;
					if (s->BASE == 16)
						printf(" -> %lld (%04llx)", branch_offset, (ptr_cell_t)(code_addr + branch_offset));
					else
						printf(" -> %lld (%lld)", branch_offset / (int64_t)sizeof(ptr_cell_t), (code_addr + branch_offset - s->dict_memory) / sizeof(ptr_cell_t));

					code_addr += sizeof(ptr_cell_t);
				}
				else if (func == execute_branch_primitive)
				{
					// BRANCH is special, it has a offset following it.
					printf("    %s", "BRANCH");

					code_addr += sizeof(ptr_cell_t);
					int64_t branch_offset = *(int64_t*)code_addr;
					if (s->BASE == 16)
						printf(" -> %lld (%04llx)", branch_offset, (ptr_cell_t)(code_addr + branch_offset));
					else
						printf(" -> %lld (%lld)", branch_offset / (int64_t)sizeof(ptr_cell_t), (code_addr + branch_offset - s->dict_memory) / sizeof(ptr_cell_t));

					code_addr += sizeof(ptr_cell_t);
				}
				else if (func == execute_loop_primitive)
				{
					// LOOP is special, it has a offset following it.
					printf("    %s", "LOOP");

					code_addr += sizeof(ptr_cell_t);
					int64_t branch_offset = *(int64_t*)code_addr;
					if (s->BASE == 16)
						printf(" -> %lld (%04llx)", branch_offset, (ptr_cell_t)(code_addr + branch_offset));
					else
						printf(" -> %lld (%lld)", branch_offset / (int64_t)sizeof(ptr_cell_t), (code_addr + branch_offset - s->dict_memory) / sizeof(ptr_cell_t));

					code_addr += sizeof(ptr_cell_t);
				}
				else if (func == execute_variable_primitive)
				{
					//char * name = get_var_name(code_addr);
					printf("    %s", "VAR");
					code_addr += sizeof(ptr_cell_t);

					int64_t offset = *(int64_t*)code_addr;
					if (s->BASE == 16)
						printf(" -> %llx", offset);
					else
						printf(" -> %lld", offset / (int64_t)sizeof(ptr_cell_t));

					code_addr += sizeof(ptr_cell_t);
				}
#ifdef XT
				else if (func == execute_colon_primitive)
				{
					//char * name = get_var_name(code_addr);
					printf("    %s", "WORD");
					code_addr += sizeof(ptr_cell_t);

					int64_t offset = *(int64_t*)code_addr;

					if (s->BASE == 16)
						printf(" -> %llx", offset);
					else
						printf(" -> %lld", offset / (int64_t)sizeof(ptr_cell_t));

					code_addr += sizeof(ptr_cell_t);
				}
#endif
				else
				{
					char* name = get_funct_name(func);
					printf("    %s", name);

					code_addr += sizeof(ptr_cell_t);
				}

				printf("\n");
			}
		}

		next_addr = entry_addr;
		entry_addr = (mem_cell_t*)*(ptr_cell_t*)entry_addr;
	}
}

//*******************************************************************************
// Function for pushing a value onto the data stack.
void push(state_t* s, dst_cell_t value)
{
	if (s->dsp >= s->data_stack + STACK_DEPTH) {
		longjmp(jump, STACK_OVERFLOW);
		return;
	}
	*s->dsp++ = value;
}

// Function for taking a value from the data stack.
dst_cell_t pop(state_t* s)
{
	if (s->dsp <= s->data_stack) {
		longjmp(jump, STACK_UNDERFLOW);
	}
	return *--s->dsp;
}

// Function for reading a value from the data stack (without removing it).
dst_cell_t get(state_t* s, int depth = 0)
{
	if (s->dsp <= s->data_stack) {
		longjmp(jump, STACK_UNDERFLOW);
	}
	return *(s->dsp - depth - 1);
}

//*******************************************************************************
// Push function for CFS stack
void cfs_push(state_t* s, ptr_cell_t value)
{
	if (s->cfsp >= s->control_flow_stack + CFS_DEPTH) {
		longjmp(jump, CSTACK_OVERFLOW);
		return;
	}
	*s->cfsp++ = value;
}

// Pop function for CFS stack.
ptr_cell_t cfs_pop(state_t* s)
{
	if (s->cfsp <= s->control_flow_stack) {
		longjmp(jump, CSTACK_UNDERFLOW);
	}
	return *--s->cfsp;
}

//*******************************************************************************
// Push to return stack (used internally by Forth JIT/interpreter).
void r_push(state_t* s, ptr_cell_t value)
{
	if (s->rsp >= s->return_stack + STACK_DEPTH) {
		longjmp(jump, RSTACK_OVERFLOW);
	}

	*s->rsp++ = value;
}

// Pop from return stack.
ptr_cell_t r_pop(state_t* s)
{
	if (s->rsp <= s->return_stack) {
		longjmp(jump, RSTACK_UNDERFLOW);
	}
	return *--s->rsp;
}

// Function for reading a value from the return stack (without removing it).
ptr_cell_t r_get(state_t* s, int depth = 0)
{
	if (s->rsp <= s->return_stack) {
		longjmp(jump, RSTACK_UNDERFLOW);
	}
	return *(s->rsp - depth - 1);
}

//*******************************************************************************
// Function reads the input source till the end of the line.
// It implements comments inside the forth program.
void execute_backslash_primitive(state_t* s)
{
	int c;
	while ((c = fgetc(s->input)) != EOF && c != '\n') {
	}
}

// Function reads the input source till the closing paren ')'.
// It implements comments inside the forth program.
void execute_paren_primitive(state_t* s)
{
	int c;
	while ((c = fgetc(s->input)) != EOF && c != ')') {
	}
	if (c == EOF)
		longjmp(jump, INPUT_UNMATCH_PAREN);
}

// The execution function for handling add operation.
// Implementation of the '+' word: pop two items from stack, add them, push result.
// S: a b -- r
void execute_add_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val + b.int_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::DBL;
		r.dbl_val = av + bv;
	}
	else if (a.type == dst_cell_t::ADR && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::ADR;
		r.ptr_val = a.ptr_val + b.int_val;
	}
	else if (a.type == dst_cell_t::INT && b.type == dst_cell_t::ADR)
	{
		r.type = dst_cell_t::ADR;
		r.ptr_val = a.int_val + b.ptr_val;
	}
	else if (a.type == dst_cell_t::PTR && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::PTR;
		r.ptr_val = a.ptr_val + b.int_val;
	}
	else if (a.type == dst_cell_t::INT && b.type == dst_cell_t::PTR)
	{
		r.type = dst_cell_t::PTR;
		r.ptr_val = a.int_val + b.ptr_val;
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling substraction operation.
// Implementation of the '-' word: pop two items from stack, substract them, push result.
// S: a b -- r
void execute_sub_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val - b.int_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::DBL;
		r.dbl_val = av - bv;
	}
	else if (a.type == dst_cell_t::ADR && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::ADR;
		r.ptr_val = a.ptr_val - b.int_val;
	}
	else if (a.type == dst_cell_t::INT && b.type == dst_cell_t::ADR)
	{
		r.type = dst_cell_t::ADR;
		r.ptr_val = a.int_val - b.ptr_val;
	}
	else if (a.type == dst_cell_t::PTR && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::PTR;
		r.ptr_val = a.ptr_val - b.int_val;
	}
	else if (a.type == dst_cell_t::INT && b.type == dst_cell_t::PTR)
	{
		r.type = dst_cell_t::PTR;
		r.ptr_val = a.int_val - b.ptr_val;
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling multiplication operation.
// Implementation of the '*' word: pop two items from stack, multiple them, push result.
// S: a b -- r
void execute_mul_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val * b.int_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::DBL;
		r.dbl_val = av * bv;
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution primitive for handling division operation.
// Implementation of the '/' word: pop two items from stack, divide them, push result.
// S: a b -- r
void execute_div_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val / b.int_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::DBL;
		r.dbl_val = av / bv;
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling add 1 operation.
// Implementation of the '1+' word: pop item from stack, add one, push result.
// S: a -- a+1
void execute_1plus_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val + 1;
	}
	else if (a.type == dst_cell_t::DBL)
	{
		r.type = dst_cell_t::DBL;
		r.dbl_val = a.dbl_val + 1;
	}
	else if (a.type == dst_cell_t::ADR)
	{
		r.type = dst_cell_t::ADR;
		r.ptr_val = a.ptr_val + 1;
	}
	else if (a.type == dst_cell_t::PTR)
	{
		r.type = dst_cell_t::PTR;
		r.ptr_val = a.ptr_val + 1;
	}
	else if (a.type == dst_cell_t::STR)
	{
		r.type = dst_cell_t::STR;
		r.ptr_val = a.ptr_val + 1;
	}
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling minus 1 operation.
// Implementation of the '1-' word: pop item from stack, subtract one, push result.
// S: a -- a-1
void execute_1minus_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val - 1;
	}
	else if (a.type == dst_cell_t::DBL)
	{
		r.type = dst_cell_t::DBL;
		r.dbl_val = a.dbl_val - 1;
	}
	else if (a.type == dst_cell_t::ADR)
	{
		r.type = dst_cell_t::ADR;
		r.ptr_val = a.ptr_val - 1;
	}
	else if (a.type == dst_cell_t::PTR)
	{
		r.type = dst_cell_t::PTR;
		r.ptr_val = a.ptr_val - 1;
	}
	else if (a.type == dst_cell_t::STR)
	{
		r.type = dst_cell_t::STR;
		r.ptr_val = a.ptr_val - 1;
	}
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling equal operation.
// Implementation of the '=' word: pop two items from stack, compares them, push result.
// S: a b -- flag
void execute_eq_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.int_val = a.int_val == b.int_val;
	}
	else if (a.type == dst_cell_t::ADR && b.type == dst_cell_t::ADR)
	{
		r.int_val = a.ptr_val == b.ptr_val;
	}
	else if (a.type == dst_cell_t::PTR && b.type == dst_cell_t::PTR)
	{
		r.int_val = a.ptr_val == b.ptr_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.int_val = av == bv;
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling greater operation.
// Implementation of the '>' word: pop two items from stack, compares them, push result.
// S: a b -- flag
void execute_gt_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::INT;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.int_val = a.int_val > b.int_val;
	}
	else if (a.type == dst_cell_t::ADR && b.type == dst_cell_t::ADR)
	{
		r.int_val = a.ptr_val > b.ptr_val;
	}
	else if (a.type == dst_cell_t::PTR && b.type == dst_cell_t::PTR)
	{
		r.int_val = a.ptr_val > b.ptr_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.int_val = av > bv;
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling less operation.
// Implementation of the '<' word: pop two items from stack, compares them, push result.
// S: a b -- flag
void execute_lt_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val < b.int_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::INT;
		r.int_val = av < bv;
	}
	else if (a.type == dst_cell_t::ADR && b.type == dst_cell_t::ADR)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.ptr_val < b.ptr_val;
	}
	else if (a.type == dst_cell_t::PTR && b.type == dst_cell_t::PTR)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.ptr_val < b.ptr_val;
	}
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling less or equal operation.
// Implementation of the '<=' word: pop two items from stack, compares them, push result.
// S: a b -- flag
void execute_lte_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::INT;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.int_val = a.int_val <= b.int_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.int_val = av <= bv;
	}
	else if (a.type == dst_cell_t::ADR && b.type == dst_cell_t::ADR)
	{
		r.int_val = a.ptr_val <= b.ptr_val;
	}
	else if (a.type == dst_cell_t::PTR && b.type == dst_cell_t::PTR)
	{
		r.int_val = a.ptr_val <= b.ptr_val;
	}
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling greater-equal operation.
// Implementation of the '>=' word: pop two items from stack, compares them, push result.
// S: a b -- flag
void execute_gte_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val >= b.int_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::INT;
		r.int_val = av >= bv;
	}
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling not-equal operation.
// Implementation of the '<>' word: pop two items from stack, compares them, push result.
// S: a b -- flag
void execute_neq_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.int_val = a.int_val != b.int_val;
	}
	else if (a.type == dst_cell_t::ADR && b.type == dst_cell_t::ADR)
	{
		r.int_val = a.ptr_val != b.ptr_val;
	}
	else if (a.type == dst_cell_t::PTR && b.type == dst_cell_t::PTR)
	{
		r.int_val = a.ptr_val != b.ptr_val;
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.int_val = av != bv;
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling equal zero operation.
// Implementation of the '=0' word: pop item from stack, compares to zero, push result.
// S: a -- flag
void execute_eqz_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::INT;

	if (a.type == dst_cell_t::INT)
		r.int_val = a.int_val == 0;
	else if (a.type == dst_cell_t::DBL)
		r.int_val = a.dbl_val == 0;
	else if (a.type == dst_cell_t::ADR)
		r.int_val = a.ptr_val == 0;
	else if (a.type == dst_cell_t::PTR)
		r.int_val = a.ptr_val == 0;
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling greater then zero operation.
// Implementation of the '0>' word: pop item from stack, compares to zero, push result.
// S: a -- flag
void execute_gtz_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::INT;

	if (a.type == dst_cell_t::INT)
		r.int_val = a.int_val > 0;
	else if (a.type == dst_cell_t::DBL)
		r.int_val = a.dbl_val > 0;
	else if (a.type == dst_cell_t::ADR)
		r.int_val = a.ptr_val > 0;
	else if (a.type == dst_cell_t::PTR)
		r.int_val = a.ptr_val > 0;
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling less then zero operation.
// Implementation of the '0<' word: pop item from stack, compares to zero, push result.
// S: a -- flag
void execute_ltz_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::INT;

	if (a.type == dst_cell_t::INT)
		r.int_val = a.int_val < 0;
	else if (a.type == dst_cell_t::DBL)
		r.int_val = a.dbl_val < 0;
	else if (a.type == dst_cell_t::ADR)
		r.int_val = a.ptr_val < 0;
	else if (a.type == dst_cell_t::PTR)
		r.int_val = a.ptr_val < 0;
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling logical "and" operation.
// Implementation of the 'AND' word: pop two items from stack, make "AND" operation, push result.
// S: a b -- r
void execute_and_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;
	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val && b.int_val;
	}
	else
	{
		longjmp(jump, STACK_REQ_INT);
	}
	push(s, r);
}

// The execution function for handling logical "or" operation.
// Implementation of the 'OR' word: pop two items from stack, make OR operation, push result.
// S: a b -- r
void execute_or_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;
	if (a.type == dst_cell_t::INT || b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val || b.int_val;
	}
	else
	{
		longjmp(jump, STACK_REQ_INT);
	}
	push(s, r);
}

// The execution function for handling logical "not" operation.
// Implementation of the 'NOT' word: pop item from stack, negate it, push result.
// S: a -- r
void execute_not_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	if (a.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = !a.int_val;
	}
	else
	{
		longjmp(jump, STACK_REQ_INT);
	}
	push(s, r);
}

// The execution primitive that reads the loop index value and pushes it on data stack.
// R: i -- S: i
void execute_I_primitive(state_t* s)
{
	ptr_cell_t index = r_get(s); // read index
	dst_cell_t value;
	value.type = dst_cell_t::INT;
	value.int_val = index;
	push(s, value);
}

// The execution primitive that takes value from the stack and displays it.
// S: a -- 
void execute_dot_primitive(state_t* s)
{
	dst_cell_t value = pop(s);

	if (value.type == dst_cell_t::INT) {
		if (s->BASE == 10)
			printf("%lld", (long long)value.int_val);
		else if (s->BASE == 16)
			printf("Ox%llX", (long long)value.int_val);
		else
			printf("0o%llo", (long long)value.int_val);
	}
	else if (value.type == dst_cell_t::DBL)
		printf("%f", value.dbl_val);
	else if (value.type == dst_cell_t::STR)
		printf("%s", (char*)value.ptr_val);
	else {
		if (s->BASE == 10)
			printf("%lld", value.ptr_val);
		else if (s->BASE == 16)
			printf("0x%llX", value.ptr_val);
		else
			printf("0o%llo", value.ptr_val);
	}
}

// The execution function for removeing top item from the stack.
// S: a -- 
void execute_drop_primitive(state_t* s)
{
	pop(s);
}

// The execution function for removeing second top item from the stack.
// S: b a -- a
void execute_nip_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t b = pop(s);
	push(s, a);
}

// The execution function for swaping top two items on the stack.
// S: a b -- b a
void execute_swap_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	push(s, b);
	push(s, a);
}

// The execution function for rotation operation.
// S: c b a -- b a c
void execute_rot_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t b = pop(s);
	dst_cell_t c = pop(s);

	push(s, b);
	push(s, a);
	push(s, c);
}

// The execution function for reverse rotation operation.
// S: c b a -- a c b
void execute_mrot_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t b = pop(s);
	dst_cell_t c = pop(s);

	push(s, a);
	push(s, c);
	push(s, b);
}

// The execution primitive that takes value from the stack and displays it.
void execute_cr_primitive(state_t* s)
{
	printf("\n");
}

// The execution function for coping memory.
// It copes u bytes from source to destination. It is assumed that destination can hold at least u bytes.
// S: source dest u --
void execute_cmove_primitive(state_t* s)
{
	dst_cell_t len = pop(s);
	dst_cell_t dest = pop(s);
	dst_cell_t src = pop(s);

	memcpy((char*)dest.ptr_val, (char*)src.ptr_val, len.int_val);
}

// The execution function for calculating cells memory size.
// S: count -- size
void execute_cells_primitive(state_t* s)
{
	dst_cell_t count = pop(s);

	if (count.type == dst_cell_t::INT)
	{
		count.int_val = count.int_val * sizeof(dst_cell_t);
		push(s, count);
	}
	else
		longjmp(jump, STACK_REQ_INT);
}

// The execution function for seting the BASE variable to decimal.
void execute_dec_primitive(state_t* s)
{
	s->BASE = 10;
}

// The execution function for seting the BASE variable to hex.
void execute_hex_primitive(state_t* s)
{
	s->BASE = 16;
}

// The execution function for seting the BASE variable to octal.
void execute_oct_primitive(state_t* s)
{
	s->BASE = 8;
}

// The execution function for printing the stack content.
void execute_dotS_primitive(state_t* s)
{
	dst_cell_t item;
	dst_cell_t* tmp = s->data_stack;

	size_t count = s->dsp - s->data_stack;
	if (s->BASE == 10)
		printf("<%lld> ", count);
	else if (s->BASE == 16)
		printf("<0x%llX> ", count);
	else
		printf("<0o%llo> ", count);

	while (tmp < s->dsp) {
		item = *tmp++;
		if (item.type == dst_cell_t::INT)
		{
			if (s->BASE == 10)
				printf("%lld ", item.int_val);
			else if (s->BASE == 16)
				printf("0x%lld ", item.int_val);
			else
				printf("0o%lld ", item.int_val);

		}
		else if (item.type == dst_cell_t::DBL)
			printf("%f ", item.dbl_val);
		else if (item.type == dst_cell_t::ADR)
		{
			if (s->BASE == 10)
				printf("%lld ", item.ptr_val);
			else if (s->BASE == 16)
				printf("0x%llX ", item.ptr_val);
			else
				printf("0o%llo ", item.ptr_val);
		}
		else if (item.type == dst_cell_t::PTR)
		{
			if (s->BASE == 10)
				printf("%lld ", item.ptr_val);
			else if (s->BASE == 16)
				printf("0x%llX ", item.ptr_val);
			else
				printf("0o%llo ", item.ptr_val);
		}
		else if (item.type == dst_cell_t::STR)
			printf("%s ", (char*)item.ptr_val);
	}
	printf("\n");
}

// The execution function for printing the stack content.
void execute_dotD_primitive(state_t* s)
{
	debug_dump_dict(s, F_BUILTIN | F_IMMEDIATE | F_VARIABLE | F_COLONWORD);
}

// The execution function for printing the stack content.
void execute_dotW_primitive(state_t* s)
{
	debug_dump_dict(s, F_COLONWORD);
}

// The execution function for printing the stack content.
void execute_dotV_primitive(state_t* s)
{
	debug_dump_dict(s, F_VARIABLE);
}

// The execution function for serializing dictionary words.
// It can be executed during interpretation or comilation.
/*void execute_serialize_primitive(state_t *s)
{
	char token[256];
	size_t len = read_word(s->input, token, sizeof(token));
	if (len == 0) {
		// EOF reached
	}

	if (s->compiling)
	{
		char* buffer = (char*)malloc(len + 1);
		memcpy(buffer, token, len + 1);

#ifdef XT
		FuncPtr func = get_func(s, find_word(s, "e>>"));
		store_cell(s, (ptr_cell_t)func);
#else
		mem_cell_t* word_addr = find_word(s, "e>>");
		store_cell(s, (ptr_cell_t)word_addr);
#endif
		dst_cell_t addr;
		addr.type = dst_cell_t::STR;
		addr.ptr_val = (ptr_cell_t)buffer;
		store_cell(s, addr);
	}
	else
	{
		BinaryBuffer buffer(1000);
		serialize(s, (char*)"", buffer);

		FILE *fptr;
		fopen_s(&fptr, token, "wb");
		size_t written = fwrite(buffer.buffer.data(), sizeof(uint8_t), buffer.buffer.size(), fptr);
		fclose(fptr);

		printf("Compressed data size: %lld\n", buffer.getSize());
		for (size_t i = 0; i < buffer.getSize(); i++)
		{
			printf("%x", buffer.buffer[i]);
		}
		printf("\n");
	}
}*/

// The compiled execution function for serializing dictionary words.
// It can be executed only in compiled words.
/*void execute_eserialize_primitive(state_t *s)
{
	dst_cell_t filename = *(dst_cell_t*)(s->ip);
	if (filename.type == dst_cell_t::STR)
	{
		BinaryBuffer buffer(1000);
		serialize(s, (char*)"", buffer);

		FILE *fptr;
		fopen_s(&fptr, (char*)filename.ptr_val, "wb");
		size_t written = fwrite(buffer.buffer.data(), sizeof(uint8_t), buffer.buffer.size(), fptr);
		fclose(fptr);

		printf("Compressed data size: %lld\n", buffer.getSize());
		for (size_t i = 0; i < buffer.getSize(); i++)
		{
			printf("%x", buffer.buffer[i]);
		}
		printf("\n");

		s->ip = (ptr_cell_t*)((dst_cell_t*)s->ip + 1);
	}
}*/

// The execution function for deserializing dictionary words.
// It can be executed during interpretation or comilation.
/*void execute_deserialize_primitive(state_t *s)
{
	char token[256];
	size_t len = read_word(s->input, token, sizeof(token));
	if (len == 0) {
		// EOF reached
	}

	if (s->compiling)
	{
		char* buffer = (char*)malloc(len + 1);
		memcpy(buffer, token, len + 1);

#ifdef XT
		FuncPtr func = get_func(s, find_word(s, "e<<"));
		store_cell(s, (ptr_cell_t)func);
#else
		mem_cell_t* word_addr = find_word(s, "e>>");
		store_cell(s, (ptr_cell_t)word_addr);
#endif
		dst_cell_t addr;
		addr.type = dst_cell_t::STR;
		addr.ptr_val = (ptr_cell_t)buffer;
		store_cell(s, addr);
	}
	else
	{
		FILE *fptr;
		fopen_s(&fptr, token, "rb");

		fseek(fptr, 0, SEEK_END);
		long fsize = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);

		BinaryBuffer buffer(fsize);
		buffer.buffer.resize(fsize);

		size_t bytesRead = fread(buffer.buffer.data(), 1, fsize, fptr);

		fclose(fptr);

		deserialize(s, buffer);
	}
}*/

// The compiled execution function for deserializing dictionary words.
// It can be executed only in compiled words.
/*void execute_edeserialize_primitive(state_t *s)
{
	dst_cell_t filename = *(dst_cell_t*)(s->ip);
	if (filename.type == dst_cell_t::STR)
	{
		FILE *fptr;
		fopen_s(&fptr, (char*)filename.ptr_val, "rb");

		fseek(fptr, 0, SEEK_END);
		long fsize = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);

		BinaryBuffer buffer(fsize);

		fread(buffer.buffer.data(), 1, fsize, fptr);

		fclose(fptr);

		deserialize(s, buffer);
	}
}*/

// The execution function for putting on stack the address of free memory.
// S:  -- addr
void execute_here_primitive(state_t* s)
{
	dst_cell_t addr;
	addr.type = dst_cell_t::ADR;
	addr.ptr_val = (ptr_cell_t)s->rgx_here_ptr;
	push(s, addr);
}

// The execution function for quiting the interpreter. 
void execute_quit_primitive(state_t* s)
{
	exit(0);
}

// The execution primitive for handling DO operation.
// It starts the loop by placing the loop index and limit on the return stack.
// S: limit index -- R: limit index
void execute_do_primitive(state_t* s)
{
	dst_cell_t start_index = pop(s);  // Pop start from DS
	dst_cell_t limit = pop(s);        // Pop limit from DS

	r_push(s, limit.int_val);         // Push limit to RS
	r_push(s, start_index.int_val);   // Push start index to RS
}

// The execution primitive for handling LOOP operation.
// It executes the jump if the loop is not ended.
// R : limit index -- limit index+1
// R : limit index -- 
void execute_loop_primitive(state_t* s)
{
	assert(s->rsp - s->return_stack >= 2);

	stack_int index = *(s->rsp - 1) + 1;
	stack_int limit = *(s->rsp - 2);

	if (index > limit)
	{
		r_pop(s);
		r_pop(s);
		s->ip++;
	}
	else
	{
		*(s->rsp - 1) = index;
		int64_t jump_offset = *(int64_t*)s->ip;
		s->ip = (ptr_cell_t*)((mem_cell_t*)s->ip + jump_offset);
	}
}

// The execution primitive for handling conditional jump.
// It is used to impelment IF ELSE THEM statments.
// S: flag -- 
void execute_0branch_primitive(state_t* s)
{
	// Pop the flag (true or false) from the data stack.
	dst_cell_t flag = pop(s);

	if (flag.int_val == 0)
	{
		// Flag is true, read offset 
		int64_t jump_offset = *(int64_t*)s->ip;

		// Jump to 'ELSE' or 'THEN' statement(bypass code after 'IF')
		s->ip = (ptr_cell_t*)((mem_cell_t*)s->ip + jump_offset);
	}
	else
	{
		// Flag is true, execute code just after the 'IF' statement. 
		// (This will bypass code after 'ELSE')
		s->ip++;
	}
}

// The execution primitive for handling unconditional jump.
void execute_branch_primitive(state_t* s)
{
	// Read offset
	int64_t jump_offset = *(int64_t*)s->ip;

	// Jump to THEN (bypass code after 'ELSE')
	s->ip = (ptr_cell_t*)((mem_cell_t*)s->ip + jump_offset);
}

// The execution primitive for memory allocation in dictionary.
void execute_allot_primitive(state_t* s)
{
	dst_cell_t value = pop(s);
	s->rgx_here_ptr += value.int_val;

	size_t padding = get_padding(s->rgx_here_ptr);
	memset(s->rgx_here_ptr, 0, padding);
	s->rgx_here_ptr += padding;
}

// The execution primitive for storing value at given memory address.
void execute_store_primitive(state_t* s)
{
	// Read address value from stack (where to write data)
	dst_cell_t addr = pop(s);

	// Read value from stack.
	dst_cell_t value = pop(s);

	if (addr.type == dst_cell_t::ADR || addr.type == dst_cell_t::PTR)
	{
		// Extract address from address value 
		mem_cell_t* mem_addr = (mem_cell_t*)addr.ptr_val;

		// Store data at given address
		*(dst_cell_t*)mem_addr = value;
	}
	else
		longjmp(jump, STACK_REQ_ADR);
}

// The execution primitive for reading value from memory and storing it on data stack.
void execute_fetch_primitive(state_t* s)
{
	dst_cell_t addr = pop(s);

	if (addr.type == dst_cell_t::ADR || addr.type == dst_cell_t::PTR)
	{
		// Extract address from stack value.
		mem_cell_t* mem_addr = (mem_cell_t*)addr.ptr_val;

		// Get the variable value.
		dst_cell_t value;
		value = *(dst_cell_t*)mem_addr;

		push(s, value);
	}
	else
		longjmp(jump, STACK_REQ_ADR);
}

// The execution function for handling byte oriented storage operation.
// It writes one byte of data into the PFA memory.
void execute_Cstore_primitive(state_t* s)
{
	// Read address value from stack (where to write data)
	dst_cell_t addr = pop(s);
	// Read data
	dst_cell_t value = pop(s);

	if (addr.type == dst_cell_t::ADR || addr.type == dst_cell_t::PTR)
	{
		// Extract address from address value 
		mem_cell_t* mem_addr = (mem_cell_t*)addr.ptr_val;

		// Store data at given address
		*(char*)mem_addr = (char)value.int_val;
	}
	else
		longjmp(jump, STACK_REQ_ADR);
}

// The execution function for handling byte oriented fetch operation.
void execute_Cfetch_primitive(state_t* s)
{
	// Read value from stack.
	dst_cell_t addr = pop(s);

	if (addr.type == dst_cell_t::ADR || addr.type == dst_cell_t::PTR)
	{
		// Extract address from stack value.
		mem_cell_t* mem_addr = (mem_cell_t*)addr.ptr_val;

		// Read byte value at given address and store it on the stack.
		dst_cell_t value;
		value.type = dst_cell_t::INT;
		value.int_val = *mem_addr;
		push(s, value);
	}
	else
		longjmp(jump, STACK_REQ_ADR);
}

// The execution function for handling comma primitive.
// This function appends the value from stack to forth memory.
void execute_coma_primitive(state_t* s)
{
	dst_cell_t value = pop(s);
	store_cell(s, value);
}

// The execution function that reads an arbitrary element form stack and copies it to the top of the stack.
// Stack: v ... u -- v ... v 
void execute_pick_primitive(state_t* s)
{
	// Read the index (from top) of the element to copy
	dst_cell_t index = pop(s);

	// Read the emement
	dst_cell_t value = get(s, (int)index.int_val);

	// Put the copy of the element on the stack
	push(s, value);
}

// The execution function for handling EMIT operation.
// Implementation of the 'EMIT' word: pop the value from the stack and display it.
// Stack: a -- 
void execute_emit_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	if (a.type == dst_cell_t::INT)
		printf("%c", (int)a.int_val);
	else {
		longjmp(jump, STACK_REQ_INT);
	}
}

// The execution function for handling duplicate operation.
// Stack: a -- a a
void execute_dup_primitive(state_t* s)
{
	// Get the element without removing it from stack.
	dst_cell_t top = get(s);

	// Add copy to stack.
	push(s, top);
}

// The execution function for handling forth string literals (S").
// The forth strings have the following format: length (one byte) followed by ASCI characters.
// The function reads the string from current ip pointer (null terminated) and 
// stores the address and length of the string on the stack
// Stack:  -- addr u
void execute_Squot_primitive(state_t* s)
{
	char str[256];
	int c;
	size_t len = 0;

	// Skip leading whitespace.
	while ((c = fgetc(s->input)) != EOF && isspace(c)) {
	}

	// Read the until " or EOF.
	while (c != EOF && c != '\"' && len < 255) {
		str[len++] = (char)c;
		c = fgetc(s->input);
	}

	str[len] = '\0'; // Null-terminate the string.

	if (c == EOF)
		longjmp(jump, INPUT_UNMATCH_QUOT);

	char* buffer = (char*)malloc(len + 1);
	memcpy(buffer, str, len + 1);

	if (s->compiling)
	{
		// Compile the XT for the runtime 'LIT' handler
#ifdef XT
		FuncPtr func = get_func(s, find_word(s, "LIT"));
		store_cell(s, (ptr_cell_t)func);
#else
		mem_cell_t* word_addr = find_word(s, "LIT");
		store_cell(s, (ptr_cell_t)word_addr);
#endif

		dst_cell_t addr;
		addr.type = dst_cell_t::STR;
		addr.ptr_val = (ptr_cell_t)buffer;
		store_cell(s, addr);

#ifdef XT
		func = get_func(s, find_word(s, "LIT"));
		store_cell(s, (ptr_cell_t)func);
#else
		word_addr = find_word(s, "LIT");
		store_cell(s, (ptr_cell_t)word_addr);
#endif
		dst_cell_t length;
		length.type = dst_cell_t::INT;
		length.int_val = strlen(buffer);
		store_cell(s, length);
	}
	else
	{
		dst_cell_t addr;
		addr.type = dst_cell_t::STR;
		addr.ptr_val = (ptr_cell_t)buffer;
		push(s, addr);

		dst_cell_t length;
		length.type = dst_cell_t::INT;
		length.int_val = strlen(buffer);
		push(s, length);
	}
}

#ifdef XT
// The execution function for calling a defined word in compiled code.
void execute_colon_primitive(state_t* s)
{
	int64_t offset = (int64_t) * (s->ip);

	s->ip++;

	r_push(s, (ptr_cell_t)s->ip);

	s->ip = (ptr_cell_t*)(s->dict_memory + offset);
}
#endif

#ifdef XT
void execute_exit_primitive(state_t* s)
{
	s->ip = (ptr_cell_t*)r_pop(s);
}
#else
void execute_exit_primitive(state_t* s)
{
	s->ip = NULL;
}
#endif

// The execution function for calling a variable in compiled code.
void execute_variable_primitive(state_t* s)
{
	int64_t offset = (int64_t) * (s->ip);

	s->ip++;

	r_push(s, (ptr_cell_t)s->ip);

	s->ip = (ptr_cell_t*)(s->dict_memory + offset);
}

// Execution function for literal primitive.
void execute_lit_primitive(state_t* s)
{
	// The value is stored just after LIT primitive.
	dst_cell_t value = *(dst_cell_t*)(s->ip);
	push(s, value);

	s->ip = (ptr_cell_t*)((dst_cell_t*)s->ip + 1);
}

// The execution function that reads the length and the pointer of the forth string
// and puts them on the stack.
// Stack: addr -- addr+1 u
void execute_count_primitive(state_t* s)
{
	// Get the address of the PFA block that stores the string
	dst_cell_t addr = pop(s);

	if (addr.type == dst_cell_t::ADR)
	{
		mem_cell_t* ptr = (mem_cell_t*)addr.ptr_val;

		// Read the lemght of the string (from the first byte)
		unsigned char str_len = *ptr;

		// Advance pointer to the begining of the string literal
		ptr++;

		// Store the address on the stack
		addr.ptr_val = (ptr_cell_t)ptr;
		push(s, addr);

		// Store the lenght on the stack
		dst_cell_t len;
		len.type = dst_cell_t::INT;
		len.int_val = str_len;
		push(s, len);
	}
	else
		longjmp(jump, STACK_REQ_ADR);
}

// The execution function that handle the forth string printing operation.
// The length and address of the string are read from stack.
// It can be used with COUNT word to display string.
// Stack: addr u --
void execute_type_primitive(state_t* s)
{
	dst_cell_t len = pop(s);
	dst_cell_t addr = pop(s);

	fwrite((char*)addr.ptr_val, sizeof(char), len.int_val, stdout);
}

// The execution function for handling aboslut value operation.
// Stack: addr u -- |u|
void execute_abs_primitive(state_t* s)
{
	dst_cell_t a = pop(s);

	if (a.type == dst_cell_t::INT)
		a.int_val = abs(a.int_val);
	else if (a.type == dst_cell_t::DBL)
		a.dbl_val = abs(a.dbl_val);
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, a);
}

// The execution function for handling max operation.
// Implementation of the 'MAX' word: pop two items from stack, chose max, push result.
void execute_max_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = MAX(a.int_val, b.int_val);
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::DBL;
		r.dbl_val = MAX(av, bv);
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling min operation.
// Implementation of the 'MIN' word: pop two items from stack, chose min, push result.
void execute_min_primitive(state_t* s)
{
	dst_cell_t b = pop(s);
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT && b.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = MIN(a.int_val, b.int_val);
	}
	else if (((a.type & 3) != 0) && ((b.type & 3) != 0))
	{
		double av = a.type == dst_cell_t::INT ? a.int_val : a.dbl_val;
		double bv = b.type == dst_cell_t::INT ? b.int_val : b.dbl_val;

		r.type = dst_cell_t::DBL;
		r.dbl_val = MIN(av, bv);
	}
	else
	{
		longjmp(jump, STACK_WRONG_VALUE);
	}

	push(s, r);
}

// The execution function for handling sqrt operation.
// Implementation of the 'SQRT' word: pop item from stack, calculate sqrt, push result.
// Stack: a -- r
void execute_sqrt_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::DBL;

	if (a.type == dst_cell_t::INT)
		r.dbl_val = sqrt(a.int_val);
	else if (a.type == dst_cell_t::DBL)
		r.dbl_val = sqrt(a.dbl_val);
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling power operation.
// Implementation of the 'POW' word: pop item from stack, calculate power, push result.
// Stack: a -- r
void execute_pow_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = a.int_val * a.int_val;
	}
	else if (a.type == dst_cell_t::DBL)
	{
		r.type = dst_cell_t::DBL;
		r.dbl_val = a.dbl_val * a.dbl_val;
	}
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling exp operation.
// Implementation of the 'EXP' word: pop item from stack, calculate exp(a), push result.
// Stack: a -- r
void execute_exp_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::DBL;

	if (a.type == dst_cell_t::INT)
		r.dbl_val = exp(a.int_val);
	else if (a.type == dst_cell_t::DBL)
		r.dbl_val = exp(a.dbl_val);
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling exp operation.
// Implementation of the 'SIN' word: pop item from stack, calculate sin(a), push result.
// Stack: a -- r
void execute_sin_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::DBL;

	if (a.type == dst_cell_t::INT)
		r.dbl_val = sin(a.int_val);
	else if (a.type == dst_cell_t::DBL)
		r.dbl_val = sin(a.dbl_val);
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling cos operation.
// Implementation of the 'COS' word: pop item from stack, calculate cos(a), push result.
// Stack: a -- r
void execute_cos_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::DBL;

	if (a.type == dst_cell_t::INT)
		r.dbl_val = cos(a.int_val);
	else if (a.type == dst_cell_t::DBL)
		r.dbl_val = cos(a.dbl_val);
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function for handling log operation.
// Implementation of the 'LOG' word: pop item from stack, calculate log(a), push result.
// Stack: a -- r
void execute_log_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;
	r.type = dst_cell_t::DBL;

	if (a.type == dst_cell_t::INT)
		r.dbl_val = log(a.int_val);
	else if (a.type == dst_cell_t::DBL)
		r.dbl_val = log(a.dbl_val);
	else
		longjmp(jump, STACK_WRONG_VALUE);

	push(s, r);
}

// The execution function allocation of memory on the heap.
// It reads the amoun tof bytes to allocate from stack and after allocation puts
// the address of the allocated memory on the stack.
// S: u -- addr
void execute_allocate_primitive(state_t* s)
{
	dst_cell_t size = pop(s);

	char* buffer = (char*)malloc(size.int_val);
	if (buffer != NULL)
	{
		memset(buffer, 0, size.int_val);

		dst_cell_t addr;
		addr.type = dst_cell_t::PTR;
		addr.ptr_val = (ptr_cell_t)buffer;
		push(s, addr);
	}
	else
		longjmp(jump, MEMORY_ALLOCATION_ERROR);
}

// The execution function for freeing memory allocation on the C heap.
// Stack: addr -- 
void execute_free_primitive(state_t* s)
{
	dst_cell_t addr = pop(s);
	char* buffer = (char*)addr.ptr_val;
	free(buffer);
}

void execute_immediate_primitive(state_t* s)
{
	// latest points at the start of the word we're defining. Use it to find
	// the flag field and set the F_IMMEDIATE flag.
	mem_cell_t* flag_addr = s->rgx_last_nfa + sizeof(ptr_cell_t);
	*flag_addr |= F_IMMEDIATE;
}

// The execution function for reading analog inputs (ADC converter).
// S: a -- r
void execute_adc_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::DBL;
		r.dbl_val = 10.5;// hardware.readADC(a.int_val);
		push(s, r);
	}
	else
	{
		longjmp(jump, STACK_REQ_INT);
	}
}

// The execution function for reading analog inputs (ADC converter).
// S: a -- r
void execute_bin_primitive(state_t* s)
{
	dst_cell_t a = pop(s);
	dst_cell_t r;

	if (a.type == dst_cell_t::INT)
	{
		r.type = dst_cell_t::INT;
		r.int_val = 4;// hardware.readBIN(a.int_val);
		push(s, r);
	}
	else
	{
		longjmp(jump, STACK_REQ_INT);
	}
}

// The execution function for sending message.
// It read the number of parameters to process form the stack (count).
// S: ... message port c -- 
void execute_send_primitive(state_t* s)
{
	dst_cell_t count = pop(s);

	if (count.type == dst_cell_t::INT)
	{
		if (count.int_val > 0)
		{
			//BinaryBuffer buffer(1000);
			int64_t i = 0;

			// Read SEND parameters (.... message port --)

			// Read the pport and message names
			dst_cell_t port = pop(s);
			dst_cell_t message = pop(s);

			// Read content of the message and store it in the buffer
			while (i < count.int_val)
			{
				dst_cell_t value = pop(s);
				/*if (value.type == dst_cell_t::INT)
					buffer.put("", (int)value.int_val);
				else if (value.type == dst_cell_t::DBL)
					buffer.put("", value.dbl_val);
				else if (value.type == dst_cell_t::STR)
					buffer.put("", (char*)value.ptr_val);*/
				i++;
			}

			// Send message
			//s->compObj->sendMessage((char*)port.ptr_val, (char*)message.ptr_val, &buffer);
		}
	}
	else
	{
		longjmp(jump, STACK_REQ_INT);
	}
}

// Function aligns memory pointer 'rgx_here_ptr' to a cell boundary.
size_t get_padding(mem_cell_t* current) {
	size_t padding = (ptr_cell_t)current % sizeof(ptr_cell_t);
	padding = padding ? sizeof(ptr_cell_t) - padding : padding;
	return padding;
}

// Function to create a new dictionary entry (word definition).
void create_builtin(state_t* s, const char* name, unsigned char flag, FuncPtr xt)
{
	// Store the Link Field (LFA).
	// Remember current 'rgx_here_ptr' pointer. It is the NFA of the new word.
	mem_cell_t* current = s->rgx_here_ptr;
	store_cell(s, (ptr_cell_t)s->rgx_last_nfa);
	s->rgx_last_nfa = current;

	// Store the flag field.
	store_cell(s, (unsigned char)flag);

	// Store the name length.
	unsigned char name_len = (unsigned char)strlen(name);
	store_cell(s, name_len);

	// Store word name.
	memcpy(s->rgx_here_ptr, name, name_len);
	s->rgx_here_ptr += name_len;

	// Align memory.
	size_t padding = get_padding(s->rgx_here_ptr);
	memset(s->rgx_here_ptr, 0, padding);
	s->rgx_here_ptr += padding;

	// Store pointer to the function.
	store_cell(s, (ptr_cell_t)xt);

}

// Function creates word (variable) and allocates memory to it. 
//
// <LFA><F><L><name><LIT><addr><EXIT>.....
//                           |        ^
//                           \--------/
//
void compile_create(state_t* s)
{
	// Create a new dictionary entry for a word.
	char name[256];
	size_t len = read_word(s->input, name, sizeof(name));
	if (len == 0)
	{
		longjmp(jump, WORD_MISSING_NAME);
	}

	// Store the Link Field (LFA).
	// Remember current 'rgx_here_ptr' pointer. It is the NFA of the new word.
	mem_cell_t* current = s->rgx_here_ptr;
	store_cell(s, (ptr_cell_t)s->rgx_last_nfa);
	s->rgx_last_nfa = current;

	// Store the flag field.
	store_cell(s, (unsigned char)F_VARIABLE);

	// Store the name length.
	unsigned char name_len = (unsigned char)strlen(name);
	store_cell(s, name_len);

	// Store word name.
	memcpy(s->rgx_here_ptr, name, name_len);
	s->rgx_here_ptr += name_len;

	// Align memory.
	size_t padding = get_padding(s->rgx_here_ptr);
	memset(s->rgx_here_ptr, 0, padding);
	s->rgx_here_ptr += padding;

	// Compile the XT for the runtime 'LIT' handler
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "LIT"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "LIT");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	dst_cell_t value;
	value.type = dst_cell_t::ADR;
	value.ptr_val = (ptr_cell_t)(s->rgx_here_ptr + sizeof(dst_cell_t) + sizeof(ptr_cell_t));
	store_cell(s, value);

#ifdef XT
	func = get_func(s, find_word(s, "EXIT"));
	store_cell(s, (ptr_cell_t)func);
#else
	word_addr = find_word(s, "EXIT");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
}

// Function creates variable. 
// It adds variable word to dictionary:
//
// <LFA><F><L><name><LIT><addr><EXIT><value>
//                           |        ^
//                           \--------/
//
void compile_variable(state_t* s)
{
	if (s->compiling)
	{
		longjmp(jump, WORD_INTERPRETER_ONLY);
	}

	// Create a new dictionary entry.
	char name[256];
	size_t len = read_word(s->input, name, sizeof(name));
	if (len == 0)
	{
		longjmp(jump, WORD_MISSING_NAME);
	}

	// Store the Link Field (LFA).
	// Remember current 'rgx_here_ptr' pointer. It is the NFA of the new word.
	mem_cell_t* current = s->rgx_here_ptr;
	store_cell(s, (ptr_cell_t)s->rgx_last_nfa);
	s->rgx_last_nfa = current;

	// Store the flag field.
	store_cell(s, (unsigned char)F_VARIABLE);

	// Store the name length.
	unsigned char name_len = (unsigned char)strlen(name);
	store_cell(s, name_len);

	// Store word name.
	memcpy(s->rgx_here_ptr, name, name_len);
	s->rgx_here_ptr += name_len;

	// Align memory.
	size_t padding = get_padding(s->rgx_here_ptr);
	memset(s->rgx_here_ptr, 0, padding);
	s->rgx_here_ptr += padding;

	// Compile the XT for the runtime 'LIT' handler
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "LIT"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "LIT");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	// Compile the address of the variable value. We must add the size of the 
	dst_cell_t value;
	value.type = dst_cell_t::ADR;
	value.ptr_val = (ptr_cell_t)(s->rgx_here_ptr + sizeof(dst_cell_t) + sizeof(ptr_cell_t));
	store_cell(s, value);

#ifdef XT
	func = get_func(s, find_word(s, "EXIT"));
	store_cell(s, (ptr_cell_t)func);
#else
	word_addr = find_word(s, "EXIT");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	// Store the default value of the variable
	value;
	value.type = dst_cell_t::INT;
	value.int_val = 0;
	store_cell(s, value);
}

// The compilation of DO...LOOP:
//
//  _DOIMPL <loop word1> <loop word2> _LOOPIMPL <addr>
//              ^                                 |
//               \-------------------------------/
//                       (loop back-edge)
void compile_do(state_t* s)
{
	if (!s->compiling) {
		longjmp(jump, WORD_COMPILATION_ONLY);
	}

	// Compile the XT for the runtime 'DO' handler
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "_DO"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "_DO");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	// Push the address of begining of the loop on the CFS.
	cfs_push(s, (ptr_cell_t)s->rgx_here_ptr);
}

// Function comiples LOOP word.
void compile_loop(state_t* s)
{
	if (!s->compiling) {
		longjmp(jump, WORD_COMPILATION_ONLY);
	}

	// Compile the XT for the runtime 'LOOP' handler
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "_LOOP"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "_LOOP");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	// Pop the loop start address from the CFS (pushed by DO)
	ptr_cell_t addr = cfs_pop(s);

	// Calculate jump offset to the beginning of the loop
	int64_t offset = addr - (ptr_cell_t)s->rgx_here_ptr;
	store_cell(s, (ptr_cell_t)offset);
}

// The compilation of IF...ELSE...THEN:
//                                              IF true (bypass this)
//                                         /-------------------------\
//                                         |                         +
//  0BRANCH <addr> <word1> <word2> ELSE <addr> <word3> <word4> THEN <word5>
//             |                                ^
//              \-------------------------------/
//                       IF false (bypass this)
//
void compile_if(state_t* s)
{
	if (!s->compiling) {
		longjmp(jump, WORD_COMPILATION_ONLY);
	}

	// Compile the XT for the runtime 'IF' handler '0BRANCH' (conditional jump primitive)
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "0BRANCH"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "0BRANCH");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	// Push the address 'IF' on the CFS (IF placeholder).
	cfs_push(s, (ptr_cell_t)s->rgx_here_ptr);

	// Reserve space for the 'IF' jump offset. 
	store_cell(s, (ptr_cell_t)0);
}

void compile_else(state_t* s)
{
	if (!s->compiling) {
		longjmp(jump, WORD_COMPILATION_ONLY);
	}

	// Compile the XT for the runtime 'ELSE' handler 'BRANCH' (unconditional jump primitive)
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "BRANCH"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "BRANCH");
	store_cell(s, (ptr_cell_t)word_addr);
#endif

	// Pop the address of the 'IF' from CFS (pushed by compilation of 'IF')
	ptr_cell_t addr = cfs_pop(s);

	// Push the address of 'ELSE' on the CFS (ELSE placeholder).
	cfs_push(s, (ptr_cell_t)s->rgx_here_ptr);

	// Calculate jump offset from the IF to ELSE
	int64_t offset = s->rgx_here_ptr - (mem_cell_t*)addr + sizeof(ptr_cell_t);

	// Reserve space for the 'ELSE' jump offset. 
	store_cell(s, (ptr_cell_t)0);

	*(int64_t*)addr = offset; // Write the actual IF offset
}

void compile_then(state_t* s)
{
	// Pop the address of the 'IF/ELSE' from CFS (pushed by compilation of 'IF' or 'ELSE')
	ptr_cell_t addr = cfs_pop(s);

	// Calculate jump offset from the IF/ELSE to TEHN
	int16_t offset = (int16_t)(s->rgx_here_ptr - (mem_cell_t*)addr);

	// Write the actual offset
	*(int64_t*)addr = offset;
}

// The compilation of BEGIN ... UNTIL:
//
//  <word1> <>word2> <0branch> <addr> ....
//   ^                          |
//   \--------------------------/
//
void compile_begin(state_t* s)
{
	// Push the current 'rgx_here_ptr' address (the start of the loop) onto the Control Flow Stack.
	// We will use this to calculate offset that will move 'ip' poniter back to the begining of the loop.
	// The offset will be calculaed by REPEAT.
	cfs_push(s, (ptr_cell_t)s->rgx_here_ptr);
}

// The compilation function for UNTIL word.
void compile_until(state_t* s)
{
	// Pop the loop start address (placed there by BEGIN) from the Control Flow Stack.
	ptr_cell_t begin_loop_start_addr = cfs_pop(s);

	// The flag is on the stack at runtime. If the flag is TRUE (1), we loop back.
	// Compile the XT for the runtime 'UNTIL' handler '0BRANCH' (conditional jump primitive)
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "0BRANCH"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "0BRANCH");
	store_cell(s, (ptr_cell_t)word_addr);
#endif

	// Calculate and store the *relative* jump offset immediately after the 0BRANCH XT.
	// The offset needs to point from the *address of the offset field itself* back to the BEGIN address.
	int64_t offset = (mem_cell_t*)begin_loop_start_addr - s->rgx_here_ptr;
	store_cell(s, (ptr_cell_t)offset);
}

// The compilation of BEGIN ... WHILE ... REPEAT:
//
//  <word1> <>word2> <0branch> <addr> <word1> <>word2> <branch> ...
//                              |                               ^
//                              \-------------------------------/
//
// The compilation function for WHILE word.
void compile_while(state_t* s)
{
	// Compile the 0BRANCH CFA (Conditional Jump). Depending on the flag the 'ip' will be moved to 
	// the end of the loop or we will continue sequencially.
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "0BRANCH"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "0BRANCH");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	// Push the address of the placeholder onto the Control Flow Stack 
	// (will be used by REPEAT to update the jump offset value).
	cfs_push(s, (ptr_cell_t)s->rgx_here_ptr);

	// Reserve space for jump offset.
	// We store a placeholder 0 value for now (the actual value is calculeted by REPEAT).
	store_cell(s, (ptr_cell_t)0);
}


// The compilation function for REPEAT word.
void compile_repeat(state_t* s) {

	// 1. Pop the WHILE's patch address from the CFS
	ptr_cell_t while_patch_addr = cfs_pop(s);

	// 2. Pop the BEGIN's loop start address from the CFS
	ptr_cell_t begin_loop_start_addr = cfs_pop(s);

	// 3. Compile an unconditional BRANCH CFA. We have reach the REPEAT so we must jump back to the BEGIN.
	// This CFA will move 'ip' pointer back to the beging of the loop.	
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "BRANCH"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "BRANCH");
	store_cell(s, (ptr_cell_t)word_addr);
#endif

	// Store the jump offset immediately after the BRANCH CFA. The BRANCH CFA will use it to ecexute the jump.
	// Calculate offset: target address (begin_loop_start_addr) - current IP location (rgx_memory_ptr).
	// The offset will move 'ip' pointer to the beging of the loop.
	int64_t offset = (mem_cell_t*)begin_loop_start_addr - s->rgx_here_ptr;

	store_cell(s, (ptr_cell_t)offset);

	// PATCH the WHILE's 0BRANCH offset: calculate jump distance from 'while_patch_addr' to 'rgx_memory_ptr'
	// This jump skips the loop body if the WHILE flag is false. This offest is executed by the 0BRANCH CFA 
	// (compield by WHILE).
	offset = s->rgx_here_ptr - (mem_cell_t*)while_patch_addr;
	*(int64_t*)while_patch_addr = offset; // Write the actual offset
}

// Function creates colon word in the dictionary and starts the compilation process.
void compile_colon(state_t* s)
{
	char name[256];
	size_t name_len = read_word(s->input, name, sizeof(name));
	if (name_len == 0) {
		longjmp(jump, WORD_MISSING_NAME);
	}
	if (strcmp(name, ";") == 0) {
		// Empty definition: no op.
		return;
	}

	// Store the Link Field (LFA).
	// Remember current 'rgx_here_ptr' pointer. It is the NFA of the new word.
	mem_cell_t* current = s->rgx_here_ptr;
	store_cell(s, (ptr_cell_t)s->rgx_last_nfa);
	s->rgx_last_nfa = current;

	// Store the flag field.
	store_cell(s, (unsigned char)F_COLONWORD);

	// Store the name length.
	store_cell(s, (unsigned char)name_len);

	// Store word name.
	memcpy(s->rgx_here_ptr, name, name_len);
	s->rgx_here_ptr += name_len;

	// Align memory.
	size_t padding = get_padding(s->rgx_here_ptr);
	memset(s->rgx_here_ptr, 0, padding);
	s->rgx_here_ptr += padding;

#ifdef XT
	//	FuncPtr func = get_func(s, find_word(s, "_:"));
	//	store_cell(s, (ptr_cell_t)func);
#endif

	s->compiling = 1;
}

// Function ends colon word definition and stops the compilation process.
void compile_semicolon(state_t* s)
{
	// Compile the XT for the runtime 'EXIT' handler
#ifdef XT
	FuncPtr func = get_func(s, find_word(s, "EXIT"));
	store_cell(s, (ptr_cell_t)func);
#else
	mem_cell_t* word_addr = find_word(s, "EXIT");
	store_cell(s, (ptr_cell_t)word_addr);
#endif
	s->compiling = 0;
}

// Function reads word from the input stream in case sensitive mode.
size_t read_word(FILE* stream, char* outptr, size_t max_len)
{
	assert(max_len > 0);
	assert(outptr != NULL);
	size_t len = 0;
	int c;

	// Skip leading whitespace.
	while ((c = fgetc(stream)) != EOF && isspace(c)) {
	}

	// Read the word until whitespace or EOF.
	while (c != EOF && !isspace(c) && len < max_len - 1) {
		outptr[len++] = (char)c;
		c = fgetc(stream);
	}

	outptr[len] = '\0'; // Null-terminate the string.
	return len;
}

// Function checks if the literal is an integer number.
bool isInteger_(state_t* s, const char* item)
{
	if (item == NULL) {
		return false;
	}

	char* endptr;
	// strtol parses the string and sets endptr to the first invalid character.
	std::strtoll(item, &endptr, s->BASE);

	// If endptr points to the null terminator ('\0'), the entire string was validly parsed.
	if (*endptr == '\0') {
		// Additional checks for range can be added here if needed.
		return true;
	}

	return false;
}

// Function checks if the literal is an double number.
bool isDouble_(const char* item)
{
	if (item == NULL) {
		return false;
	}

	char* endptr;
	// Attempt to convert the string to a double.
	std::strtod(item, &endptr);

	// Check if the entire string was successfully converted (endptr points to the null terminator).
	if (*endptr == '\0') {
		return true;
	}

	return false;
}

// Function checks if the token is string
bool isString_(const char* item)
{
	if (item == NULL) {
		return false;
	}

	if (item[0] == '"' && item[strlen(item) - 1] == '"')
		return true;
	else
		return false;
}


// Function reads word from the input stream
size_t read_word_(FILE* stream, char* outptr, size_t max_len)
{
	// Read word
	size_t len = read_word(stream, outptr, max_len);
	if (!isString_(outptr))
	{
		// Convert the word to uppercase.
		for (size_t i = 0; i < len; ++i)
		{
			outptr[i] = toupper(outptr[i]);
		}
	}
	return len;
}

mem_cell_t* move_back_(mem_cell_t* nfa)
{
	ptr_cell_t previous_nfa;
	memcpy(&previous_nfa, nfa, sizeof(ptr_cell_t));
	return (mem_cell_t*)previous_nfa;
}

// Function finds word in the dictionary.
mem_cell_t* find_word(state_t* s, const char* word)
{
	mem_cell_t* current_nfa = s->rgx_last_nfa;

	while (current_nfa >= s->dict_memory)
	{
		unsigned char name_len = get_length(current_nfa);

		if (name_len == strlen(word) && strncmp(get_name_ptr(current_nfa), word, name_len) == 0) {
			// We found word
			return current_nfa;
		}

		current_nfa = move_back_(current_nfa);
	}

	return NULL;
}

// Function executes "toplevel" Forth word. word_addr is the address of the dictionary
// entry for the word to execute.
// "Toplevel" means this is not a word called from another word, but is read
// from the input stream (either as an immediate word while compiling, or
// as a toplevel interpreted word).
// This function handles nested invocation of words by itself.
static void execute_word(state_t* s, mem_cell_t* word_addr)
{
	//  Ckeck if the word is buildin?
	if (check_flag(word_addr, F_BUILTIN))
	{
		// This is buildin word, execurte its primitive.
		FuncPtr func = get_func(s, word_addr);
		func(s);
		return;
	}

	// The entry is not a builtin; set ip to the first word of its code and
	// start executing. Nested calls are handled by a combination of s->ip
	// and the return stack.
	s->ip = (ptr_cell_t*)get_pfa_addr(word_addr);
#ifdef XT
	r_push(s, (ptr_cell_t)NULL);
	//s->ip++;
#endif

	while (true)
	{
#ifdef XT

		FuncPtr xt = (FuncPtr) * (s->ip);
		s->ip++;

		xt(s);

		if (s->ip == NULL)
			break;
#else
		ptr_cell_t* subentry = s->ip;

		if (subentry == NULL)
		{
			if (s->rsp <= s->return_stack) {
				// No more nested words, resume execution of the toplevel code.
				break;
			}
			// The execution of nested word is finished. 
			// Resume execution of the upper level code (the word that called this word).
			s->ip = (ptr_cell_t*)r_pop(s);
		}
		else if (check_flag((mem_cell_t*)*subentry, F_BUILTIN))
		{
			// It's a builtin word, execute it.
			s->ip++;
			FuncPtr func = get_func(s, (mem_cell_t*)*subentry);
			func(s);
		}
		else
		{
			// This is colon word. Execute its code (execution of nested word).
			s->ip++;
			r_push(s, (ptr_cell_t)s->ip);
			s->ip = (ptr_cell_t*)get_pfa_addr((mem_cell_t*)*subentry);
		}
#endif
	}
}

void execute_word(state_t* s, char* token)
{
	// Try to find word for given token
	mem_cell_t* word_addr = find_word(s, token);

	if (word_addr != NULL)
	{
		// Word was found, check if we are in compilation mode?
		if (s->compiling && !check_flag(word_addr, F_IMMEDIATE))
		{
			// We are in compilation mode, store the word primitive in the PFA block.
#ifdef XT
			FuncPtr func = get_func(s, word_addr);

			if (check_flag(word_addr, F_COLONWORD))
			{
				// Word is colon defined, store its special primitive 
				// and offset to the word PFA block.
				store_cell(s, (ptr_cell_t)execute_colon_primitive);

				// The offset points to word data block
				mem_cell_t* pfa_addr = get_pfa_addr(word_addr);
				int64_t offset = pfa_addr - s->dict_memory;
				store_cell(s, (ptr_cell_t)offset);
			}
			else if (check_flag(word_addr, F_VARIABLE))
			{
				// Word is variable, store its special primitive 
				// and offset to the variable PFA block.
				store_cell(s, (ptr_cell_t)execute_variable_primitive);

				// The offset points to variable data block
				mem_cell_t* pfa_addr = get_pfa_addr(word_addr);
				int64_t offset = pfa_addr - s->dict_memory;
				store_cell(s, (ptr_cell_t)offset);
			}
			else
				store_cell(s, (ptr_cell_t)func);

#else
			store_cell(s, (ptr_cell_t)word_addr);
#endif
		}
		else
		{
			// We are not in compilation mode or word is immediate, execute the word directly.
			execute_word(s, word_addr);
		}
	}
	else
	{
		dst_cell_t value;

		// Check if token is an integer number
		if (isInteger_(s, token))
		{
			value.type = dst_cell_t::INT;
			value.int_val = std::strtoll(token, NULL, s->BASE);
		}
		else if (isDouble_(token)) // Check if token is a double number
		{
			value.type = dst_cell_t::DBL;
			value.dbl_val = std::strtod(token, NULL);
		}
		else if (isString_(token)) // Check if token is a string
		{
			char* buffer = (char*)malloc(strlen(token) - 1);
			memcpy(buffer, token + 1, strlen(token) - 2);
			buffer[strlen(token) - 2] = '\0';
			value.type = dst_cell_t::STR;
			value.ptr_val = (ptr_cell_t)buffer;
		}
		else {
			memcpy(s->error_msg, token, strlen(token) + 1);
			longjmp(jump, WORD_NOT_FOUND);
		}

		// Check if we are in compilation mode?
		if (s->compiling)
		{
			// We are in compilation mode
			// Store the literal primitive and value in the PFA block of word being compiled.
#ifdef XT
			FuncPtr func = get_func(s, find_word(s, "LIT"));
			store_cell(s, (ptr_cell_t)func);
			store_cell(s, value);
#else
			mem_cell_t* word_addr = find_word(s, "LIT");
			store_cell(s, (ptr_cell_t)word_addr);
			store_cell(s, value);
#endif
		}
		else //  Not in compilation mode, store value directly on stack.
			push(s, value);
	}
}

// Function interprets given input source until EOF.
void interpret(state_t* s)
{
	int error_code = setjmp(jump);

	if (error_code == 0)
	{
		while (true)
		{
			//Read token from input source
			char token[256];
			size_t len = read_word(s->input, token, sizeof(token));
			if (len == 0) {
				// EOF reached
				break;
			}

			// Try to find word for given token
			mem_cell_t* word_addr = find_word(s, token);

			if (word_addr != NULL)
			{
				// Word was found, check if we are in compilation mode?
				if (s->compiling && !check_flag(word_addr, F_IMMEDIATE))
				{
					// We are in compilation mode, store the word primitive in the PFA block.
#ifdef XT
					FuncPtr func = get_func(s, word_addr);

					if (check_flag(word_addr, F_COLONWORD))
					{
						// Word is colon defined, store its special primitive 
						// and offset to the word PFA block.
						store_cell(s, (ptr_cell_t)execute_colon_primitive);

						// The offset points to word data block
						mem_cell_t* pfa_addr = get_pfa_addr(word_addr);
						int64_t offset = pfa_addr - s->dict_memory;
						store_cell(s, (ptr_cell_t)offset);
					}
					else if (check_flag(word_addr, F_VARIABLE))
					{
						// Word is variable, store its special primitive 
						// and offset to the variable PFA block.
						store_cell(s, (ptr_cell_t)execute_variable_primitive);

						// The offset points to variable data block
						mem_cell_t* pfa_addr = get_pfa_addr(word_addr);
						int64_t offset = pfa_addr - s->dict_memory;
						store_cell(s, (ptr_cell_t)offset);
					}
					else
						store_cell(s, (ptr_cell_t)func);

#else
					store_cell(s, (ptr_cell_t)word_addr);
#endif
				}
				else
				{
					// We are not in compilation mode or word is immediate, execute the word directly.
					execute_word(s, word_addr);
				}
			}
			else
			{
				dst_cell_t value;

				// Check if token is an integer number
				if (isInteger_(s, token))
				{
					value.type = dst_cell_t::INT;
					value.int_val = std::strtoll(token, NULL, s->BASE);
				}
				else if (isDouble_(token)) // Check if token is a double number
				{
					value.type = dst_cell_t::DBL;
					value.dbl_val = std::strtod(token, NULL);
				}
				else if (isString_(token)) // Check if token is a string
				{
					char* buffer = (char*)malloc(strlen(token) - 1);
					memcpy(buffer, token + 1, strlen(token) - 2);
					buffer[strlen(token) - 2] = '\0';
					value.type = dst_cell_t::STR;
					value.ptr_val = (ptr_cell_t)buffer;
				}
				else {
					memcpy(s->error_msg, token, strlen(token) + 1);
					longjmp(jump, WORD_NOT_FOUND);
				}

				// Check if we are in compilation mode?
				if (s->compiling)
				{
					// We are in compilation mode
					// Store the literal primitive and value in the PFA block of word being compiled.
#ifdef XT
					FuncPtr func = get_func(s, find_word(s, "LIT"));
					store_cell(s, (ptr_cell_t)func);
					store_cell(s, value);
#else
					mem_cell_t* word_addr = find_word(s, "LIT");
					store_cell(s, (ptr_cell_t)word_addr);
					store_cell(s, value);
#endif
				}
				else //  Not in compilation mode, store value directly on stack.
					push(s, value);
			}
		}
	}
	else
	{
		if (error_code == STACK_OVERFLOW)
			printf("Error: Data stack overflow!\n");
		else if (error_code == STACK_UNDERFLOW)
			printf("Error: Data stack underflow!\n");
		else if (error_code == RSTACK_OVERFLOW)
			printf("Error: Return stack overflow!\n");
		else if (error_code == RSTACK_UNDERFLOW)
			printf("Error: Return stack underflow!\n");
		else if (error_code == CSTACK_OVERFLOW)
			printf("Error: Control flow stack overflow!\n");
		else if (error_code == CSTACK_UNDERFLOW)
			printf("Error: Control flow stack underflow!\n");

		else if (error_code == WORD_NOT_FOUND)
			printf("Error: Forth word \"%s\" not found!\n", s->error_msg);

		else if (error_code == CFA_IS_NULL)
			printf("Error: CFA null!\n");

		else if (error_code == STACK_REQ_INT)
			printf("Error: Integer value expected on the stack!\n");
		else if (error_code == STACK_REQ_DBL)
			printf("Error: Double value expected on the stack!\n");
		else if (error_code == STACK_REQ_ADR)
			printf("Error: Address value expected on the stack!\n");

		else if (error_code == STACK_WRONG_VALUE)
			printf("Error: Wrong stack value!\n");

		else if (error_code == MEMORY_ALLOCATION_ERROR)
			printf("Error: Memory allocation failed!\n");
		else if (error_code == MEMORY_OVERFLOW)
			printf("Error: Forth memory overflow!\n");
		else if (error_code == INPUT_UNMATCH_PAREN)
			printf("Error: Unmatched parenthesis in input!\n");
		else if (error_code == INPUT_UNMATCH_QUOT)
			printf("Error: Unmatched \" in input!\n");
		else if (error_code == WORD_COMPILATION_ONLY)
			printf("Error: Word can only be used in compiling mode!\n");
		else if (error_code == WORD_INTERPRETER_ONLY)
			printf("Error: Word can only be used in interpretation mode!\n");
		else if (error_code == WORD_MISSING_NAME)
			printf("Error: Missing word name!\n");

		else
			printf("Error: Unknown error!\n");

	}
}