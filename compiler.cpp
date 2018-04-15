// compiler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<ctype.h>
#include<string.h>
#pragma warning(disable : 4996)  
#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");



//LEXICAL ANALYZER BEGIN
//------------------------------

//token codes
enum {
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
					exit(0); //go to next step of compilation actually
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
		exit(0); //go to next step of compilation actually
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
}int rule_while()
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
						startTk = crntTk;
						if (consume_token(ELSE)) {
							if (stm())
								return 1;
							else tkerr("Missing else statement");
						}
						crntTk = startTk;
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

int stm() {
	if (stm_compound()) return 1;
	if (rule_if()) return 1;
	if (rule_while()) return 1;
	//if (rule_for()) return 1;
	if (rule_break()) return 1;
	//if (rule_return()) return 1;
	//expr();
	//Token *startTk = crntTk;
	//if (consume_token(SEMICOLON)) return 1;
	//crntTk = startTk;
	//return 0;
}

int stm_compound() {
	Token *startTk = crntTk;
	if (consume_token(LACC)) {
		while (1) {
			if (decl_var()) continue;
			if (stm()) continue;
			break;
		}
		if (consume_token(RACC))
			return 1;
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
	if (expr_unary()) {
		if (consume_token(ASSIGN)) {
			if (expr_assign()) {
				return 1;
			}
			if (expr_or()) {
				return 1;
			}
			return 0;
		}
	}
	crntTk = startTk;
	return 0;
}

int expr_or() {
	if (expr_and()) {
		if (expr_or1())
			return 1;
	}
	return 0;
}

int expr_or1() {
	Token *startTk = crntTk;
	if (consume_token(OR)) {
		if (expr_and()) {
			expr_or1();
			return 1;
		}
	}
	crntTk = startTk;
	return 0;
}

int expr_and() {
	if (expr_and()) {
		if (expr_or1())
			return 1;
	}
	return 0;
}

int expr_and1() {

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

