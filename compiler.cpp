#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<ctype.h>
#include<string.h>

#pragma warning(disable : 4996)
#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

#define STACK_SIZE (32*1024)
char stack[STACK_SIZE];
char *SP;			// Stack Pointer
char *stackAfter;	// first byte after stack; used for stack limit tests

#define GLOBAL_SIZE (32*1024)
char globals[GLOBAL_SIZE];
int nGlobals;


//Syntax analyzer main funtion definition
int unit();

//global vars for code generation part
int sizeArgs;
int offset;

//LEXICAL ANALYZER BEGIN
//------------------------------

//token codes
enum token_codes{
	ID, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE,
	END, CT_INT, CT_REAL, CT_CHAR, CT_STRING, ASSIGN, ADD, SUB,
	MUL, DIV, DOT, AND, OR, NOT, EQUAL, NOTEQ, LESS, LESSEQ, GREATER,
	GREATEREQ, SEMICOLON, COMMA, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC
};

typedef struct _Token {
	int code;       // code (name)
	union {
		char *text;  // used for ID, CT_STRING (dynamically allocated)
		long int real_integer;  // used for CT_INT
		char character; //used for CT_CHAR
		double real_double;    // used for CT_REAL
	};
	struct _Token *next; // link to the next token
}Token;

Token *tokens;
Token *lastToken;

void err(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
	exit(-1);
}

void tkerr(const char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	fputc('\n', stderr);
	va_end(va);
	exit(-1);
}

int get_integer(char *value) {
	int number;
	sscanf(value, "%d", &number);
	return number;
}

double get_double(char *value) {
	double fractional_part = 0;
	double integer_part = 0;
	double power = 0.1;
	for (int i = 0; value[i]; i++)
		if (value[i] == '.')
			for (int j = i; value[j]; j++) {
				fractional_part = fractional_part + (value[j] - '0') * power;
				power = power * 0.1;
				i++;
			}
		else
			integer_part = integer_part * 10 + value[i] - '0';
	return integer_part + fractional_part;
}

Token *addTk(int code, char* value) {
	Token *tk;
	SAFEALLOC(tk, Token)
		tk->code = code;
	if (code == ID || code == CT_STRING) {
		tk->text = (char *)malloc(strlen(value) * sizeof(char));
		strcpy(tk->text, value);
	}
	else {
		if (code == CT_INT)
			tk->real_integer = get_integer(value);
		else if (code == CT_REAL)
			tk->real_double = get_double(value);
		else if (code == CT_CHAR)
			tk->character = value[0];
	}
	tk->next = NULL;
	if (lastToken) {
		lastToken->next = tk;
		//printf("%d %d\n", lastToken->code, tk->code);
	}
	else {
		tokens = tk;
		//printf("%d\n", tokens->code);

	}
	lastToken = tk;
	return tk;
}

int iskeyword(char *tk_name) {
	if (strcmp(tk_name, "break") == 0)
		return BREAK;
	if (strcmp(tk_name, "char") == 0)
		return CHAR;
	if (strcmp(tk_name, "double") == 0)
		return DOUBLE;
	if (strcmp(tk_name, "else") == 0)
		return ELSE;
	if (strcmp(tk_name, "for") == 0)
		return FOR;
	if (strcmp(tk_name, "if") == 0)
		return IF;
	if (strcmp(tk_name, "int") == 0)
		return INT;
	if (strcmp(tk_name, "return") == 0)
		return RETURN;
	if (strcmp(tk_name, "struct") == 0)
		return STRUCT;
	if (strcmp(tk_name, "void") == 0)
		return VOID;
	if (strcmp(tk_name, "while") == 0)
		return WHILE;
	return 0;
}

void consume_whitespaces(FILE* fp, int  first_char) {
	int c = first_char;
	while (isspace(c))
		c = fgetc(fp);
	ungetc(c, fp);
}

void consume_multi_line_comments(FILE* fp) {
	//COMMENT: '/*' ( [^*] | '*'+ [^*/] )* '*'+ '/'  ;
	int c = fgetc(fp);
	while ((c = fgetc(fp)) != '*' || (c = fgetc(fp)) != '/');
}

