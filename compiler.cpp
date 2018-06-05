#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<ctype.h>
#include<string.h>


#pragma warning(disable : 4996)  
#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");


//Syntax analyzer main funtion definition
int unit();


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
}

//------------------------------
//SYMBOLS TABLE END



//TYPE ANALYSIS BEGIN
//------------------------------

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

Symbol *addExtFunc(const char *name, Type type)
{
	Symbol *s = addSymbol(&symbols, name, CLS_EXTFUNC);
	s->type = type;
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

	symbol = addExtFunc("put_s", createType(TB_VOID, -1));
	args = addFuncArg(symbol, "s", createType(TB_CHAR, 0));	symbol = addExtFunc("get_s", createType(TB_VOID, -1));
	args = addFuncArg(symbol, "s", createType(TB_CHAR, 0));	symbol = addExtFunc("put_i", createType(TB_VOID, -1));
	args = addFuncArg(symbol, "i", createType(TB_INT, -1));	symbol = addExtFunc("get_i", createType(TB_INT, -1));	symbol = addExtFunc("put_d", createType(TB_VOID, -1));
	args = addFuncArg(symbol, "d", createType(TB_DOUBLE, -1));	symbol = addExtFunc("get_d", createType(TB_DOUBLE, -1));	symbol = addExtFunc("put_c", createType(TB_VOID, -1));
	args = addFuncArg(symbol, "c", createType(TB_CHAR, -1));	symbol = addExtFunc("get_c", createType(TB_CHAR, -1));	symbol = addExtFunc("seconds", createType(TB_DOUBLE, -1));
}

//------------------------------
//TYPE ANALYSIS END


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
int rule_if(RetVal *rv);
int rule_while(RetVal *rv);
int rule_for();
int rule_break();
int rule_return(RetVal *rv);


int unit() {
	initSymbols(&symbols);
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
			s = addSymbol(&crtFunc->args, tk_name->text, CLS_VAR);
			s->mem = MEM_ARG;
			s->type = type;
			return 1;		
		}else
			tkerr("Missing function argument ID");
	}
	crntTk = startTk;
	return 0;
}

int function_body(Token *tk_name, Type type) {
	Token *startTk = crntTk;
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
			crtDepth--;
			if (stm_compound()) {
				deleteSymbolsAfter(&symbols, crtFunc);
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
		if (function_body(tk_name,type)){
			return 1;
		}
		tkerr("Error on declarig");
	}
	crntTk = startTk;
	return 0;
}

