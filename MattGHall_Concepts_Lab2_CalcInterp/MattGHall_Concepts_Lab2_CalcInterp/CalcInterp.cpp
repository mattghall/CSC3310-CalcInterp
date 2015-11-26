#pragma once
#include <string>
#include <iomanip> 
#include <map>
#include <queue>
#include "CalcInterp.h";
#include <stdlib.h> 

char cltext[CLTEXT_MAX];	// Global token text buffer

static int cltextpos = 0;
int lineNumber = 1;
ifstream fin;				// lex input file stream

// Map holding the variables
map<string, int> IdTable;

// There will always be a missed token if we don't cache it
int tokenCache = 0;

bool calclexopen(const char filename[])
{
	fin.open(filename, ios_base::in);

	return  fin.is_open();
}

void cltextclear()
{
	cltextpos = 0;
	cltext[cltextpos] = 0;
}

void cltextappend(int c)
{
	if (cltextpos >= CLTEXT_MAX - 1)
		return;		// ignore char
	cltext[cltextpos++] = (char)c;
	cltext[cltextpos] = 0;			// null sentinel at end
}

// ungets a certain number of fins
void ungetFin(int x)
{
	for (int i = 0; i < x; i++)
	{
		fin.unget();
	}
}


// Checks for a specific char*
bool CheckFor(char* token, int c)
{
	cltextclear();
	int length = strlen(token);
	for (int i = 0; i < length; i++)
	{
		c = MakeCapital(c);

		if (c != token[i] && c != token[i])
		{
			ungetFin(i);
			cltextclear();
			return false;
		}
		cltextappend(c);
		c = fin.get();
	}
	fin.unget();
	return true;
}

// If c is a lower case letter, it returns an uppercase version
int MakeCapital(int c)
{
	if (c >= 97 && c <= 122)
	{
		return c - 32;
	}

	return c;
}

EparseResponse check4numconst(int c)
{
	cltextclear();
	// Numerical consts must start with a number 0-9
	if (c >= '0' && c <= '9')
	{
		// loop continually while the input is a number or a decimal point
		while ((c >= '0' && c <= '9') || c == '.')
		{
			cltextappend(c);
			// if c is a decimal point, switch to a different thread because there
			// cannot be two decimal points
			if (c == '.')
			{
				c = fin.get();		// get the next digit

				// All other digits must be numbers for the rest of the numerical const
				while (c >= '0' && c <= '9')
				{
					cltextappend(c);
					c = fin.get();
				}
				// check to make sure there isn't a double decimal
				if (c == '.')
				{
					cltextappend(c);
					return eERR;
				}

				// c is not part of the numerical const, unget
				fin.unget();
				return eTRUE;
			}

			c = fin.get();
			//cltextappend(c);
		}

		fin.unget();		// do not consume char following token
		return eTRUE;
	}
	return eFALSE;
}

bool check4var(int c)
{
	c = MakeCapital(c);

	if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
	{
		cltextclear();
		while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '_'))
		{
			cltextappend(c);
			c = MakeCapital(fin.get());
		}

		fin.unget();		// do not consume char following token
		return true;
	}
	return false;
}

// Converts Token Enum to string
string getStringFromToken(int tok)
{
	switch (tok)
	{
	case 0: return "ENDF";
	case 1: return "ADDOP";
	case 2: return "MULTOP";
	case 3: return "SUBOP";
	case 4: return "DIVOP";
	case 5: return "NUMCONST";
	case 6: return "ID";
	case 7: return "LPAREN";
	case 8: return "RPAREN";
	case 9: return "ENDL";
	case 10: return "ASIGN";
	case 11: return "READSTMT";
	case 12: return "WRITESTMT";
	case 13: return "ERR";
	default: return "ERR";
	}
}

// Returns Mathematic ENUM or 0 as default
int Check4MathSymbols(int c)
{
	// Check for math expressions
	if (c == '+')	return ADDOP;
	if (c == '*')	return MULTOP;
	if (c == '/')	return DIVOP;
	if (c == '-')	return SUBOP;
	if (c == '(')	return LPAREN;
	if (c == ')')	return RPAREN;

	//return default as 0
	return 0;
}