void consume_comments(FILE* fp, int first_char) {
	int c = first_char;
	if (c != EOF) {
		if (c == '/') {
			c = fgetc(fp);
			if (c == '/')
				do {
					c = fgetc(fp);
				} while (c != '\n' && c != '\r' && c != '\0' && c != EOF);
				if (c == EOF) {
					printf("hello\n");
					if (unit()) {  //go to syntax alaysis
						printf("Lexical and syntactical analysis OK!\n");
						exit(0);
					}
				}
			else
				if (c == '*')
					consume_multi_line_comments(fp);
				else
					ungetc(c, fp);
		}
		else
			ungetc(c, fp);
	}
}
int getNextToken(FILE * fp, int first_char) {
	consume_whitespaces(fp, first_char);
	int state = 0;
	char ch = fgetc(fp);;
	char *token_name = (char *)calloc(100, sizeof(char));
	int cnt = 0;
	if (ch == EOF) {
		printf("Hello\n");
		if (unit()) {  //go to syntax alaysis
			printf("Lexical and syntactical analysis OK!\n");
			exit(0);
		}
	}
	while (1) {
		switch (state) {
			//no characters received yet -> initial state
		case 0:
			if (isalpha(ch)) {
				strcpy(token_name, &ch);
				cnt++;
				state = 1;
			}
			else if (ch == '=') {
				if ((ch = fgetc(fp)) == '=') {
					addTk(EQUAL, NULL);
					return EQUAL;
				}
				ungetc(ch, fp);
				addTk(ASSIGN, NULL);
				return ASSIGN;
			}
			else if (ch == '+') {
				addTk(ADD, NULL);
				return ADD;
			}
			else if (ch == '-') {
				addTk(SUB, NULL);
				return SUB;
			}
			else if (ch == '*') {
				addTk(MUL, NULL);
				return MUL;
			}
			else if (ch == '/') {
				ch = fgetc(fp);
				if (ch != '*' && ch != '/'){
					addTk(DIV, NULL);
					return DIV;
				}
				else {
					ungetc(ch, fp);
					consume_comments(fp, '/');
					return -1;
				}
			}
			else if (ch == '.') {
				addTk(DOT, NULL);
				return DOT;
			}
			else if (ch == '!') {
				if ((ch = fgetc(fp)) == '=') {
					addTk(NOTEQ, NULL);
					return NOTEQ;
				}
				ungetc(ch, fp);
				addTk(NOT, NULL);
				return NOT;
			}
			else if (ch == '<') {
				if ((ch = fgetc(fp)) == '=') {
					addTk(LESSEQ, NULL);
					return LESSEQ;
				}
				ungetc(ch, fp);
				addTk(LESS, NULL);
				return LESS;
			}
			else if (ch == '>') {
				if ((ch = fgetc(fp)) == '=') {
					addTk(GREATEREQ, NULL);
					return GREATEREQ;
				}
				ungetc(ch, fp);
				addTk(GREATER, NULL);
				return GREATER;
			}
			else if (ch == '&') {
				if ((ch = fgetc(fp)) == '&') {
					addTk(AND, NULL);
					return AND;
				}
				tkerr("Invalid character. \'&\' expected");
			}
			else if (ch == '|') {
				if ((ch = fgetc(fp)) == '|') {
					addTk(OR, NULL);
					return OR;
				}
				tkerr("Invalid character. \'|\' expected");
			}
			else if (ch == ',') {
				addTk(COMMA, NULL);
				return COMMA;
			}
			else if (ch == ';') {
				addTk(SEMICOLON, NULL);
				return SEMICOLON;
			}
			else if (ch == '(') {
				addTk(LPAR, NULL);
				return LPAR;
			}
			else if (ch == ')') {
				addTk(RPAR, NULL);
				return RPAR;
			}
			else if (ch == '[') {
				addTk(LBRACKET, NULL);
				return LBRACKET;
			}
			else if (ch == ']') {
				addTk(RBRACKET, NULL);
				return RBRACKET;
			}
			else if (ch == '{') {
				addTk(LACC, NULL);
				return LACC;
			}
			else if (ch == '}') {
				addTk(RACC, NULL);
				return RACC;
			}
			else if (ch == '\"') {
				state = 2;
			}
			else if (isdigit(ch)) {
				strcpy(token_name, &ch);
				cnt++;
				if (ch != '0')
					state = 3;
				else if (ch == '0') {
					ch = fgetc(fp);
					strcpy(token_name + cnt, &ch);
					cnt++;
					if (ch == 'x')
						state = 4;
					else if (ch >= '0' && ch <= '7')
						state = 5;
					else {
						if (!isdigit(ch)) {
							token_name[cnt] = '\0';
							ungetc(ch, fp);
							addTk(CT_INT, token_name);
							return CT_INT;
						}
						else
							tkerr("invalid character for int number");
					}

				}
			}
			else if (ch == '\'') {
				state = 7;
			}
			break;

			//keyword or identifier case
		case 1:
			if (isalnum(ch) || ch == '_') {
				strcpy(token_name + cnt, &ch);
				cnt++;
			}
			else {
				token_name[cnt] = '\0';
				ungetc(ch, fp);
				int tk_code = iskeyword(token_name);
				if (tk_code == 0) {
					addTk(ID, token_name);
					return ID;
				}
				else {
					addTk(tk_code, NULL);
					return tk_code;
				}
			}
			break;

			//string case
		case 2:
			if (ch == '\\') {
				strcpy(token_name + cnt, &ch);
				cnt++;
				ch = fgetc(fp);
				if (strchr("abfnrtv'?\"\\0", ch)) {
					strcpy(token_name + cnt, &ch);
					cnt++;
				}
				else
					tkerr("Invalid character \'\\\'");
			}
			else {
				if (ch == '\"') {
					token_name[cnt] = '\0';
					addTk(CT_STRING, token_name);
					return CT_STRING;
				}
				else {
					strcpy(token_name + cnt, &ch);
					cnt++;
				}
			}
			break;

			// decimal or float case
		case 3:
			if (isdigit(ch)) {
				strcpy(token_name + cnt, &ch);
				cnt++;
			}
			else if (ch == '.') {
				strcpy(token_name + cnt, &ch);
				cnt++;
				state = 6;
			}
			else {
				ungetc(ch, fp);
				token_name[cnt] = '\0';
				addTk(CT_INT, token_name);
				return CT_INT;
			}
			break;

			//hexa integer case
		case 4:
			if (isdigit(ch)) {
				strcpy(token_name + cnt, &ch);
				cnt++;
			}
			else if ((ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
				strcpy(token_name + cnt, &ch);
				cnt++;
			}
			else {
				token_name[cnt] = '\0';
				addTk(CT_INT, token_name);
				return CT_INT;
			}
			break;

			//octal integer case
		case 5:
			if ((ch >= '0' && ch <= '7')) {
				strcpy(token_name + cnt, &ch);
				cnt++;
			}
			else {

				token_name[cnt] = '\0';
				addTk(CT_INT, token_name);
				return CT_INT;
			}
			break;

			//float case
		case 6:
			if (isdigit(ch)) {
				strcpy(token_name + cnt, &ch);
				cnt++;
			}
			else if (ch == 'e' || ch == 'E') {
				strcpy(token_name + cnt, &ch);
				cnt++;
				ch = fgetc(fp);
				if (ch == '-' || ch == '+') {
					do {
						strcpy(token_name + cnt, &ch);
						cnt++;
						ch = fgetc(fp);
					} while (isdigit(ch));
					ungetc(ch, fp);
					token_name[cnt] = '\0';
					addTk(CT_REAL, token_name);
					return CT_REAL;
				}
				else
					tkerr("Invalid character at exponent of float");
			}
			else {
				ungetc(ch, fp);
				token_name[cnt] = '\0';
				addTk(CT_REAL, token_name);
				return CT_REAL;
			}
			break;

			//character case
		case 7:
			if (ch == '\\') {
				strcpy(token_name + cnt, &ch);
				cnt++;
				ch = fgetc(fp);
				if (strchr("abfnrtv'?\"\\0", ch)) {
					strcpy(token_name + cnt, &ch);
					cnt++;
					ch = fgetc(fp);
					if (ch == '\'') {
						addTk(CT_CHAR, &ch);
						return CT_CHAR;
					}
					else
						tkerr("Invalid character before \'");
				}
				else
					tkerr("Invalid character \'\\\'");
			}
			else
				if (ch != '\'') {
					strcpy(token_name + cnt, &ch);
					cnt++;
					ch = fgetc(fp);
					if (ch == '\'') {
						addTk(CT_CHAR, token_name);
						return CT_CHAR;
					}
					else
						tkerr("Invalid character before \'");
				}
			break;
		}
		ch = fgetc(fp);
	}
}

//------------------------------
//LEXICAL ANALYZER END



//SYMBOLS TABLE BEGIN
//------------------------------

struct _Symbol;
typedef struct _Symbol Symbol;
Symbol *crtStruct;
Symbol *crtFunc;

int crtDepth = 0;

enum { TB_INT, TB_DOUBLE, TB_CHAR, TB_STRUCT, TB_VOID };
enum { CLS_VAR, CLS_FUNC, CLS_EXTFUNC, CLS_STRUCT };
enum { MEM_GLOBAL, MEM_ARG, MEM_LOCAL };

typedef struct {
	Symbol **begin;			// the beginning of the symbols, or NULL
	Symbol **end;			// the position after the last symbol
	Symbol **after;			// the position after the allocated space
}Symbols;

void initSymbols(Symbols *symbols)
{
	symbols->begin = NULL;
	symbols->end = NULL;
	symbols->after = NULL;
}

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
Symbols symbols;

Symbol *addSymbol(Symbols *symbols, const char *name, int cls)
{
	Symbol *s;
	if (symbols->end == symbols->after) {				// create more room
		int count = symbols->after - symbols->begin;
		int n = count * 2;								// double the room
		if (n == 0) n = 1;								// needed for the initial case
		symbols->begin = (Symbol**)realloc(symbols->begin, n * sizeof(Symbol*));
		if (symbols->begin == NULL) err("not enough memory");
		symbols->end = symbols->begin + count;
		symbols->after = symbols->begin + n;
	}
	SAFEALLOC(s, Symbol)
		*symbols->end++ = s;
	s->name = name;
	s->cls = cls;
	s->depth = crtDepth;
	printf("Symbol name: %s", s->name);
	return s;
}

Symbol *findSymbol(Symbols *symbols, const char *name) {
	Symbol **s;
	if (symbols->begin != NULL) {
		for (s = (symbols->end) - 1; s != symbols->begin; s--)
			if (strcmp((*s)->name, name) == 0)
				return *s;
		if (strcmp((*s)->name, name) == 0)
			return *s;
	}
	return NULL;
}

Symbol *requireSymbol(Symbols *symbols, const char *name) {
	Symbol **s;
	if (symbols->begin != NULL) {
		for (s = (symbols->end) - 1; s != symbols->begin; s--)
			if (strcmp((*s)->name, name) == 0)
				return *s;
		if (strcmp((*s)->name, name) == 0)
			return *s;
	}
	tkerr("The symbol required was not found\n");
}

void deleteSymbolsAfter(Symbols *symbols, Symbol *symbol) {
	int crnt_position = 0, found_position;
	Symbol **s;
	for (s = symbols->begin; s != symbols->end; s++) {
		if ((*s) == symbol) {
			found_position = crnt_position;
			for (Symbol** to_delete = ++s; to_delete != symbols->end; to_delete++)
				free((*to_delete));
			symbols->end = symbols->begin + found_position + 1;
			break;
		}
		crnt_position++;
	}
}

void addVar(Token *tk_name, Type *type)
{
	printf("Token name: %s, type: %d\n", tk_name->text, type->typeBase);
	Symbol *s;
	if (crtStruct) {
		if (findSymbol(&crtStruct->members, tk_name->text))
			tkerr("symbol redefinition: %s", tk_name->text);
		s = addSymbol(&crtStruct->members, tk_name->text, CLS_VAR);
	}
	else if (crtFunc) {
		s = findSymbol(&symbols, tk_name->text);
		if (s&&s->depth == crtDepth)
			tkerr("symbol redefinition: %s", tk_name->text);
		s = addSymbol(&symbols, tk_name->text, CLS_VAR);
		s->mem = MEM_LOCAL;
	}
	else {
		if (findSymbol(&symbols, tk_name->text))
			tkerr("symbol redefinition: %s", tk_name->text);
		s = addSymbol(&symbols, tk_name->text, CLS_VAR);
		s->mem = MEM_GLOBAL;
	}
	s->type = *type;

	if (crtStruct || crtFunc)
		s->offset = offset;
	else
		s->addr = allocGlobal(typeFullSize(&s->type));
	offset += typeFullSize(&s->type);
}

//------------------------------
//SYMBOLS TABLE END



//TYPE ANALYSIS BEGIN
//------------------------------

void put_s();
void get_s();
void put_c();
void get_c();
void put_d();
void get_d();
void put_i();
void get_i();
void seconds();

Type createType(int typeBase, int nElements)
{
	Type t;
	t.typeBase = typeBase;
	t.nElements = nElements;
	return t;
}

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

Type getArithType(Type *s1, Type *s2) {
	switch (s1->typeBase) {
		case TB_DOUBLE: return *s1;
		case TB_CHAR: return *s2;
		case TB_INT :
			switch (s2->typeBase) {
				case TB_DOUBLE: return *s2;
				case TB_INT:
				case TB_CHAR:
					return *s1;
			}
	}
	return *s1;
}

void cast(Type *dst, Type *src)
{
	if (src->nElements>-1) {
		if (dst->nElements>-1) {
			if (src->typeBase != dst->typeBase)
				tkerr("an array cannot be converted to an array of another type");
		}
		else {
			tkerr("an array cannot be converted to a non-array");
		}
	}
	else {
		if (dst->nElements>-1) {
			tkerr("a non-array cannot be converted to an array");
		}
	}
	switch (src->typeBase) {
	case TB_CHAR:
	case TB_INT:
	case TB_DOUBLE:
		switch (dst->typeBase) {
		case TB_CHAR:
		case TB_INT:
		case TB_DOUBLE:
			return;
		}
	case TB_STRUCT:
		if (dst->typeBase == TB_STRUCT) {
			if (src->s != dst->s)
				tkerr("a structure cannot be converted to another one");
			return;
		}
	}
	tkerr("incompatible types");
}

Symbol *addExtFunc(const char *name, Type type, void *addr)
{
	Symbol *s = addSymbol(&symbols, name, CLS_EXTFUNC);
	s->type = type;
	s->addr = addr;
	initSymbols(&s->args);
	return s;
}

Symbol *addFuncArg(Symbol *func, const char *name, Type type)
{
	Symbol *a = addSymbol(&func->args, name, CLS_VAR);
	a->type = type;
	return a;
}

void addExtFuncs() {
	Symbol *symbol, *args;

	symbol = addExtFunc("put_s", createType(TB_VOID, -1), put_s);
	args = addFuncArg(symbol, "s", createType(TB_CHAR, 0));

	symbol = addExtFunc("get_s", createType(TB_VOID, -1), get_s);
	args = addFuncArg(symbol, "s", createType(TB_CHAR, 0));

	symbol = addExtFunc("put_i", createType(TB_VOID, -1), put_i);
	args = addFuncArg(symbol, "i", createType(TB_INT, -1));

	symbol = addExtFunc("get_i", createType(TB_INT, -1), get_i);

	symbol = addExtFunc("put_d", createType(TB_VOID, -1), put_d);
	args = addFuncArg(symbol, "d", createType(TB_DOUBLE, -1));

	symbol = addExtFunc("get_d", createType(TB_DOUBLE, -1), get_d);

	symbol = addExtFunc("put_c", createType(TB_VOID, -1), put_c);
	args = addFuncArg(symbol, "c", createType(TB_CHAR, -1));

	symbol = addExtFunc("get_c", createType(TB_CHAR, -1), get_c);

	symbol = addExtFunc("seconds", createType(TB_DOUBLE, -1), seconds);
}

//------------------------------
//TYPE ANALYSIS END



//CODE GENERATION BEGIN
//------------------------------

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

Instr *instructions, *lastInstruction; // double linked list
Instr *crtLoopEnd;   //for the FOR, WHILE loop instruction

int typeArgSize(Type *type);
Instr *addInstrI(int opcode, long int val);
int typeBaseSize(Type *type);

void *allocGlobal(int size) {
	void *p = globals + nGlobals;
	if (nGlobals + size>GLOBAL_SIZE)
		err("insufficient globals space");
	nGlobals += size;
	return p;
}
Instr *getRVal(RetVal *rv)
{
	if (rv->isLVal) {
		switch (rv->type.typeBase) {
		case TB_INT:
		case TB_DOUBLE:
		case TB_CHAR:
		case TB_STRUCT:
			addInstrI(O_LOAD, typeArgSize(&(rv->type)));
			break;
		default:
			tkerr("unhandled type: %d", rv->type.typeBase);
		}
	}
	return lastInstruction;
}


Instr *createInstr(int opcode){
	Instr *i;
	SAFEALLOC(i, Instr)
	i->opcode = opcode;
	return i;
}
void insertInstrAfter(Instr *after, Instr *i){
	i->next = after->next;
	i->last = after;
	after->next = i;
	if (i->next == NULL)lastInstruction = i;
}

Instr *addInstr(int opcode){
	Instr *i = createInstr(opcode);
	i->next = NULL;
	i->last = lastInstruction;
	if (lastInstruction)
		lastInstruction->next = i;
	else
		instructions = i;
	lastInstruction = i;
	return i;
}

Instr *appendInstr(Instr *i) {
	i->next = NULL;
	i->last = lastInstruction;
	if (lastInstruction)
		lastInstruction->next = i;
	else
		instructions = i;
	lastInstruction = i;
}

Instr *addInstrAfter(Instr *after, int opcode)
{
	Instr *i = createInstr(opcode);
	insertInstrAfter(after, i);
	return i;
}
Instr *addInstrA(int opcode, void *addr) {
	Instr *instr = addInstr(opcode);
	instr->args[0].addr = addr;
	return instr;
}

Instr *addInstrI(int opcode, long int val) {
	Instr *instr = addInstr(opcode);
	instr->args[0].i = val;
	return instr;
}

Instr *addInstrII(int opcode, long int val1, long int val2) {
	Instr *instr = addInstr(opcode);
	instr->args[0].i = val1;
	instr->args[1].i = val2;
	return instr;
}

void addCastInstr(Instr *after, Type *actualType, Type *neededType)
{
	if (actualType->nElements >= 0 || neededType->nElements >= 0)return;
	switch (actualType->typeBase) {
	case TB_CHAR:
		switch (neededType->typeBase) {
		case TB_CHAR:break;
		case TB_INT:addInstrAfter(after, O_CAST_C_I); break;
		case TB_DOUBLE:addInstrAfter(after, O_CAST_C_D); break;
		}
		break;
	case TB_INT:
		switch (neededType->typeBase) {
		case TB_CHAR:addInstrAfter(after, O_CAST_I_C); break;
		case TB_INT:break;
		case TB_DOUBLE:addInstrAfter(after, O_CAST_I_D); break;
		}
		break;
	case TB_DOUBLE:
		switch (neededType->typeBase) {
		case TB_CHAR:addInstrAfter(after, O_CAST_D_C); break;
		case TB_INT:addInstrAfter(after, O_CAST_D_I); break;
		case TB_DOUBLE:break;
		}
		break;
	}
}

Instr *createCondJmp(RetVal *rv)
{
	if (rv->type.nElements >= 0)    // arrays
		return addInstr(O_JF_A);
	else {                          // non-arrays
		getRVal(rv);
		switch (rv->type.typeBase) {
		case TB_CHAR:return addInstr(O_JF_C);
		case TB_DOUBLE:return addInstr(O_JF_D);
		case TB_INT:return addInstr(O_JF_I);
		default:return NULL;
		}
	}
}

void deleteInstructionsAfter(Instr *start) {
	Instr *crnt_instr = instructions;
	for (Instr *p = start->next; p != NULL; p = p->next)
		free(p);
	start->next = NULL;
	lastInstruction = start;
}

void pushd(double d) {
	if (SP + sizeof(double)>stackAfter)
		err("out of stack");
	*(double*)SP = d;
	SP += sizeof(double);
}

double popd() {
	SP -= sizeof(double);
	if (SP<stack)
		err("not enough stack bytes for popd");
	return *(double*)SP;
}

void pushi(long int i) {
	if (SP + sizeof(long int)>stackAfter)
		err("out of stack");
	*(long int*)SP = i;
	SP += sizeof(long int);
}

long int popi() {
	SP -= sizeof(long int);
	if (SP<stack)
		err("not enough stack bytes for popd");
	return *(long int*)SP;
}

void pusha(void *a){
	if (SP + sizeof(void*)>stackAfter)
		err("out of stack");
	*(void**)SP = a;
	SP += sizeof(void*);
}

void *popa()
{
	SP -= sizeof(void*);
	if (SP<stack)
		err("not enough stack bytes for popa");
	return *(void**)SP;
}
void pushc(char c) {
	if (SP + sizeof(char)>stackAfter)
		err("out of stack");
	*(char*)SP = c;
	SP += sizeof(char);
}

char popc() {
	SP -= sizeof(char);
	if (SP<stack)
		err("not enough stack bytes for popd");
	return *(char*)SP;
}

int typeFullSize(Type *type) {
	return typeBaseSize(type)*(type->nElements>0 ? type->nElements : 1);
}

int typeArgSize(Type *type) {
	if (type->nElements >= 0)return sizeof(void*);
	return typeBaseSize(type);
}

int typeBaseSize(Type *type)
{
	int size = 0;
	Symbol **is;
	switch (type->typeBase) {
		case TB_INT:size = sizeof(long int); break;
		case TB_DOUBLE:size = sizeof(double); break;
		case TB_CHAR:size = sizeof(char); break;
		case TB_STRUCT:
			for (is = type->s->members.begin; is != type->s->members.end; is++) {
				size += typeFullSize(&(*is)->type);
			}
			break;
		case TB_VOID:size = 0; break;
		default:err("invalid typeBase: %d", type->typeBase);
		}
	return size;
}
void put_s() {
	char *s = (char *)popa();
	printf("#%s\n", s);
}

void get_s() {
	char *s = (char *)popa();
	scanf("%s", s);
}

void put_i() {
	printf("#%ld\n", (long int)popi());
}

void get_i() {
	long int i;
	scanf("%ld", &i);
	pushi(i);
}

void put_d() {
	printf("#%lf\n", (double)popd());
}

void get_d() {
	float d;
	scanf("%lf", &d);
	pushd(d);
}

void put_c() {
	printf("#%c\n", (char)popc());
}

void get_c() {
	char c;
	scanf("%c", &c);
	pushc(c);
}

void seconds() {
	pushd(0.0);
}

void run(Instr *IP) {
	long int iVal1, iVal2;
	double dVal1, dVal2;
	char cVal1, cVal2;
	char *aVal1, *aVal2;
	char *FP = 0, *oldSP;
	SP = stack;
	stackAfter = stack + STACK_SIZE;
	while (1) {
		printf("%p/%ld\t", IP, SP - stack);
		switch (IP->opcode) {
		case O_ADD_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_ADD_C\t(%c+%c -> %c)\n", cVal2, cVal1, cVal2 + cVal1);
			pushc(cVal2 + cVal1);
			IP = IP->next;
			break;
		case O_ADD_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_ADD_D\t(%lf+%lf -> %lf)\n", dVal2, dVal1, dVal2 + dVal1);
			pushc(dVal2 + dVal1);
			IP = IP->next;
			break;
		case O_ADD_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_ADD_I\t(%ld+%ld -> %ld)\n", iVal2, iVal1, iVal2 + iVal1);
			pushi(iVal2 + iVal1);
			IP = IP->next;
			break;
		case O_AND_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_AND_C\t(%c&&%c -> %c)\n", cVal2, cVal1, cVal2&&cVal1);
			pushc(cVal2&&cVal1);
			IP = IP->next;
			break;
		case O_AND_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_AND_D\t(%lf&&%lf -> %lf\n", dVal2, dVal1, (double)(dVal2&&dVal1));
			pushd((double)(dVal2&&dVal1));
			IP = IP->next;
			break;
		case O_AND_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_AND_I\t(%ld&&%ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2&&iVal1));
			pushi((long int)(iVal2&&iVal1));
			IP = IP->next;
			break;
		case O_AND_A:
			aVal1 = (char *)popa();
			aVal2 = (char *)popa();
			printf("O_AND_A\t(%p&&%p -> %i)\n", aVal2, aVal1, (aVal2&&aVal1));
			pushi((aVal2&&aVal1));
			IP = IP->next;
			break;
		case O_CALL:
			aVal1 = (char *)IP->args[0].addr;
			printf("CALL\t%p\n", aVal1);
			pusha(IP->next);
			IP = (Instr*)aVal1;
			break;
		case O_CALLEXT:
			printf("O_CALLEXT\t%p\n", IP->args[0].addr);
			(*(void(*)())IP->args[0].addr)();
			IP = IP->next;
			break;
		case O_CAST_I_C:
			iVal1 = popi();
			cVal1 = (char)iVal1;
			printf("CAST_I_C\t(%ld -> %c)\n", iVal1, cVal1);
			pushc(cVal1);
			IP = IP->next;
			break;
		case O_CAST_I_D:
			iVal1 = popi();
			dVal1 = (double)iVal1;
			printf("CAST_I_D\t(%ld -> %lf)\n", iVal1, dVal1);
			pushd(dVal1);
			IP = IP->next;
			break;
		case O_CAST_C_D:
			cVal1 = popc();
			dVal1 = (double)cVal1;
			printf("O_CAST_C_D\t(%c -> %lf)\n", cVal1, dVal1);
			pushd(dVal1);
			IP = IP->next;
			break;
		case O_CAST_C_I:
			cVal1 = popc();
			iVal1 = (int)cVal1;
			printf("O_CAST_C_I\t(%c -> %ld)\n", cVal1, iVal1);
			pushi(iVal1);
			IP = IP->next;
			break;
		case O_CAST_D_C:
			dVal1 = popd();
			cVal1 = (char)dVal1;
			printf("O_CAST_D_C\t(%lf -> %c)\n", dVal1, cVal1);
			pushc(cVal1);
			IP = IP->next;
			break;
		case O_CAST_D_I:
			dVal1 = popd();
			iVal1 = (long int)dVal1;
			printf("O_CAST_D_I\t(%lf -> %ld)\n", dVal1, iVal1);
			pushi(iVal1);
			IP = IP->next;
			break;
		case O_DIV_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_DIV_C\t(%c/%c -> %c)\n", cVal2, cVal1, cVal2 / cVal1);
			pushc(cVal2 / cVal1);
			IP = IP->next;
			break;
		case O_DIV_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_DIV_D\t(%lf/%lf -> %lf)\n", dVal2, dVal1, dVal2 / dVal1);
			pushc(dVal2 / dVal1);
			IP = IP->next;
			break;
		case O_DIV_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_DIV_I\t(%ld/%ld -> %ld)\n", iVal2, iVal1, iVal2 / iVal1);
			pushi(iVal2 / iVal1);
			IP = IP->next;
			break;
		case O_DROP:
			iVal1 = IP->args[0].i;
			printf("DROP\t%ld\n", iVal1);
			if (SP - iVal1<stack)err("not enough stack bytes");
			SP -= iVal1;
			IP = IP->next;
			break;
		case O_ENTER:
			iVal1 = IP->args[0].i;
			printf("ENTER\t%ld\n", iVal1);
			pusha(FP);
			FP = SP;
			SP += iVal1;
			IP = IP->next;
			break;
		case O_EQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_EQ_C\t(%c==%c -> %ld)\n", cVal2, cVal1, (long int)(cVal2 == cVal1));
			pushi(cVal2 == cVal1);
			IP = IP->next;
			break;
		case O_EQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_EQ_D\t(%lf==%lf -> %ld)\n", dVal2, dVal1, (long int)(dVal2 == dVal1));
			pushi(dVal2 == dVal1);
			IP = IP->next;
			break;
		case O_EQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_EQ_I\t(%ld==%ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2 == iVal1));
			pushi(iVal2 == iVal1);
			IP = IP->next;
			break;
		case O_EQ_A:
			aVal1 = (char *)popa();
			aVal2 = (char *)popa();
			printf("O_EQ_A\t(%p==%p -> %ld)\n", aVal2, aVal1, (long int)(aVal2 == aVal1));
			pushi(aVal2 == aVal1);
			IP = IP->next;
			break;
		case O_GREATER_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_GREATER_C\t(%c>%c -> %ld)\n", cVal2, cVal1, (long int)(cVal2>cVal1));
			pushi(cVal2>cVal1);
			IP = IP->next;
			break;
		case O_GREATER_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_GREATER_D\t(%lf>%lf -> %ld)\n", dVal2, dVal1, (long int)(dVal2>dVal1));
			pushi(dVal2>dVal1);
			IP = IP->next;
			break;
		case O_GREATER_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_GREATER_I\t(%ld>%ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2>iVal1));
			pushi(iVal2>iVal1);
			IP = IP->next;
			break;
		case O_GREATEREQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_GREATEREQ_C\t(%c>=%c -> %ld)\n", cVal2, cVal1, (long int)(cVal2 >= cVal1));
			pushi(cVal2 >= cVal1);
			IP = IP->next;
			break;
		case O_GREATEREQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_GREATEREQ_D\t(%lf>=%lf -> %ld)\n", dVal2, dVal1, (long int)(dVal2 >= dVal1));
			pushi(dVal2 >= dVal1);
			IP = IP->next;
			break;
		case O_GREATEREQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_GREATEREQ_I\t(%ld>=%ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2 >= iVal1));
			pushi(iVal2 >= iVal1);
			IP = IP->next;
			break;
		case O_HALT:
			printf("HALT\n");
			return;
		case O_INSERT:
			iVal1 = IP->args[0].i;	// iDst
			iVal2 = IP->args[1].i;	// nBytes
			printf("INSERT\t%ld,%ld\n", iVal1, iVal2);
			if (SP + iVal2>stackAfter)err("out of stack");
			memmove(SP - iVal1 + iVal2, SP - iVal1, iVal1); //make room
			memmove(SP - iVal1, SP + iVal2, iVal2);         //dup
			SP += iVal2;
			IP = IP->next;
			break;
		case O_JF_C:
			cVal1 = popc();
			printf("O_JF_C\t%p\t(%c)\n", IP->args[0].addr, cVal1);
			IP = (Instr *) ((cVal1 == 0) ? IP->args[0].addr : IP->next);
			break;
		case O_JF_D:
			dVal1 = popd();
			printf("O_JF_D\t%p\t(%lf)\n", IP->args[0].addr, dVal1);
			IP = (Instr *) ((dVal1 == 0.0) ? IP->args[0].addr : IP->next);
			break;
		case O_JF_I:
			iVal1 = popi();
			printf("JF\t%p\t(%ld)\n", IP->args[0].addr, iVal1);
			IP = (Instr *) ((iVal1 == 0) ? IP->args[0].addr : IP->next);
			break;
		case O_JF_A:
			aVal1 = (char *)popa();
			printf("O_JF_A\t%p\t(%p)\n", IP->args[0].addr, aVal1);
			IP = (Instr *) ((aVal1 == 0) ? IP->args[0].addr : IP->next);
			break;
		case O_JMP:
			printf("O_JMP\t%p\n", IP->args[0].addr);
			IP = (Instr *) IP->args[0].addr;
			break;
		case O_JT_C:
			cVal1 = popc();
			printf("O_JT_C\t%p\t(%c)\n", IP->args[0].addr, cVal1);
			IP = (Instr *) (cVal1 ? IP->args[0].addr : IP->next);
			break;
		case O_JT_D:
			dVal1 = popd();
			printf("O_JT_D\t%p\t(%lf)\n", IP->args[0].addr, dVal1);
			IP = (Instr *) (dVal1 ? IP->args[0].addr : IP->next);
			break;
		case O_JT_I:
			iVal1 = popi();
			printf("O_JT_I\t%p\t(%ld)\n", IP->args[0].addr, iVal1);
			IP = (Instr *) (iVal1 ? IP->args[0].addr : IP->next);
			break;
		case O_JT_A:
			aVal1 = (char *)popa();
			printf("O_JT_A\t%p\t(%p)\n", IP->args[0].addr, aVal1);
			IP = (Instr *) (aVal1 ? IP->args[0].addr : IP->next);
			break;
		case O_LESS_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_LESS_C\t(%c < %c -> %ld)\n", cVal2, cVal1, (long int)(cVal2<cVal1));
			pushi(cVal2<cVal1);
			IP = IP->next;
			break;
		case O_LESS_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_LESS_D\t(%lf < %lf -> %ld)\n", dVal2, dVal1, (long int)(dVal2<dVal1));
			pushi(dVal2<dVal1);
			IP = IP->next;
			break;
		case O_LESS_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_LESS_I\t(%ld < %ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2<iVal1));
			pushi(iVal2<iVal1);
			IP = IP->next;
			break;
		case O_LESSEQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_LESSEQ_C\t(%c <= %c -> %ld)\n", cVal2, cVal1, (long int)(cVal2 <= cVal1));
			pushi(cVal2 <= cVal1);
			IP = IP->next;
			break;
		case O_LESSEQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_LESSEQ_D\t(%lf <= %lf -> %ld)\n", dVal2, dVal1, (long int)(dVal2 <= dVal1));
			pushi(dVal2 <= dVal1);
			IP = IP->next;
			break;
		case O_LESSEQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_LESSEQ_I\t(%ld <= %ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2 <= iVal1));
			pushi(iVal2 <= iVal1);
			IP = IP->next;
			break;
		case O_LOAD:
			iVal1 = IP->args[0].i;
			aVal1 = (char *) popa();
			printf("LOAD\t%ld\t(%p)\n", iVal1, aVal1);
			if (SP + iVal1>stackAfter)err("out of stack");
			memcpy(SP, aVal1, iVal1);
			SP += iVal1;
			IP = IP->next;
			break;
		case O_MUL_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_MUL_C\t(%c * %c -> %c)\n", cVal2, cVal1, cVal2*cVal1);
			pushc(cVal2*cVal1);
			IP = IP->next;
			break;
		case O_MUL_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_MUL_D\t(%lf * %lf -> %lf)\n", dVal2, dVal1, dVal2*dVal1);
			pushc(dVal2*dVal1);
			IP = IP->next;
			break;
		case O_MUL_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_MUL_I\t(%ld * %ld -> %ld)\n", iVal2, iVal1, iVal2*iVal1);
			pushi(iVal2*iVal1);
			IP = IP->next;
			break;
		case O_NEG_C:
			cVal1 = popc();
			printf("O_NEG_C\t(%c -> %c)\n", cVal1, -cVal1);
			pushc(-cVal1);
			IP = IP->next;
			break;
		case O_NEG_D:
			dVal1 = popd();
			printf("O_NEG_D\t(%lf -> %lf)\n", dVal1, -dVal1);
			pushc(-dVal1);
			IP = IP->next;
			break;
		case O_NEG_I:
			iVal1 = popi();
			printf("O_NEG_I\t(%ld -> %ld)\n", iVal1, -iVal1);
			pushi(-iVal1);
			IP = IP->next;
			break;
		case O_NOP:
			IP = IP->next;
			break;
		case O_NOT_C:
			cVal1 = popc();
			printf("O_NOT_C\t(%c -> %c)\n", cVal1, !cVal1);
			pushc(!cVal1);
			IP = IP->next;
			break;
		case O_NOT_D:
			dVal1 = popd();
			printf("O_NOT_D\t(%lf -> %lf)\n", dVal1, (double)(!dVal1));
			pushc((double)(!dVal1));
			IP = IP->next;
			break;
		case O_NOT_I:
			iVal1 = popi();
			printf("O_NOT_I\t(%ld -> %ld)\n", iVal1, (long int)(!iVal1));
			pushi((long int)(!iVal1));
			IP = IP->next;
			break;
		case O_NOT_A:
			aVal1 = (char *)popa();
			printf("O_NOT_A\t(%p -> %i)\n", aVal1, (!aVal1));
			pushi((!aVal1));
			IP = IP->next;
			break;
		case O_NOTEQ_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_NOTEQ_C\t(%c != %c -> %ld)\n", cVal2, cVal1, (long int)(cVal2 != cVal1));
			pushi(cVal2 != cVal1);
			IP = IP->next;
			break;
		case O_NOTEQ_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_NOTEQ_D\t(%lf != %lf -> %ld)\n", dVal2, dVal1, (long int)(dVal2 != dVal1));
			pushi(dVal2 != dVal1);
			IP = IP->next;
			break;
		case O_NOTEQ_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_NOTEQ_I\t(%ld != %ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2 != iVal1));
			pushi(iVal2 != iVal1);
			IP = IP->next;
			break;
		case O_NOTEQ_A:
			aVal1 = (char *)popa();
			aVal2 = (char *)popa();
			printf("O_NOTEQ_A\t(%p != %p -> %ld)\n", aVal2, aVal1, (long int)(aVal2 != aVal1));
			pushi(aVal2 != aVal1);
			IP = IP->next;
			break;
		case O_OFFSET:
			iVal1 = popi();
			aVal1 = (char *) popa();
			printf("OFFSET\t(%p + %ld -> %p)\n", aVal1, iVal1, aVal1 + iVal1);
			pusha(aVal1 + iVal1);
			IP = IP->next;
			break;
		case O_OR_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_OR_C\t(%c || %c -> %ld)\n", cVal2, cVal1, (long int)(cVal2 || cVal1));
			pushi(cVal2 || cVal1);
			IP = IP->next;
			break;
		case O_OR_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_OR_D\t(%lf || %lf -> %ld)\n", dVal2, dVal1, (long int)(dVal2 || dVal1));
			pushi(dVal2 || dVal1);
			IP = IP->next;
			break;
		case O_OR_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_OR_I\t(%ld || %ld -> %ld)\n", iVal2, iVal1, (long int)(iVal2 || iVal1));
			pushi(iVal2 || iVal1);
			IP = IP->next;
			break;
		case O_OR_A:
			aVal1 = (char *)popa();
			aVal2 = (char *)popa();
			printf("O_OR_A\t(%p || %p -> %ld)\n", aVal2, aVal1, (long int)(aVal2 || aVal1));
			pushi(aVal2 || aVal1);
			IP = IP->next;
			break;
		case O_PUSHFPADDR:
			iVal1 = IP->args[0].i;
			printf("PUSHFPADDR\t%ld\t(%p)\n", iVal1, FP + iVal1);
			pusha(FP + iVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_C:
			iVal1 = IP->args[0].i;
			printf("O_PUSHCT_C\t%c\n", (char)iVal1);
			pushc(iVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_D:
			dVal1 = IP->args[0].d;
			printf("O_PUSHCT_D\t%lf\n", dVal1);
			pushd(dVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_I:
			iVal1 = IP->args[0].i;
			printf("PUSHCT_I\t%ld\n", iVal1);
			pushi(iVal1);
			IP = IP->next;
			break;
		case O_PUSHCT_A:
			aVal1 = (char *)IP->args[0].addr;
			printf("PUSHCT_A\t%p\n", aVal1);
			pusha(aVal1);
			IP = IP->next;
			break;
		case O_RET:
			iVal1 = IP->args[0].i;  // sizeArgs
			iVal2 = IP->args[1].i;  // sizeof(retType)
			printf("RET\t%ld,%ld\n", iVal1, iVal2);
			oldSP = SP;
			SP = FP;
			FP = (char *) popa();
			IP = (Instr *) popa();
			if (SP - iVal1<stack)err("not enough stack bytes");
			SP -= iVal1;
			memmove(SP, oldSP - iVal2, iVal2);
			SP += iVal2;
			break;
		case O_STORE:
			iVal1 = IP->args[0].i;
			if (SP - (sizeof(void*) + iVal1)<stack)
				err("not enough stack bytes for SET");
			aVal1 = (char *) (*(void**)(SP - ((sizeof(void*) + iVal1))));
			printf("STORE\t%ld\t(%p)\n", iVal1, aVal1);
			memcpy(aVal1, SP - iVal1, iVal1);
			SP -= sizeof(void*) + iVal1;
			IP = IP->next;
			break;
		case O_SUB_C:
			cVal1 = popc();
			cVal2 = popc();
			printf("O_SUB_C\t(%c - %c -> %c)\n", cVal2, cVal1, cVal2 - cVal1);
			pushc(cVal2 - cVal1);
			IP = IP->next;
			break;
		case O_SUB_D:
			dVal1 = popd();
			dVal2 = popd();
			printf("O_SUB_D\t(%lf - %lf -> %lf)\n", dVal2, dVal1, dVal2 - dVal1);
			pushd(dVal2 - dVal1);
			IP = IP->next;
			break;
		case O_SUB_I:
			iVal1 = popi();
			iVal2 = popi();
			printf("O_SUB_I\t(%ld - %ld -> %ld)\n", iVal2, iVal1, iVal2 - iVal1);
			pushi(iVal2 - iVal1);
			IP = IP->next;
			break;
		default:
			err("invalid opcode: %d", IP->opcode);
		}
	}
}