int rule_while(RetVal *rv){
	Token *startTk = crntTk;
	if (consume_token(WHILE)) {
		if (consume_token(LPAR)) {
			if (expr(rv)) {
				if (rv->type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");
				if (consume_token(RPAR)) {
					if (stm()) {
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
}int rule_break() {	Token *startTk = crntTk;	if (consume_token(BREAK)) {		if (consume_token(SEMICOLON))			return 1;		else tkerr("Missing semicolon after break");	}	crntTk = startTk;
	return 0;}
int rule_if(RetVal *rv) {
	Token *startTk = crntTk;
	if (consume_token(IF)) {
		if (consume_token(LPAR)) {
			if (expr(rv)) {
				if (rv->type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");
				if (consume_token(RPAR)) {
					if (stm()) {
						if (consume_token(ELSE)) {
							if (stm())
								return 1;
							else tkerr("Missing else statement");
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

int rule_for() {
	RetVal rv1, rv2, rv3;
	Token *startTk = crntTk;
	if (consume_token(FOR)) {
		if (consume_token(LPAR)) {
			expr(&rv1);
			if (consume_token(SEMICOLON)) {
				if (expr(&rv2)) {
					if (rv2.type.typeBase == TB_STRUCT)
						tkerr("a structure cannot be logically tested");
				}
				if (consume_token(SEMICOLON)) {
					expr(&rv3);
					if (consume_token(RPAR)) {
						if (stm())
							return 1;
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

int rule_return(RetVal *rv){
	Token *startTk = crntTk;
	if (consume_token(RETURN)) {
		if(expr(rv)){
			if (crtFunc->type.typeBase == TB_VOID)
				tkerr("a void function cannot return a value");
			cast(&crtFunc->type, &(rv->type));
		}
		if (consume_token(SEMICOLON))
			return 1;
		else
			tkerr("Missing ';' after return");
	}
	crntTk = startTk;
	return 0;
}

int stm() {
	RetVal rv;
	if (stm_compound()) 
		return 1;
	if (rule_if(&rv)) 
		return 1;
	if (rule_while(&rv)) 
		return 1;
	if (rule_for()) 
		return 1;
	if (rule_break())
		return 1;
	if (rule_return(&rv))
		return 1;
	expr(&rv);
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
	if (consume_token(SUB) || consume_token(NOT)) {
		if (expr_unary(rv)) {
			if (consume_token(ASSIGN)) {
				if (expr_assign(&rve)) {
					if (!rv->isLVal)
						tkerr("cannot assign to a non-lval");
					if (rv->type.nElements>-1 || rve.type.nElements>-1)
						tkerr("the arrays cannot be assigned");
					cast(&(rv->type), &(rve.type));
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
	if (consume_token(OR)) {
		if (expr_and(&rve)) {
			if(expr_or1(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");
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
	if (consume_token(AND)) {
		if (expr_eq(&rve)) {
			if (expr_and1(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be logically tested");
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
	if (consume_token(EQUAL)) {
		if (expr_rel(&rve)) {
			if (expr_eq1(rv)) {
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");
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
	if (consume_token(LESS)) {
		tkop = consumedTk;
		if (expr_add(&rve)) {
			if (expr_rel1(rv)) {
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr("an array cannot be compared");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be compared");
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
	if (consume_token(ADD)) {
		if (expr_mul(&rve)) {
			if (expr_add1(rv)) {
				if (rv->type.nElements > -1 || rve.type.nElements > -1)
					tkerr("an array cannot be added");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be added");
				rv->type = getArithType(&rv->type, &rve.type);
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
	if (consume_token(MUL)) {
		if (expr_cast(&rve)) {
			if(expr_mul1(rv)){
				if (rv->type.nElements>-1 || rve.type.nElements>-1)
					tkerr("an array cannot be multiplied");
				if (rv->type.typeBase == TB_STRUCT || rve.type.typeBase == TB_STRUCT)
					tkerr("a structure cannot be multiplied");
				rv->type = getArithType(&rv->type, &rve.type);
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
				crtDefArg++;
				while (1) {
					if (consume_token(COMMA)) {
						if (expr(&arg)) {
							if (crtDefArg == s->args.end)
								tkerr("too many arguments in call");
							cast(&(*crtDefArg)->type, &arg.type);
							crtDefArg++;
						}
						else 
							tkerr("Missing expression after ','");
					}
					break;
				}
			}
			if (consume_token(RPAR)) {
				if (crtDefArg != s->args.end)
					tkerr("too few arguments in call");
				rv->type = s->type;
				rv->isCtVal = rv->isLVal = 0;
				//printf("Expression\n");
				return 1;
			}
			else {
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
		//printf("Int expression\n");
		return 1;
	}
	if (consume_token(CT_REAL)) {
		Token *tkr = consumedTk;
		rv->type = createType(TB_DOUBLE, -1); 
		rv->ctVal.d = tkr->real_double;
		rv->isCtVal = 1; 
		rv->isLVal = 0;
		//printf("Real expression\n");
		return 1;
	}
	if (consume_token(CT_CHAR)) {
		Token *tkc = consumedTk;
		rv->type = createType(TB_CHAR, -1); 
		rv->ctVal.i = tkc->character;
		rv->isCtVal = 1; 
		rv->isLVal = 0;
		//printf("Character expression\n");
		return 1;
	}
	if (consume_token(CT_STRING)) {
		Token *tks = consumedTk;
		rv->type = createType(TB_CHAR, 0); 
		rv->ctVal.str = tks->text;
		rv->isCtVal = 1; 
		rv->isLVal = 0;
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
	if (consume_token(LBRACKET)) {
		if (expr(&rv)) {
			if (!rv.isCtVal)
				tkerr("the array size is not a constant");
			if (rv.type.typeBase != TB_INT)
				tkerr("the array size is not an integer");
			ret->nElements = rv.ctVal.i;
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