// Recursive function that calls itself until there is a non-comment non-whitespace char
int removeNonTokens()
{
	//Remove Whitespace
	int c;
	// Skip whitespace
	while ((c = fin.get()) == ' ' || c == '\t' || c == '\n')
	{
		if (c == '\n') lineNumber++;
	}

	// Remove Comments
	if (c == '/')
	{
		// If the previous wasn't a comment, it must be a divide
		if (fin.get() != '*')
		{
			fin.unget();		// do not consume char following token
			return c;
		}
		else
		{
			// Skip over comment
			while ((c = fin.get()))
			{
				if (c == '*')
				{
					if ((c = fin.get()) == '/')
					{
						return(removeNonTokens());
					}
					// I fixed it so comments work :D
					fin.unget(); // do not consume char following token
				}

				if (c == '\n')
				{
					lineNumber++;
					return(removeNonTokens());
				}
			}
		}
	}
	return c;
}

int calclex()
{
	int c;

	while (true)
	{
		cltextclear();
		c = removeNonTokens();

		if (c == EOF)	return ENDF;

		// Add char to buffer
		cltextappend(c);

		int result = Check4MathSymbols(c);
		if (result != 0) return result;

		// Check for multi-char values
		EparseResponse response = check4numconst(c);
		if (response == eTRUE) return NUMCONST;
		else if (response == eERR) return ERR;

		cltextappend(c);
		if (CheckFor("READ", c)) return READSTMT;
		if (CheckFor("WRITE", c)) return WRITESTMT;
		if (CheckFor(":=", c)) return ASIGN;
		if (check4var(c)) return ID;

		// Unidentified input character -- Return ERR
		return ERR;
	}

	// Should never get here
	return ENDF;
}

bool CompileProgram()
{
	int tokenCount = 0;

	// tokenCache keeps track of the token immediately following an end of statement. e.g. In the statement "sum := 5+5 write sum", the code would go through the statement until
	//	it encounters the write statement, then it would finish the statement and save the 'write' token so it doesn't get lost
	if ((tokenCache = calclex()) != ENDF)
	{
		// Will loop until the cached token is ENDF
		while (tokenCache != ENDF)
		{
			switch (tokenCache)
			{
			// We've reached the end
			case ENDF:
				tokenCache = 0;
				break;
			case ERR:
				cout << "ERROR on line " << lineNumber << endl;
				return false;
			case READSTMT:
				CaseRead();
				break;
			case WRITESTMT:
				CaseWrite();
				break;
			case ID:
				CaseId();
				break;
			// Should never get here
			default:
				cout << "ERROR on line " << lineNumber << endl;
				cout << "'" << getStringFromToken(tokenCache) << "' cannot be the first token of a statement" << endl;
				return false;
			}
			
			tokenCount++;
		}
	}

	cout << endl << "Number of tokens = " << tokenCount << endl;
	return true;
}

void CaseRead()
{
	cout << "Read: ";
	// Get the Id to read
	string nextId = GetNextId();
	double input = 0;

	// Input the value for the ID
	cout << "Enter a value for " << nextId << " => ";
	cin >> input;

	if (!cin)
	{
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Invalid input" << endl;
		system("pause"); exit(1);
	}
	
	// Add the pair to the table
	IdTable[nextId] = input;

	tokenCache = calclex();
}

void CaseWrite()
{
	cout << "Write: ";

	double output = GetStatement(0.0);
	// Output the found Id
	cout << output << endl;
}

// If this gets fired, ID is the beginning of a line so the only possibility is an assignment statement
void CaseId()
{
	string idName = cltext;

	if (ConfirmAsign())
	{
		cout << "Assign: " << idName << " := ";
		// This is an alignment statement, we just need to get the full thing to asign
		double val = GetAsignmentStatement(0.0);
		cout << val << endl;

		// Add the pair to the table
		IdTable[idName] = val;
	}
}

// Set the tokenCache to be the next value because the current function did not grab the next token
string GetNextId()
{
	int token;
	if ((token = calclex()) == ID)
	{
		return cltext;
	}
	cout << "ERROR on line " << lineNumber << endl;
	cout << "Expected ID, received " << getStringFromToken(token) << endl;
	system("pause"); exit(1);
}

// For closing parenthesis
void GetNextToken()
{
	tokenCache = calclex();
}

