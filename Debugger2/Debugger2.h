#ifndef __DEBUGGER2_H__
#define __DEBUGGER2_H__

#define UMDF_USING_NTSTATUS
#include<ntstatus.h>
#include<Windows.h>
#include<string>
#include<vector>

class Debugger2 {
private:
	std::wstring wTargetPath;
	DEBUG_EVENT deTarget;
	CREATE_PROCESS_DEBUG_INFO cpdiTarget;
	EXCEPTION_DEBUG_INFO ediTarget;

	std::vector<LPVOID> vBreakPoints;
	std::vector<std::vector<BYTE>> vBreakPointCodes;

	bool BreakOnEntryPoint();
	bool SetBreakPoint(LPVOID lpAddress);
	bool DeleteBreakPoint(LPVOID lpAddress);
	bool ReadMemory(LPVOID lpAddress, std::vector<BYTE> &vMemory);
	bool WriteMemory(LPVOID lpAddress, std::vector<BYTE> &vMemory);

public:
	Debugger2(std::wstring wFilePath);
	~Debugger2();
	bool Run(bool(*_OnBreak)(Debugger2 &));
	bool GetReg(CONTEXT &ct);
	bool SetReg(CONTEXT &ct);
	void SingleStep();
};

#endif