//------------------------------
//CODE GENERATION END



//SYNTACTIC ANALYZER BEGIN
//------------------------------

Token *consumedTk;
Token *crntTk;

int consume_token(int code)
{
	if (crntTk->code == code) {
		consumedTk = crntTk;
		crntTk = crntTk->next;
		return 1;
	}
	return 0;
}

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


int unit() {
	initSymbols(&symbols);

	//Code generation block
	Instr *labelMain = addInstr(O_CALL);
	addInstr(O_HALT);

	addExtFuncs();

	crntTk = tokens;
	while (crntTk) {
		if (decl_struct())
			continue;
		if (decl_func())
			continue;
		if (decl_var())
			continue;
		break;
	}

	//Code generation block
	labelMain->args[0].addr = requireSymbol(&symbols, "main")->addr;
	run(labelMain);

	return 1;
}

int func_arg() {
	Type type;
	Token *startTk = crntTk;

	if (typebase(&type)) {
		Token *tk_name = crntTk;
		if (consume_token(ID)) {
			if(!array_decl(&type))
				type.nElements = -1;
			Symbol  *s = addSymbol(&symbols, tk_name->text, CLS_VAR);
			s->mem = MEM_ARG;
			s->type = type;

			//Code generation
			s->offset = offset;

			s = addSymbol(&crtFunc->args, tk_name->text, CLS_VAR);
			s->mem = MEM_ARG;
			s->type = type;

			//Code generation
			s->offset = offset;

			//Code generation
			offset += typeArgSize(&s->type);

			return 1;
		}else
			tkerr("Missing function argument ID");
	}
	crntTk = startTk;
	return 0;
}

