#ifndef _CalcInterp_H
#define _CalcInterp_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <map>

using namespace std;

enum EgrammarTokens {
	ENDF = 0, ADDOP = 1, MULTOP = 2, SUBOP = 3, DIVOP = 4, NUMCONST = 5, 
	ID = 6, LPAREN = 7, RPAREN = 8, ENDL = 9, ASIGN = 10, READSTMT = 11, 
	WRITESTMT = 12, ERR = 13
};

enum EparseResponse{
	eFALSE = 0, eTRUE = 1, eERR = 3
};

// Execution functions
bool CompileProgram();
string GetNextId();
void GetNextToken();
void CaseRead();
void CaseWrite();
bool ConfirmAsign();
void CaseId();
double GetStatement(double curVal);
double GetStatementFromId(double curVal);
double GetAsignmentStatement(double curVal);
double GetStatementFromHighOp(double curVal);
double GetStatementFromCache(double curVal);

#define CLTEXT_MAX 100
extern char cltext[CLTEXT_MAX];	// Global token text buffer
extern int lineNumber;

// Compiling functions
bool calclexopen(const char filename[]);
void cltextclear();
void cltextappend(int c);
int calclex();
int MakeCapital(int c);
bool check4var(int c);
EparseResponse check4numconst(int c);

bool CheckFor(char*, int c);
int Check4MathSymbols(int c);
int removeNonTokens();

string getStringFromToken(int tok);

#endif