// Standard function that gets everything to the right of the current value. Called mostly from REad, Write, or LowOp commands
double GetStatement(double curVal)
{
	int token;
	double newVal;
	token = calclex();
	
	// The last statement that is called will be the last thing written here.
	tokenCache = token;

	switch (token)
	{
	// You've hit the bottom of the recursive pit of death! Return the value!
	case ENDF:
		return curVal;
	case ERR:
		cout << "ERROR on line " << lineNumber << endl;
		system("pause"); exit(1);
	// If the following occur, the statement has ended
	case READSTMT:
		tokenCache = token;
		return curVal;
	case WRITESTMT:
		tokenCache = token;
		return curVal;
	case ASIGN:
		tokenCache = token;
		return curVal;
	// End of parenthesized statement. Return result
	case RPAREN:
		return curVal;
	// Recursively get the value from the entire parenthesized statement
	case LPAREN:
		// Get value inside the statement
		newVal = GetStatement(curVal);
		// Get value after the statement (if any)
		newVal = GetStatementFromId(newVal);
		// If the right parenthesis was the end of the statement, get the following token
		if (tokenCache == RPAREN)
		{
			GetNextToken();
		}
		return newVal;
	// Operators
	case ADDOP:
		newVal = GetStatement(curVal);
		return curVal + newVal;
	case SUBOP:
		newVal = GetStatement(curVal);
		return curVal - newVal;
	case MULTOP:
		newVal = GetStatementFromHighOp(curVal);
		newVal = curVal * newVal;			
		return GetStatementFromId(newVal);
	case DIVOP:
		newVal = GetStatementFromHighOp(curVal);
		if (newVal == 0)
		{
			cout << "ERROR - Cannot Divide By Zero" << endl;
			system("pause"); exit(1);
		}
		newVal = curVal / newVal;
		return GetStatementFromId(newVal);
	case ID:
		return GetStatementFromId(IdTable[cltext]);
	case NUMCONST:
		return GetStatementFromId(atof(cltext));
	default:
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Unknown Token" << endl;
		system("pause"); exit(1);
	}
}

// This is when the first part of the statement cannot be write or sum
double GetAsignmentStatement(double curVal)
{
	int token;
	double newVal;
	token = calclex();

	// The last statement that is called will be the last thing written here.
	tokenCache = token;

	switch (token)
	{
		// You've hit the bottom of the recursive pit of death! Return the value!
	case ENDF:
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Expected Value, reached end of file instead" << endl;
		system("pause"); exit(1);
	case ERR:
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Expected Value" << endl;
		system("pause"); exit(1);
		// End of parenthesized statement. Return result
	case LPAREN:
		// Get value inside the statement
		newVal = GetStatement(curVal);
		// Get value after the statement (if any)
		newVal = GetStatementFromId(newVal);
		// If the right parenthesis was the end of the statement, get the following token
		if (tokenCache == RPAREN)
		{
			GetNextToken();
		}
		return newVal;
	case ID:
		return GetStatementFromId(IdTable[cltext]);
	case NUMCONST:
		return GetStatementFromId(atof(cltext));
	default:
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Expected a value or statement." << endl;
		cout << "An assignment statement cannot begin with " << getStringFromToken(token) << " '" << cltext << "'" << endl;
		system("pause"); exit(1);
	}
}

// Because a ID followed by another ID is a special case, this function does the same as above except it will return an end of statement if it encounters an ID immediately after an ID
double GetStatementFromId(double curVal)
{
	int token;
	double newVal;
	token = calclex();

	// The last statement that is called will be the last thing written here.
	tokenCache = token;

	switch (token)
	{
		// You've hit the bottom of the recursive pit of death! Return the value!
	case ENDF:
		return curVal;
	case ERR:
		cout << "ERROR on line " << lineNumber << endl;
		system("pause"); exit(1);
		// If the following occur, the statement has ended
	case READSTMT:
		tokenCache = token;
		return curVal;
	case WRITESTMT:
		tokenCache = token;
		return curVal;
	case ASIGN:
		tokenCache = token;
		return curVal;
		// End of parenthesized statement. Return result
	case RPAREN:
		return curVal;
	// ID or NUMCONST followed by and ID or NUMCONST is an end of statement char
	case ID:
		tokenCache = token;
		return curVal;
	case NUMCONST:
		tokenCache = token;
		return curVal;
	// Recursively get the value from the entire parenthesized statement
	case LPAREN:
		// Get value inside the statement
		newVal = GetStatement(curVal);
		// Get value after the statement (if any)
		newVal = GetStatementFromId(newVal);
		// If the right parenthesis was the end of the statement, get the following token
		if (tokenCache == RPAREN)
		{
			GetNextToken();
		}
		return newVal;
	// Operators
	case ADDOP:
		newVal = GetStatement(curVal);
		return curVal + newVal;
	case SUBOP:
		newVal = GetStatement(curVal);
		return curVal - newVal;
	case MULTOP:
		newVal = GetStatementFromHighOp(curVal);
		newVal = curVal * newVal;
		return GetStatementFromCache(newVal);
	case DIVOP:
		newVal = GetStatementFromHighOp(curVal);
		if (newVal == 0)
		{
			cout << "ERROR - Cannot Divide By Zero" << endl;
			system("pause"); exit(1);
		}
		newVal = curVal / newVal;
		return GetStatementFromCache(newVal);
	default:
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Unknown Token" << endl;
		system("pause"); exit(1);
	}
}