int function_body(Token *tk_name, Type type) {
	Token *startTk = crntTk;
	Symbol **ps;

	if (consume_token(LPAR)) {
		if (findSymbol(&symbols, tk_name->text))
			tkerr("symbol redefinition: %s", tk_name->text);
		crtFunc = addSymbol(&symbols, tk_name->text, CLS_FUNC);
		initSymbols(&crtFunc->args);
		crtFunc->type = type;
		crtDepth++;
		if (func_arg()) {
			while (1) {
				if (consume_token(COMMA)) {
					if (func_arg())
						continue;
					else
						tkerr("Missing function argument after comma");
				}
			    break;
			}
		}
		if (consume_token(RPAR)) {

			//Code generation begin
			crtFunc->addr = addInstr(O_ENTER);
			sizeArgs = offset;
			//update args offsets for correct FP indexing
			for (ps = symbols.begin; ps != symbols.end; ps++) {
				if ((*ps)->mem == MEM_ARG) {
					//2*sizeof(void*) == sizeof(retAddr)+sizeof(FP)
					(*ps)->offset -= sizeArgs + 2 * sizeof(void*);
				}
			}
			offset = 0;
			//Code generation end

			crtDepth--;
			if (stm_compound()) {
				deleteSymbolsAfter(&symbols, crtFunc);

				//Code generation begin
				((Instr*)crtFunc->addr)->args[0].i = offset;  // setup the ENTER argument
				if (crtFunc->type.typeBase == TB_VOID) {
					addInstrII(O_RET, sizeArgs, 0);
				}
				//Code generation end

				crtFunc = NULL;
				return 1;
			}
			else tkerr("Missing compound statement in function body");
		}else
			tkerr("Missing ')' in function arguments");
	}
	crntTk = startTk;
	return 0;
}

