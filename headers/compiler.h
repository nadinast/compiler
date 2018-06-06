#pragma once

//Structures for the lexical analyzer
enum token_codes {
	ID, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE,
	END, CT_INT, CT_REAL, CT_CHAR, CT_STRING, ASSIGN, ADD, SUB,
	MUL, DIV, DOT, AND, OR, NOT, EQUAL, NOTEQ, LESS, LESSEQ, GREATER,
	GREATEREQ, SEMICOLON, COMMA, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC
};

typedef struct _Token {
	int code;					 // code (name)
	union {
		char *text;				 // used for ID, CT_STRING (dynamically allocated)
		long int real_integer;   // used for CT_INT
		char character;			 //used for CT_CHAR
		double real_double;      // used for CT_REAL
	};
	struct _Token *next;		 // link to the next token
}Token;



//structures for symbols table and domain analysis
struct _Symbol;
typedef struct _Symbol Symbol;

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };
enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };
enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

typedef struct {
	Symbol **begin;			// the beginning of the symbols, or NULL
	Symbol **end;			// the position after the last symbol
	Symbol **after;			// the position after the allocated space
}Symbols;

typedef struct Type {
	int typeBase;			// TB_*
	Symbol *s;				// struct definition for TB_STRUCT
	int nElements;			// >0 array of given size, 0=array without size, <0 non array
}Type;

typedef struct _Symbol {
	const char *name;		// a reference to the name stored in a token
	int cls;				// CLS_*
	int mem;				// MEM_*
	Type type;
	int depth;				// 0-global, 1-in function, 2... - nested blocks in function
	union {
		Symbols args;		// used only of functions
		Symbols members;	// used only for structs
	};
	union {
		void *addr;			// vm: the memory address for global symbols
		int offset;			// vm: the stack offset for local symbols
	};
}Symbol;




//Functions and structures declarations for types analysis
void put_s();
void get_s();
void put_c();
void get_c();
void put_d();
void get_d();
void put_i();
void get_i();
void seconds();

typedef union {
	long int i;			// int, char
	double d;			// double
	const char *str;	// char[]
}CtVal;

typedef struct {
	Type type;			// type of the result
	int isLVal;			// if it is a LVal
	int isCtVal;		// if it is a constant value (int, real, char, char[])
	CtVal ctVal;		// the constat value
}RetVal;



//Structures for code generation
//_C - character
//_D - double
//_I - integer
//_A - address
enum {
	O_ADD_C, O_ADD_D, O_ADD_I, O_AND_A,
	O_AND_C, O_AND_D, O_AND_I,
	O_CALL, O_CALLEXT,
	O_CAST_I_C, O_CAST_I_D,
	O_CAST_C_D, O_CAST_C_I,
	O_CAST_D_C, O_CAST_D_I,
	O_DIV_C, O_DIV_D, O_DIV_I,
	O_DROP, O_ENTER,
	O_EQ_C, O_EQ_D, O_EQ_I, O_EQ_A,
	O_GREATER_C, O_GREATER_D, O_GREATER_I,
	O_GREATEREQ_C, O_GREATEREQ_D, O_GREATEREQ_I,
	O_HALT, O_INSERT,
	O_JF_C, O_JF_D, O_JF_I, O_JF_A,
	O_JMP,
	O_JT_C, O_JT_D, O_JT_I, O_JT_A,
	O_LESS_C, O_LESS_D, O_LESS_I,
	O_LESSEQ_C, O_LESSEQ_D, O_LESSEQ_I,
	O_LOAD,
	O_MUL_C, O_MUL_D, O_MUL_I,
	O_NEG_C, O_NEG_D, O_NEG_I,
	O_NOP,
	O_NOT_C, O_NOT_D, O_NOT_I, O_NOT_A,
	O_NOTEQ_C, O_NOTEQ_D, O_NOTEQ_I, O_NOTEQ_A,
	O_OFFSET,
	O_OR_C, O_OR_D, O_OR_I, O_OR_A,
	O_PUSHFPADDR,
	O_PUSHCT_C, O_PUSHCT_D, O_PUSHCT_I, O_PUSHCT_A,
	O_RET, O_STORE,
	O_SUB_C, O_SUB_D, O_SUB_I
};

typedef struct _Instr {
	int opcode;					// O_*
	union {
		long int i;				// int, char
		double d;
		void *addr;
	}args[2];
	struct _Instr *last, *next;	// links to last, next instructions
}Instr;



//Functions for syntax analyzer

//Syntax analyzer main funtion definition
int unit();

int decl_struct();
int decl_var();
int array_decl(Type *ret);
int typebase(Type *ret);
int type_name(Type *ret);
int decl_func();
int func_arg();
int stm();
int stm_compound();
int expr(RetVal *rv);
int expr_assign(RetVal *rv);
int expr_or(RetVal *rv);
int expr_and(RetVal *rv);
int expr_eq(RetVal *rv);
int expr_rel(RetVal *rv);
int expr_add(RetVal *rv);
int expr_mul(RetVal *rv);
int expr_cast(RetVal *rv);
int expr_unary(RetVal *rv);
int expr_postfix(RetVal *rv);
int expr_primary(RetVal *rv);
int rule_if(RetVal *rv, Instr  *i1, Instr *i2);
int rule_while(RetVal *rv, Instr *i1, Instr *i2);
int rule_for(Instr *i2, Instr* i3, Instr* i4, Instr *is, Instr *ib3, Instr *ibs);
int rule_break();
int rule_return(RetVal *rv, Instr* i);




//Functions for code generation
int typeArgSize(Type *type);
Instr *addInstrI(int opcode, long int val);
int typeBaseSize(Type *type);
int typeFullSize(Type *type);
void *allocGlobal(int size);