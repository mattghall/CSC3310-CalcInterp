//Matt G Hall
// Main

#include <iomanip> 
#include <string>
#include <map> 
#include "CalcInterp.h"

using namespace std;

void doSymEntry();
void dopSymEntry();

int main(int argc, char *argv[])
{
	// Pick up commandline input filename, if any
	if (argc > 1 && (!calclexopen(argv[1])))
	{
		cout << "Error: Cannot open input file " << argv[1] << endl;
		exit(1);
	}
	
	if (!CompileProgram())
	{
		system("pause");
		exit(1);
	}
		
	system("pause");
	return 0;
}




// cd C:\Users\MattGHall\Documents\Visual Studio 2013\Projects\Concepts\MattGHall_Concepts_Lab1_CalcLex\Debug
// MattGHall_Concepts_Lab1_CalcLex.exe SampleProgram.txt