// This is called form mult and div ops so it doesn't add or sub out of order
double GetStatementFromHighOp(double curVal)
{
	int token;
	double newVal;
	token = calclex();

	// The last statement that is called will be the last thing written here.
	tokenCache = token;

	switch (token)
	{
		// You've hit the bottom of the recursive pit of death! Return the value!
	case ENDF:
		return curVal;
	case ERR:
		cout << "ERROR on line " << lineNumber << endl;
		system("pause"); exit(1);
		// If the following occur, the statement has ended
	case READSTMT:
		tokenCache = token;
		return curVal;
	case WRITESTMT:
		tokenCache = token;
		return curVal;
	case ASIGN:
		tokenCache = token;
		return curVal;
		// End of parenthesized statement. Return result
	case RPAREN:
		return curVal;
		// Recursively get the value from the entire parenthesized statement
	case LPAREN:
		// Get value inside the statement
		newVal = GetStatement(curVal);
		// If the right parenthesis was the end of the statement, get the following token
		if (tokenCache == RPAREN)
		{
			GetNextToken();
		}
		return newVal;
		// Operators
	case ADDOP:
		tokenCache = token;
		return curVal;
	case SUBOP:
		tokenCache = token;
		return curVal;
	case MULTOP:
		newVal = GetStatement(curVal);
		return curVal * newVal;
	case DIVOP:
		newVal = GetStatement(curVal);
		if (newVal == 0)
		{
			cout << "ERROR - Cannot Divide By Zero" << endl;
			system("pause"); exit(1);
		}
		return curVal / newVal;
	// Because this was called from a high operator, we just return the value directly
	case ID:
		newVal = IdTable[cltext];
		GetNextToken();
		return newVal;
	case NUMCONST:
		newVal = atof(cltext);
		GetNextToken();
		return newVal;
	default:
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Unknown Token" << endl;
		system("pause"); exit(1);
	}
}

// When calling a High Operation from a High Operation, you need to use the cache directly because it's the end of it's mini statement
double GetStatementFromCache(double curVal)
{
	int token = tokenCache;
	double newVal;

	switch (token)
	{
		// You've hit the bottom of the recursive pit of death! Return the value!
	case ENDF:
		return curVal;
	case ERR:
		cout << "ERROR on line " << lineNumber << endl;
		system("pause"); exit(1);
		// If the following occur, the statement has ended
	case READSTMT:
		tokenCache = token;
		return curVal;
	case WRITESTMT:
		tokenCache = token;
		return curVal;
	case ASIGN:
		tokenCache = token;
		return curVal;
		// End of parenthesized statement. Return result
	case RPAREN:
		return curVal;
		// ID or NUMCONST followed by and ID or NUMCONST is an end of statement char
	case ID:
		tokenCache = token;
		return curVal;
	case NUMCONST:
		tokenCache = token;
		return curVal;
		// Recursively get the value from the entire parenthesized statement
	case LPAREN:
		// Get value inside the statement
		newVal = GetStatement(curVal);
		// Get value after the statement (if any)
		newVal = GetStatementFromId(newVal);
		// If the right parenthesis was the end of the statement, get the following token
		if (tokenCache == RPAREN)
		{
			GetNextToken();
		}
		return newVal;
		// Operators
	case ADDOP:
		newVal = GetStatement(curVal);
		return curVal + newVal;
	case SUBOP:
		newVal = GetStatement(curVal);
		return curVal - newVal;
	case MULTOP:
		newVal = GetStatementFromHighOp(curVal);
		newVal = curVal * newVal;
		return GetStatementFromCache(newVal);
	case DIVOP:
		newVal = GetStatementFromHighOp(curVal);
		if (newVal == 0)
		{
			cout << "ERROR - Cannot Divide By Zero" << endl;
			system("pause"); exit(1);
		}
		newVal = curVal / newVal;
		return GetStatementFromCache(newVal);
	default:
		cout << "ERROR on line " << lineNumber << endl;
		cout << "Unknown Token" << endl;
		system("pause"); exit(1);
	}
}

bool ConfirmAsign()
{
	int token;
	if ((token = calclex()) == ASIGN)
	{
		return true;
	}
	cout << "ERROR on line " << lineNumber << endl;
	cout << "Expected Assignment operator, received " << getStringFromToken(token) << endl;
	system("pause"); exit(1);
}
