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
				if (c == EOF)
					if (unit()) {  //go to syntax alaysis
						printf("Lexical and syntactical analysis OK!\n");
						exit(0);
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
	if (ch == EOF) 
		if (unit()) {  //go to syntax alaysis
			printf("Lexical and syntactical analysis OK!\n");
			exit(0);
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
		if (n == 0)n = 1;								// needed for the initial case
		symbols->begin = (Symbol**)realloc(symbols->begin, n * sizeof(Symbol*));
		if (symbols->begin == NULL)err("not enough memory");
		symbols->end = symbols->begin + count;
		symbols->after = symbols->begin + n;
	}
	SAFEALLOC(s, Symbol)
		*symbols->end++ = s;
	s->name = name;
	s->cls = cls;
	s->depth = crtDepth;
	return s;
}

Symbol *findSymbol(Symbols *symbols, const char *name) {
	Symbol **s;
	for (s = (symbols->end)--; s != symbols->begin; s--)
		if (strcmp((*s)->name, name) == 0)
			return *s;
	if (strcmp((*s)->name, name) == 0)
		return *s;
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

/*factor(out ret:double) ::= 
	REAL:tk 
	{
		ret=tk->realVal;
	} 
	| LPAR expr:ret RPAR*/
/*int factor(double *ret)
{
	if(consume(REAL)){
		Token *tk=consumedTk;
		*ret=tk->realVal;
	}
	else if(consume(LPAR)){
	if(!expr(ret))tkerr(crtTk,"invalid expression after (");
	if(!consume(RPAR))tkerr(crtTk,"missing )");
}
else return 0;
return 1;
}*/

//------------------------------
//SYMBOLS TABLE END




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
int expr();
int expr_assign();
int expr_or();
int expr_and();
int expr_eq();
int expr_rel();
int expr_add();
int expr_mul();
int expr_cast();
int expr_unary();
int expr_postfix();
int expr_primary();
int rule_if();
int rule_while();
int rule_for();
int rule_break();
int rule_return();


int unit() {
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

int rule_while()
{
	Token *startTk = crntTk;
	if (consume_token(WHILE)) {
		if (consume_token(LPAR)) {
			if (expr()) {
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
int rule_if() {
	Token *startTk = crntTk;
	if (consume_token(IF)) {
		if (consume_token(LPAR)) {
			if (expr()) {
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
	Token *startTk = crntTk;
	if (consume_token(FOR)) {
		if (consume_token(LPAR)) {
			expr();
			if (consume_token(SEMICOLON)) {
				expr();
				if (consume_token(SEMICOLON)) {
					expr();
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

int rule_return() {
	Token *startTk = crntTk;
	if (consume_token(RETURN)) {
		expr();
		if (consume_token(SEMICOLON))
			return 1;
		else
			tkerr("Missing ';' after return");
	}
	crntTk = startTk;
	return 0;
}

int stm() {
	if (stm_compound()) 
		return 1;
	if (rule_if()) 
		return 1;
	if (rule_while()) 
		return 1;
	if (rule_for()) 
		return 1;
	if (rule_break()) 
		return 1;
	if (rule_return()) 
		return 1;
	expr();
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

int expr() {
	if(expr_assign()) return 1;
	return 0;
}

int expr_assign() {
	Token *startTk = crntTk;
	if (consume_token(SUB) || consume_token(NOT)) {
		if (expr_unary()) {
			if (consume_token(ASSIGN)) {
				if (expr_assign()) {
					return 1;
				}
				tkerr("Missing assign expression");
			}else
			   tkerr("Missing '=' in assign expression");
		}
		return 0;
	}
	if (expr_postfix()) {
		if (consume_token(ASSIGN)) {
			if (expr_assign()) {
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
	if (expr_or()) 
		return 1;
	crntTk = startTk;
	return 0;
}

int expr_or1() {
	Token *startTk = crntTk;
	if (consume_token(OR)) {
		if (expr_and()) {
			expr_or1();
			return 1;
		}else
			tkerr("Missing expression after '||'");
	}
	crntTk = startTk;
	return 1;
}

int expr_or() {
	if (expr_and()) {
		if (expr_or1())
			return 1;
	}
	return 0;
}

int expr_and1() {
	Token *startTk = crntTk;
	if (consume_token(AND)) {
		if (expr_eq()) {
			expr_and1();
			return 1;
		}
		tkerr("Missing expression after '&&'");
	}
	crntTk = startTk;
	return 1;
}

int expr_and() {
	if (expr_eq()) {
		if (expr_and1())
			return 1;
	}
	return 0;
}

int expr_eq1() {
	Token *startTk = crntTk;
	if (consume_token(EQUAL)) {
		if (expr_rel()) {
			expr_eq1();
			return 1;
		}
		tkerr("Missing expression after '=='");
	}
	if (consume_token(NOTEQ)) {
		if (expr_rel()) {
			expr_eq1();
			return 1;
		}
		tkerr("Missing expression after '=='");
	}
	crntTk = startTk;
	return 1;
}

int expr_eq() {
	if (expr_rel()) {
		if (expr_eq1())
			return 1;
	}
	return 0;
}

int expr_rel1() {
	Token *startTk = crntTk;
	if (consume_token(LESS)) {
		if (expr_add()) {
			expr_rel1();
			return 1;
		}
		tkerr("Missing expression after '<'");
	}
	if (consume_token(LESSEQ)) {
		if (expr_add()) {
			expr_rel1();
			return 1;
		}
		tkerr("Missing expression after '<='");
	}
	if (consume_token(GREATER)) {
		if (expr_add()) {
			expr_rel1();
			return 1;
		}
		tkerr("Missing expression after '>'");
	}
	if (consume_token(GREATEREQ)) {
		if (expr_add()) {
			expr_rel1();
			return 1;
		}
		tkerr("Missing expression after '>='");
	}
	crntTk = startTk;
	return 1;
}

int expr_rel() {
	if (expr_add()) {
		if (expr_rel1())
			return 1;
	}
	return 0;
}

int expr_add1() {
	Token *startTk = crntTk;
	if (consume_token(ADD)) {
		if (expr_mul()) {
			expr_add1();
			return 1;
		}
		tkerr("Missing expression after '+'");
	}
	if (consume_token(SUB)) {
		if (expr_mul()) {
			expr_add1();
			return 1;
		}
		tkerr("Missing expression after '-'");
	}
	crntTk = startTk;
	return 1;
}

int expr_add() {
	if (expr_mul()) {
		if (expr_add1())
			return 1;
	}
	return 0;
}

int expr_mul1() {
	Token *startTk = crntTk;
	if (consume_token(MUL)) {
		if (expr_cast()) {
			expr_mul1();
			return 1;
		}
		tkerr("Missing expression after '*'");
	}
	if (consume_token(DIV)) {
		if (expr_cast()) {
			expr_mul1();
			return 1;
		}
		tkerr("Missing expression after '/'");
	}
	crntTk = startTk;
	return 1;
}

int expr_mul() {
	if (expr_cast()) {
		if (expr_mul1())
			return 1;
	}
	return 0;
}

int expr_cast() {
	Token *startTk = crntTk;
	Type type;
	if (consume_token(LPAR)) {
		if (type_name(&type)) {
			if (consume_token(RPAR)) {
				if (expr_cast()) 
					return 1;
				tkerr("Missing expression in cast");
			}else
				tkerr("Missing ')' at expression cast");
		}else
			tkerr("Missing type name in cast");
	}
	if (expr_unary()) 
		return 1;
	crntTk = startTk;
	return 0;
}

int expr_unary() {
	Token *startTk = crntTk;
	if (consume_token(SUB)) {
		if (expr_unary()) 
			return 1;
		tkerr("Missing unary expression");
	}
	if (consume_token(NOT)) {
		if (expr_unary()) 
			return 1;
		tkerr("Missing unary expression");
	}
	if (expr_postfix()) 
		return 1;
	crntTk = startTk;
	return 0;
}

int expr_postfix1() {
	Token *startTk = crntTk;
	if (consume_token(LBRACKET)) {
		if (expr()) {
			if (consume_token(RBRACKET)) {
				if (expr_postfix1()) 
					return 1;
				//tkerr("Missing postfix expression");
			}else
				tkerr("Missing ']' in postfix expression");
		}else
			tkerr("Missing expression in postfix expression");
	}
	if (consume_token(DOT)) {
		if (consume_token(ID)) {
			if(expr_postfix1()) 
				return 1;
			//tkerr("Missing postfix expression");
		}else
			tkerr("Missing ID in postfix expression");
	}
	crntTk = startTk;
	return 1;
}

int expr_postfix() {
	if (expr_primary()) {
		if (expr_postfix1()) {
			//printf("Postfix expression\n");
			return 1;
		}
	}
	return 0;
}

int expr_primary() {
	Token *startTk = crntTk;
	if (consume_token(ID)) {
		if (consume_token(LPAR)) {
			if (expr()) {
				while (1) {
					if (consume_token(COMMA)) {
						if (expr()) 
							continue;
						else 
							tkerr("Missing expression after ','");
					}
					break;
				}
			}
			if (consume_token(RPAR)) {
				//printf("Expression\n");
				return 1;
			}
			else
				tkerr("Missing ')' in expression");
		}
		//printf("ID expression primary\n");
		return 1;
	}
	if (consume_token(CT_INT)) {
		//printf("Int expression\n");
		return 1;
	}
	if (consume_token(CT_REAL)) {
		//printf("Real expression\n");
		return 1;
	}
	if (consume_token(CT_CHAR)) {
		//printf("Character expression\n");
		return 1;
	}
	if (consume_token(CT_STRING)) {
		//printf("String expression\n");
		return 1;
	}
	if (consume_token(LPAR)) {
		if (expr()) {
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
	if (consume_token(LBRACKET)) {
		expr();
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