int decl_func() {
	Token *startTk = crntTk;
	Type type;

	if (typebase(&type)) {
		if (consume_token(MUL))
			type.nElements = 0;
		else
			type.nElements = -1;
	}else
		if (consume_token(VOID))
			type.typeBase = TB_VOID;

	Token *tk_name = crntTk;
	if (consume_token(ID)) {
		sizeArgs = offset = 0;
		if (function_body(tk_name,type)){
			return 1;
		}
		tkerr("Error on declarig");
	}
	crntTk = startTk;
	return 0;
}

int rule_while(RetVal *rv, Instr *i1, Instr *i2){
	Token *startTk = crntTk;
	if (consume_token(WHILE)) {

		//Code generation begin
		Instr *oldLoopEnd = crtLoopEnd;
		crtLoopEnd = createInstr(O_NOP);
		i1 = lastInstruction;
		//Code generation end

		if (consume_token(LPAR)) {
			if (expr(rv)) {
				if (rv->type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");
				if (consume_token(RPAR)) {

					//Code generation
					i2 = createCondJmp(rv);

					if (stm()) {

						//Code generation begin
						addInstrA(O_JMP, i1->next);
						appendInstr(crtLoopEnd);
						i2->args[0].addr = crtLoopEnd;
						crtLoopEnd = oldLoopEnd;
						//Code generation end

						return 1;
					}
					else tkerr("Missing while statement");
				}
				else tkerr("Missing ')'");
			}
			else tkerr("Invalid expression after '(' in while");
		}
		else tkerr("Missing '(' after while");
	}
	crntTk = startTk;
	return 0;
}

int rule_break() {
	Token *startTk = crntTk;
	if (consume_token(BREAK)) {
		if (consume_token(SEMICOLON)) {
			//Code generation begin
			if (!crtLoopEnd)
				tkerr("break without for or while");
			addInstrA(O_JMP, crtLoopEnd);
				//Code generation end
			return 1;
		}
		else tkerr("Missing semicolon after break");	}
	crntTk = startTk;
	return 0;
}

int rule_if(RetVal *rv, Instr  *i1, Instr *i2) {
	Token *startTk = crntTk;
	if (consume_token(IF)) {
		if (consume_token(LPAR)) {
			if (expr(rv)) {
				if (rv->type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");
				if (consume_token(RPAR)) {

					//Code generation
					i1 = createCondJmp(rv);

					if (stm()) {
						if (consume_token(ELSE)) {

							//Code generation
							i2 = addInstr(O_JMP);

							if (stm()) {

								//Code generation begin
								i1->args[0].addr = i2->next;
								i1 = i2;
								//Code generation end

								return 1;
							}
							else tkerr("Missing else statement");

							//Code generation
							i1->args[0].addr = addInstr(O_NOP);

						}
						return 1;
					}
					else tkerr("Missing if statement");
				}
				else tkerr("Missing ')'");
			}
			else tkerr("Invalid expression after '(' in if");
		}
		else tkerr("Missing '(' after if");
	}
	crntTk = startTk;
	return 0;
}

int rule_for(Instr *i2, Instr* i3, Instr* i4, Instr *is, Instr *ib3, Instr *ibs) {
	RetVal rv1, rv2, rv3;
	Token *startTk = crntTk;
	if (consume_token(FOR)) {

		//Code generation begin
		Instr *oldLoopEnd = crtLoopEnd;
		crtLoopEnd = createInstr(O_NOP);
		//Code generation end

		if (consume_token(LPAR)) {
			expr(&rv1);

			//Code generation begin
			if(typeArgSize(&rv1.type))
				addInstrI(O_DROP, typeArgSize(&rv1.type));
			//Code generation end

			if (consume_token(SEMICOLON)) {

				//Code generation
				i2 = lastInstruction;

				if (expr(&rv2)) {

					//Code generation
					i4 = createCondJmp(&rv2);

					if (rv2.type.typeBase == TB_STRUCT)
						tkerr("a structure cannot be logically tested");
				}
				if (consume_token(SEMICOLON)) {

					//Code generation
					ib3 = lastInstruction;

					expr(&rv3);

					//Code generation begin
					if (typeArgSize(&rv3.type))
						addInstrI(O_DROP, typeArgSize(&rv3.type));
					//Code generation end

					if (consume_token(RPAR)) {

						//Code generation
						ibs = lastInstruction;

						if (stm()) {

							//Code generation begin
							// if rv3 exists, exchange rv3 code with stm code: rv3 stm -> stm rv3
							if (ib3 != ibs) {
								i3 = ib3->next;
								is = ibs->next;
								ib3->next = is;
								is->last = ib3;
								lastInstruction->next = i3;
								i3->last = lastInstruction;
								ibs->next = NULL;
								lastInstruction = ibs;
							}
							addInstrA(O_JMP, i2->next);
							appendInstr(crtLoopEnd);
							if (i4)i4->args[0].addr = crtLoopEnd;
							crtLoopEnd = oldLoopEnd;
							//Code generation end

							return 1;
						}
						else
							tkerr("Missing for statement");
					}else
						tkerr("Missing ')' in for statement");
				}else
					tkerr("Missing ';' in for statement");
			}else
				tkerr("Missing ';' for statement");
		}else
			tkerr("Missing '(' for statement");
	}
	crntTk = startTk;
	return 0;
}

int rule_return(RetVal *rv, Instr *i){
	Token *startTk = crntTk;
	if (consume_token(RETURN)) {
		if(expr(rv)){

			//Code generation begin
			i = getRVal(rv);
			addCastInstr(i, &(rv->type), &crtFunc->type);
			//Code generation end

			if (crtFunc->type.typeBase == TB_VOID)
				tkerr("a void function cannot return a value");
			cast(&crtFunc->type, &(rv->type));
		}
		if (consume_token(SEMICOLON)) {

			//Code generation begin
			if (crtFunc->type.typeBase == TB_VOID)
				addInstrII(O_RET, sizeArgs, 0);
			else
				addInstrII(O_RET, sizeArgs, typeArgSize(&crtFunc->type));
			//Code generation end

			return 1;
		}
		else
			tkerr("Missing ';' after return");
	}
	crntTk = startTk;
	return 0;
}

int stm() {
	RetVal rv;
	//Code generation
	Instr *i, *i1, *i2, *i3, *i4, *is, *ib3, *ibs;

	if (stm_compound())
		return 1;
	if (rule_if(&rv, i1, i2))
		return 1;
	if (rule_while(&rv, i1, i2))
		return 1;
	if (rule_for(i2, i3, i4, is, ib3, ibs))
		return 1;
	if (rule_break())
		return 1;
	if (rule_return(&rv, i))
		return 1;
	expr(&rv);

	//Code generation begin
	if (typeArgSize(&rv.type))
		addInstrI(O_DROP, typeArgSize(&rv.type));
	//Code generation end

	if (consume_token(SEMICOLON))
		return 1;
	return 0;
}

int stm_compound() {
	Token *startTk = crntTk;
	Symbol *start = symbols.end[-1];

	if (consume_token(LACC)) {
		crtDepth++;
		while (1) {
			if (decl_var()) continue;
			if (stm()) continue;
			break;
		}
		if (consume_token(RACC)) {
			crtDepth--;
			deleteSymbolsAfter(&symbols, start);
			return 1;
		}
		else tkerr("Missing '}' in compound statement");
	}
	crntTk = startTk;
	return 0;
}

int expr(RetVal *rv) {
	if(expr_assign(rv)) return 1;
	return 0;
}

int expr_assign(RetVal *rv) {
	Token *startTk = crntTk;
	RetVal rve;

	//Code generation
	Instr *i;

	if (consume_token(SUB) || consume_token(NOT)) {
		if (expr_unary(rv)) {
			if (consume_token(ASSIGN)) {
				if (expr_assign(&rve)) {
					if (!rv->isLVal)
						tkerr("cannot assign to a non-lval");
					if (rv->type.nElements>-1 || rve.type.nElements>-1)
						tkerr("the arrays cannot be assigned");
					cast(&(rv->type), &(rve.type));

					//Code generation begin
					i = getRVal(&rve);
					addCastInstr(i, &rve.type, &rv->type);
					//duplicate the value on top before the dst addr
					addInstrII(O_INSERT,
						sizeof(void*) + typeArgSize(&rv->type),
						typeArgSize(&rv->type));
					addInstrI(O_STORE, typeArgSize(&rv->type));
					//Code generation end

					rv->isCtVal = rv->isLVal = 0;
				}
				tkerr("Missing assign expression");
			}else
			   tkerr("Missing '=' in assign expression");
		}
		return 0;
	}
	if (expr_postfix(rv)) {
		if (consume_token(ASSIGN)) {
			if (expr_assign(&rve)) {
				if (!rv->isLVal)
					tkerr("cannot assign to a non-lval");
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr("the arrays cannot be assigned");
				cast(&(rv->type), &(rve.type));
				rv->isCtVal = rv->isLVal = 0;
				return 1;
			}
			tkerr("Missing assign expression");
		}
	}
	crntTk = startTk;
	/*if (expr_unary()) {
		if (consume_token(ASSIGN)) {
			if (expr_assign()) {
				return 1;
			}
			tkerr("Missing assign expression");
		}else
			tkerr("Missing '=' in assign expression");
	}*/
	if (expr_or(rv))
		return 1;
	crntTk = startTk;
	return 0;
}

int expr_or1(RetVal *rv) {
	Token *startTk = crntTk;
	RetVal rve;

	//Code generation
	Instr *i1, *i2; Type t, t1, t2;

	if (consume_token(OR)) {

		//Code generation begin
		i1 = rv->type.nElements<0 ? getRVal(rv) : lastInstruction;
		t1 = rv->type;
		//Code generation end

		if (expr_and(&rve)) {
			if(expr_or1(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");

				//Code generation begin
				if (rv->type.nElements >= 0)      // vectors
					addInstr(O_OR_A);
				else {	// non-vectors
					i2 = getRVal(&rve); t2 = rve.type;
					t = getArithType(&t1, &t2);
					addCastInstr(i1, &t1, &t);
					addCastInstr(i2, &t2, &t);
					switch (t.typeBase) {
					case TB_INT:addInstr(O_OR_I); break;
					case TB_DOUBLE:addInstr(O_OR_D); break;
					case TB_CHAR:addInstr(O_OR_C); break;
					}
				}
				//Code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}else
			tkerr("Missing expression after '||'");
	}
	crntTk = startTk;
	return 1;
}

int expr_or(RetVal *rv) {
	if (expr_and(rv)) {
		if (expr_or1(rv))
			return 1;
	}
	return 0;
}

int expr_and1(RetVal *rv) {
	Token *startTk = crntTk;
	RetVal rve;

	//Code generation
	Instr *i1, *i2; Type t, t1, t2;

	if (consume_token(AND)) {

		//Code generation begin
		i1 = rv->type.nElements<0 ? getRVal(rv) : lastInstruction;
		t1 = rv->type;
		//Code generation end

		if (expr_eq(&rve)) {
			if (expr_and1(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");

				//Code generation begin
				if (rv->type.nElements >= 0)      // vectors
					addInstr(O_AND_A);
				else {  // non-vectors
					i2 = getRVal(&rve); t2 = rve.type;
					t = getArithType(&t1, &t2);
					addCastInstr(i1, &t1, &t);
					addCastInstr(i2, &t2, &t);
					switch (t.typeBase) {
						case TB_INT:addInstr(O_AND_I); break;
						case TB_DOUBLE:addInstr(O_AND_D); break;
						case TB_CHAR:addInstr(O_AND_C); break;
					}
				}
				//code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '&&'");
	}
	crntTk = startTk;
	return 1;
}

int expr_and(RetVal *rv) {
	if (expr_eq(rv)) {
		if (expr_and1(rv))
			return 1;
	}
	return 0;
}

int expr_eq1(RetVal *rv) {
	Token *startTk = crntTk;
	RetVal rve;

	//Code generation begin
	Instr *i1, *i2; Type t, t1, t2;
	i1 = rv->type.nElements<0 ? getRVal(rv) : lastInstruction;
	t1 = rv->type;
	//Code generation end

	if (consume_token(EQUAL)) {
		if (expr_rel(&rve)) {
			if (expr_eq1(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");

				//Code generation end
				if (rv->type.nElements >= 0) {      // vectors
					addInstr(O_EQ_A);
				}
				else {  // non-vectors
					i2 = getRVal(&rve); t2 = rve.type;
					t = getArithType(&t1, &t2);
					addCastInstr(i1, &t1, &t);
					addCastInstr(i2, &t2, &t);
					switch (t.typeBase) {
						case TB_INT:addInstr(O_EQ_I); break;
						case TB_DOUBLE:addInstr(O_EQ_D); break;
						case TB_CHAR:addInstr(O_EQ_C); break;
					}
				}
				//Code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '=='");
	}
	if (consume_token(NOTEQ)) {
		if (expr_rel(&rve)) {
			if (expr_eq1(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");

				//Code generation begin
				if (rv->type.nElements >= 0) {      // vectors
					addInstr(O_NOTEQ_A);
				}
				else {  // non-vectors
					i2 = getRVal(&rve); t2 = rve.type;
					t = getArithType(&t1, &t2);
					addCastInstr(i1, &t1, &t);
					addCastInstr(i2, &t2, &t);
					switch (t.typeBase) {
						case TB_INT:addInstr(O_NOTEQ_I); break;
						case TB_DOUBLE:addInstr(O_NOTEQ_D); break;
						case TB_CHAR:addInstr(O_NOTEQ_C); break;

					}
				}
				//Code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '!='");
	}
	crntTk = startTk;
	return 1;
}

int expr_eq(RetVal *rv) {
	if (expr_rel(rv)) {
		if (expr_eq1(rv))
			return 1;
	}
	return 0;
}

int expr_rel1(RetVal *rv) {
	Token *startTk = crntTk;
	Token *tkop;
	RetVal rve;

	//Code generation begin
	Instr *i1, *i2;
	Type t, t1, t2;
	i1 = getRVal(rv);
	t1 = rv->type;
	//Code generation end

	if (consume_token(LESS)) {
		tkop = consumedTk;
		if (expr_add(&rve)) {
			if (expr_rel1(rv)) {
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr("an array cannot be compared");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				t = getArithType(&t1, &t2);
				addCastInstr(i1, &t1, &t);
				addCastInstr(i2, &t2, &t);
				switch (t.typeBase) {
					case TB_INT:addInstr(O_LESS_I); break;
					case TB_DOUBLE:addInstr(O_LESS_D); break;
					case TB_CHAR:addInstr(O_LESS_C); break;
				}
				//Code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '<'");
	}
	if (consume_token(LESSEQ)) {
		tkop = consumedTk;
		if (expr_add(&rve)) {
			if (expr_rel1(rv)) {
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr("an array cannot be compared");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				t = getArithType(&t1, &t2);
				addCastInstr(i1, &t1, &t);
				addCastInstr(i2, &t2, &t);
				switch (t.typeBase) {
					case TB_INT:addInstr(O_LESSEQ_I); break;
					case TB_DOUBLE:addInstr(O_LESSEQ_D); break;
					case TB_CHAR:addInstr(O_LESSEQ_C); break;
				}
				//Code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '<='");
	}
	if (consume_token(GREATER)) {
		tkop = consumedTk;
		if (expr_add(&rve)) {
			if (expr_rel1(rv)) {
				if (rv->type.nElements > -1 || rve.type.nElements > -1)
					tkerr("an array cannot be compared");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				t = getArithType(&t1, &t2);
				addCastInstr(i1, &t1, &t);
				addCastInstr(i2, &t2, &t);
				switch (t.typeBase) {
					case TB_INT:addInstr(O_GREATER_I); break;
					case TB_DOUBLE:addInstr(O_GREATER_D); break;
					case TB_CHAR:addInstr(O_GREATER_C); break;
				}
				//Code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '>'");
	}
	if (consume_token(GREATEREQ)) {
		tkop = consumedTk;
		if (expr_add(&rve)) {
			if (expr_rel1(rv)) {
				if (rv->type.nElements > -1 || rve.type.nElements > -1)
					tkerr("an array cannot be compared");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				t = getArithType(&t1, &t2);
				addCastInstr(i1, &t1, &t);
				addCastInstr(i2, &t2, &t);
				switch (t.typeBase) {
					case TB_INT:addInstr(O_GREATEREQ_I); break;
					case TB_DOUBLE:addInstr(O_GREATEREQ_D); break;
					case TB_CHAR:addInstr(O_GREATEREQ_C); break;
				}
				//Code generation end

				rv->type = createType(TB_INT, -1);
				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '>='");
	}
	crntTk = startTk;
	return 1;
}

int expr_rel(RetVal *rv) {
	if (expr_add(rv)) {
		if (expr_rel1(rv))
			return 1;
	}
	return 0;
}

int expr_add1(RetVal *rv) {
	Token *startTk = crntTk;
	RetVal rve;

	//Code generation begin
	Instr *i1, *i2;
	Type t1, t2;
	i1 = getRVal(rv);
	t1 = rv->type;
	//Code generation end

	if (consume_token(ADD)) {
		if (expr_mul(&rve)) {
			if (expr_add1(rv)) {
				if (rv->type.nElements > -1 || rve.type.nElements > -1)
					tkerr("an array cannot be added");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be added");
				rv->type = getArithType(&rv->type, &rve.type);

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				addCastInstr(i1, &t1, &rv->type);
				addCastInstr(i2, &t2, &rv->type);
				switch (rv->type.typeBase) {
					case TB_INT:addInstr(O_ADD_I); break;
					case TB_DOUBLE:addInstr(O_ADD_D); break;
					case TB_CHAR:addInstr(O_ADD_C); break;
				}
				//Code generation end

				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '+'");
	}
	if (consume_token(SUB)) {
		if (expr_mul(&rve)) {
			if (expr_add1(rv)) {
				if (rv->type.nElements > -1 || rve.type.nElements > -1)
					tkerr("an array cannot be subtracted");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be subtracted");
				rv->type = getArithType(&rv->type, &rve.type);

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				addCastInstr(i1, &t1, &rv->type);
				addCastInstr(i2, &t2, &rv->type);
				switch (rv->type.typeBase) {
					case TB_INT:addInstr(O_SUB_I); break;
					case TB_DOUBLE:addInstr(O_SUB_D); break;
					case TB_CHAR:addInstr(O_SUB_C); break;
				}
				//Code generation end

				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '-'");
	}
	crntTk = startTk;
	return 1;
}

int expr_add(RetVal *rv) {
	if (expr_mul(rv)) {
		if (expr_add1(rv))
			return 1;
	}
	return 0;
}

int expr_mul1(RetVal *rv) {
	Token *startTk = crntTk;
	RetVal rve;

	//Code generation begin
	Instr *i1, *i2;
	Type t1, t2;
	i1 = getRVal(rv);
	t1 = rv->type;
	//Code generation end

	if (consume_token(MUL)) {
		if (expr_cast(&rve)) {
			if(expr_mul1(rv)){
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr("an array cannot be multiplied");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be multiplied");
				rv->type = getArithType(&rv->type, &rve.type);

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				addCastInstr(i1, &t1, &rv->type);
				addCastInstr(i2, &t2, &rv->type);
				switch (rv->type.typeBase) {
					case TB_INT:addInstr(O_MUL_I); break;
					case TB_DOUBLE:addInstr(O_MUL_D); break;
					case TB_CHAR:addInstr(O_MUL_C); break;
				}
				//Code generation end

				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '*'");
	}
	if (consume_token(DIV)) {
		if (expr_cast(&rve)) {
			if (expr_mul1(rv)) {
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr("an array cannot be divided");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be divided");
				rv->type = getArithType(&rv->type, &rve.type);

				//Code generation begin
				i2 = getRVal(&rve); t2 = rve.type;
				addCastInstr(i1, &t1, &rv->type);
				addCastInstr(i2, &t2, &rv->type);
				switch (rv->type.typeBase) {
					case TB_INT:addInstr(O_DIV_I); break;
					case TB_DOUBLE:addInstr(O_DIV_D); break;
					case TB_CHAR:addInstr(O_DIV_C); break;
				}
				//Code generation end

				rv->isCtVal = rv->isLVal = 0;
			}
			return 1;
		}
		tkerr("Missing expression after '/'");
	}
	crntTk = startTk;
	return 1;
}

int expr_mul(RetVal *rv) {
	if (expr_cast(rv)) {
		if (expr_mul1(rv))
			return 1;
	}
	return 0;
}

int expr_cast(RetVal *rv) {
	Token *startTk = crntTk;
	Type type;
	RetVal rve;
	if (consume_token(LPAR)) {
		if (type_name(&type)) {
			if (consume_token(RPAR)) {
				if (expr_cast(&rve)) {
					cast(&type, &rve.type);

					//Code generation begin
					if (rv->type.nElements<0 && rv->type.typeBase != TB_STRUCT) {
						switch (rve.type.typeBase) {
						case TB_CHAR:
							switch (type.typeBase) {
							case TB_INT:addInstr(O_CAST_C_I); break;
							case TB_DOUBLE:addInstr(O_CAST_C_D); break;
							}
							break;
						case TB_DOUBLE:
							switch (type.typeBase) {
							case TB_CHAR:addInstr(O_CAST_D_C); break;
							case TB_INT:addInstr(O_CAST_D_I); break;
							}
							break;
						case TB_INT:
							switch (type.typeBase) {
							case TB_CHAR:addInstr(O_CAST_I_C); break;
							case TB_DOUBLE:addInstr(O_CAST_I_D); break;
							}
							break;
						}
					}
					//Code generation end

					rv->type = type;
					rv->isCtVal = rv->isLVal = 0;
					return 1;
				}
				tkerr("Missing expression in cast");
			}else
				tkerr("Missing ')' at expression cast");
		}else
			tkerr("Missing type name in cast");
	}
	if (expr_unary(rv))
		return 1;
	crntTk = startTk;
	return 0;
}

int expr_unary(RetVal *rv) {
	Token *startTk = crntTk;
	Token * tkop;
	if (consume_token(SUB)) {
		tkop = consumedTk;
		if (expr_unary(rv)) {
			if (rv->type.nElements >= 0)
				tkerr("unary '-' cannot be applied to an array");
			if (rv->type.typeBase == TB_STRUCT)
				tkerr("unary '-' cannot be applied to a struct");

			//Code generation begin
			getRVal(rv);
			switch (rv->type.typeBase) {
				case TB_CHAR:addInstr(O_NEG_C); break;
				case TB_INT:addInstr(O_NEG_I); break;
				case TB_DOUBLE:addInstr(O_NEG_D); break;
			}
			//Code generation end

			rv->isCtVal = rv->isLVal = 0;
			return 1;
		}
		tkerr("Missing unary expression");
	}
	if (consume_token(NOT)) {
		tkop = consumedTk;
		if (expr_unary(rv)) {
			if (rv->type.typeBase == TB_STRUCT)
				tkerr("'!' cannot be applied to a struct");

			//Code generation begin
			if (rv->type.nElements<0) {
				getRVal(rv);
				switch (rv->type.typeBase) {
					case TB_CHAR:addInstr(O_NOT_C); break;
					case TB_INT:addInstr(O_NOT_I); break;
					case TB_DOUBLE:addInstr(O_NOT_D); break;
				}
			}
			else
				addInstr(O_NOT_A);
			//Code generation end

			rv->type = createType(TB_INT, -1);
			rv->isCtVal = rv->isLVal = 0;
			return 1;
		}
		tkerr("Missing unary expression");
	}
	if (expr_postfix(rv))
		return 1;
	crntTk = startTk;
	return 0;
}

int expr_postfix1(RetVal *rv){
	Token *startTk = crntTk;
	RetVal rve;
	if (consume_token(LBRACKET)) {
		if (expr(&rve)) {
			if (consume_token(RBRACKET)) {
				if (expr_postfix1(rv)) {
					if (rv->type.nElements<0)
						tkerr("only an array can be indexed");
					Type typeInt = createType(TB_INT, -1);
					cast(&typeInt, &rve.type);
					rv->type = rv->type;
					rv->type.nElements = -1;
					rv->isLVal = 1;
					rv->isCtVal = 0;
					return 1;
					//tkerr("Missing postfix expression");

					//Code generation begin
					addCastInstr(lastInstruction, &rve.type, &typeInt);
					getRVal(&rve);
					if (typeBaseSize(&rv->type) != 1) {
						addInstrI(O_PUSHCT_I, typeBaseSize(&rv->type));
						addInstr(O_MUL_I);
					}
					addInstr(O_OFFSET);
					//code generation end

				}
			}else
				tkerr("Missing ']' in postfix expression");
		}else
			tkerr("Missing expression in postfix expression");
	}
	if (consume_token(DOT)) {
		Token *tk_name = crntTk;
		if (consume_token(ID)) {
			if (expr_postfix1(rv)) {
				Symbol *sStruct = rv->type.s;
				Symbol *sMember = findSymbol(&sStruct->members, tk_name->text);
				if (!sMember)
					tkerr("struct %s does not have a member %s", sStruct->name, tk_name->text);
				rv->type = sMember->type;
				rv->isLVal = 1;
				rv->isCtVal = 0;
				return 1;
				//tkerr("Missing postfix expression");

				//Code generation begin
				if (sMember->offset) {
					addInstrI(O_PUSHCT_I, sMember->offset);
					addInstr(O_OFFSET);
				}
				//code generation end
			}
		}else
			tkerr("Missing ID in postfix expression");
	}
	crntTk = startTk;
	return 1;
}

int expr_postfix(RetVal *rv) {
	if (expr_primary(rv)) {
		if (expr_postfix1(rv)) {
			//printf("Postfix expression\n");
			return 1;
		}
	}
	return 0;
}

int expr_primary(RetVal *rv) {
	Token *startTk = crntTk;
	Token *tk_name = crntTk;
	RetVal arg;

	//Code generation
	Instr *i;

	if (consume_token(ID)) {
		Symbol *s = findSymbol(&symbols, tk_name->text);
		if (!s)
			tkerr("undefined symbol %s", tk_name->text);
		rv->type = s->type;
		rv->isCtVal = 0;
		rv->isLVal = 1;
		if (consume_token(LPAR)) {
			Symbol **crtDefArg = s->args.begin;
			if (s->cls != CLS_FUNC && s->cls != CLS_EXTFUNC)
				tkerr("call of the non-function %s", tk_name->text);
			if (expr(&arg)) {
				if (crtDefArg == s->args.end)
					tkerr("too many arguments in call");
				cast(&(*crtDefArg)->type, &arg.type);

				//Code generation begin
				if ((*crtDefArg)->type.nElements<0)   //only arrays are passed by addr
					i = getRVal(&arg);
				else
					i = lastInstruction;
				addCastInstr(i, &arg.type, &(*crtDefArg)->type);
				//Code generation end

				crtDefArg++;
			}
			while (1) {
				if (consume_token(COMMA)) {
					if (expr(&arg)) {
						if (crtDefArg == s->args.end)
							tkerr("too many arguments in call");
						cast(&(*crtDefArg)->type, &arg.type);

						//Code generation begin
						if ((*crtDefArg)->type.nElements<0)
							i = getRVal(&arg);
						else
							i = lastInstruction;
						addCastInstr(i, &arg.type, &(*crtDefArg)->type);
						// Code generation end

						crtDefArg++;
					}
					else
						tkerr("Missing expression after ','");
				}
				break;
			}
			if (consume_token(RPAR)) {
				if (crtDefArg != s->args.end)
					tkerr("too few arguments in call");
				rv->type = s->type;
				rv->isCtVal = rv->isLVal = 0;

				//Code generation begin
				i = addInstr(s->cls == CLS_FUNC ? O_CALL : O_CALLEXT);
				i->args[0].addr = s->addr;
				//Code generation end

				//printf("Expression\n");
				return 1;
			}
			else {

				//Code generation begin
				if (s->depth)
					addInstrI(O_PUSHFPADDR, s->offset);
				else
					addInstrA(O_PUSHCT_A, s->addr);
				//Code generation end

				if (s->cls == CLS_FUNC || s->cls == CLS_EXTFUNC)
					tkerr("missing call for function %s", tk_name->text);
				tkerr("Missing ')' in expression");
			}
		}
		//printf("ID expression primary\n");
		return 1;
	}
	if (consume_token(CT_INT)) {
		Token *tki = consumedTk;
		rv->type = createType(TB_INT, -1);
		rv->ctVal.i = tki->real_integer;
		rv->isCtVal = 1;
		rv->isLVal = 0;

		//Code generation begin
		i = addInstrI(O_PUSHCT_I, tki->real_integer);

		//printf("Int expression\n");
		return 1;
	}
	if (consume_token(CT_REAL)) {
		Token *tkr = consumedTk;
		rv->type = createType(TB_DOUBLE, -1);
		rv->ctVal.d = tkr->real_double;
		rv->isCtVal = 1;
		rv->isLVal = 0;

		//Code generation begin
		i = addInstr(O_PUSHCT_D);
		i->args[0].d = tkr->real_double;
		//Code generation end

		//printf("Real expression\n");
		return 1;
	}
	if (consume_token(CT_CHAR)) {
		Token *tkc = consumedTk;
		rv->type = createType(TB_CHAR, -1);
		rv->ctVal.i = tkc->character;
		rv->isCtVal = 1;
		rv->isLVal = 0;

		//Code generation
		addInstrI(O_PUSHCT_C, tkc->character);

		//printf("Character expression\n");
		return 1;
	}
	if (consume_token(CT_STRING)) {
		Token *tks = consumedTk;
		rv->type = createType(TB_CHAR, 0);
		rv->ctVal.str = tks->text;
		rv->isCtVal = 1;
		rv->isLVal = 0;

		//Code generation
		addInstrA(O_PUSHCT_C, tks->text);

		//printf("String expression\n");
		return 1;
	}
	if (consume_token(LPAR)) {
		if (expr(rv)) {
			if (consume_token(RPAR)) {
				//printf("Expr primary\n");
				return 1;
			}
			tkerr("Missing ')' in expression");
		}else
			tkerr("Missing expression after '('");
	}
	crntTk = startTk;
	return 0;
}

int typebase(Type *ret) {
	Token *startTk = crntTk;
	if (consume_token(INT)) {
		//printf("Int type\n");
		ret->typeBase = TB_INT;
		return 1;
	}
	if (consume_token(DOUBLE)) {
		//printf("Double type\n");
		ret->typeBase = TB_DOUBLE;
		return 1;
	}
	if (consume_token(CHAR)) {
		//printf("Char type\n");
		ret->typeBase = TB_CHAR;
		return 1;
	}
	if (consume_token(STRUCT)) {
		Token *tk_name = crntTk;
		if (consume_token(ID)) {
			//printf("Struct type\n");
			Symbol *s = findSymbol(&symbols, tk_name->text);
			if (s == NULL) tkerr("undefined symbol: %s", tk_name->text);
			if (s->cls != CLS_STRUCT) tkerr("%s is not a struct", tk_name->text);
			ret->typeBase = TB_STRUCT;
			ret->s = s;
			return 1;
		}
		else tkerr("Missing struct ID");
	}
	crntTk = startTk;
	return 0;
}

int decl_struct() {
	Token *startTk = crntTk;
	if (consume_token(STRUCT)) {
		Token *tk_name = crntTk;
		if (consume_token(ID)) {
			if (consume_token(LACC)) {
				offset = 0;
				if (findSymbol(&symbols, tk_name->text))
					tkerr("symbol redefinition: %s", tk_name->text);
				crtStruct = addSymbol(&symbols, tk_name->text, CLS_STRUCT);
				initSymbols(&crtStruct->members);

				while (1) {
					if (decl_var())
						continue;
					break;

				}
				if (consume_token(RACC)) {
					if (consume_token(SEMICOLON)) {
						//printf("Structure declared\n");
						return 1;
					}
					else
						tkerr("Missing semicolon at struct declaration");
				}else
					tkerr("Missing '}' at structure declaration");
			}else
				tkerr("Missing '{' at structure declaration");
		}else
			tkerr("Missing structure ID at structure declaration");
	}
	crntTk = startTk;
	crtStruct = NULL;
	return 0;
}

int decl_var() {
	Token *startTk = crntTk;
	Type type;

	if (typebase(&type)) {
		Token *tk_name = crntTk;
		if (consume_token(ID)) {
			if (!array_decl(&type))
				type.nElements = -1;
			addVar(tk_name, &type);
			while (1) {
				if (consume_token(COMMA)) {
					Token *tk_name1 = crntTk;
					if (consume_token(ID)) {
						if(!array_decl(&type))
							type.nElements = -1;
						addVar(tk_name1, &type);
						continue;
					}else
						tkerr("Missing ID after comma at variable declaration");
				}
				break;
			}
			if (consume_token(SEMICOLON)) {
				//printf("Variable declared\n");
				return 1;
			}
			else
				tkerr("Missing ';' at the end of the variable declaration");
		}else
			tkerr("Missing ID at variable declaration");
	}
	crntTk = startTk;
	return 0;
}

/*arrayDecl(out ret:Type):
LBRACKET
( expr
{
ret->nElements=0;       // for now do not compute the real size
}
)? RBRACKET ;
*/

int array_decl(Type *ret) {
	Token *startTk = crntTk;
	RetVal rv;
	Instr *instrBeforeExpr;
	if (consume_token(LBRACKET)) {

		//Code generation
		instrBeforeExpr = lastInstruction;

		if (expr(&rv)) {
			if (!rv.isCtVal)
				tkerr("the array size is not a constant");
			if (rv.type.typeBase != TB_INT)
				tkerr("the array size is not an integer");
			ret->nElements = rv.ctVal.i;

			//Code generation
			deleteInstructionsAfter(instrBeforeExpr);
		}
		else
			ret->nElements = 0;
		if (consume_token(RBRACKET)) {
			//printf("Array declared\n");
			return 1;
		}
		tkerr("Missing ']' at array declaration");
	}
	crntTk = startTk;
	return 0;
}

int type_name(Type *ret) {
	if (typebase(ret)) {
		if(!array_decl(ret))
			ret->nElements = -1;
		//printf("Type name\n");
		return 1;
	}
	return 0;
}


//------------------------------
//SYNTACTIC ANALYZER END



int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Not enough arguments on the command line\n");
		exit(-1);
	}
	FILE* filep = fopen(argv[1], "r");
	if (filep == NULL) {
		printf("Error on opening file\n");
		exit(-2);
	}
	int c;
	tokens = (Token*)malloc(1000 * sizeof(Token));
	while ((c = fgetc(filep)) != EOF) {
		//printf("%d ",getNextToken(filep, c));
		getNextToken(filep, c);
	}
	return 0;
}
