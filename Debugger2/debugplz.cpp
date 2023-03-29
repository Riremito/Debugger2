#include"Debugger2.h"
#include<stdio.h>


#define TEST_DIR L"C:\\Users\\elfenlied\\Desktop\\debugplz"
#define TEST_EXE L"HelloWorld_x64.exe"

bool OnBreak(Debugger2 &d2) {
	CONTEXT ct;
	if (d2.GetReg(ct)) {
		//printf("BREAK! %llX\n", ct.Rip);
	}
	//d2.SingleStep();
	return true;
}

int wmain(int argc, wchar_t *argv[]) {
	Debugger2 d2((argc < 2) ? TEST_DIR L"\\" TEST_EXE : argv[1]);

	d2.Run(OnBreak);
	return 0;
}