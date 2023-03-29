#include"Debugger2.h"

Debugger2::Debugger2(std::wstring wFilePath) {
	wTargetPath = wFilePath;
	memset(&deTarget, 0, sizeof(deTarget));
	memset(&cpdiTarget, 0, sizeof(cpdiTarget));
	memset(&ediTarget, 0, sizeof(ediTarget));
}

Debugger2::~Debugger2() {
}

bool Debugger2::Run(bool (*_OnBreak)(Debugger2 &)) {
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	// run new process
	wprintf(L"*TargetPath = %s\n", wTargetPath.c_str());
	if (!CreateProcessW(wTargetPath.c_str(), NULL, NULL, NULL, NULL, DEBUG_PROCESS | CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		puts("failed to run new process");
		return false;
	}

	// start
	ResumeThread(pi.hThread);

	// debug
	while (WaitForDebugEvent(&deTarget, INFINITE)) {
		if (deTarget.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
			break;
		}
		// check main thread
		if (deTarget.dwThreadId != pi.dwThreadId) {
			ContinueDebugEvent(deTarget.dwProcessId, deTarget.dwThreadId, DBG_CONTINUE);
			continue;
		}

		// debug event
		switch (deTarget.dwDebugEventCode) {
			// break point and single step
		case EXCEPTION_DEBUG_EVENT:
		{
			_OnBreak(*this);
			{
				ediTarget = deTarget.u.Exception;

				switch (ediTarget.ExceptionRecord.ExceptionCode) {
				case EXCEPTION_BREAKPOINT:
				case STATUS_WX86_BREAKPOINT:
				{
					bool ret = DeleteBreakPoint(ediTarget.ExceptionRecord.ExceptionAddress);
					if (ret) {
						CONTEXT ct;
						memset(&ct, 0, sizeof(ct));
						ct.ContextFlags = CONTEXT_FULL;

						if (GetThreadContext(cpdiTarget.hThread, &ct)) {
							ct.EFlags |= 0x00000100;
							ct.Rip--;
							ret = SetThreadContext(cpdiTarget.hThread, &ct);
						}
					}
					break;
				}
				case STATUS_SINGLE_STEP:
				case STATUS_WX86_SINGLE_STEP:
					//_OnBreak(*this);
					SingleStep();
					break;
				case STATUS_ACCESS_VIOLATION:
					break;
				default:
					printf("%p = %08X\n", ediTarget.ExceptionRecord.ExceptionAddress, ediTarget.ExceptionRecord.ExceptionCode);
					break;
				}
			}
			break;
		}
			// break on entry point
		case CREATE_PROCESS_DEBUG_EVENT:
			BreakOnEntryPoint();
			break;
			// stop debugging
		case EXIT_PROCESS_DEBUG_EVENT:
			break;
			// others
		default:
			break;
		}
		if (!ContinueDebugEvent(deTarget.dwProcessId, deTarget.dwThreadId, DBG_CONTINUE)) {
			break;
		}
	}

	// end
	if (pi.hThread) {
		CloseHandle(pi.hThread);
	}
	if (pi.hProcess) {
		CloseHandle(pi.hProcess);
	}

	return false;
}

bool Debugger2::GetReg(CONTEXT &ct) {
	memset(&ct, 0, sizeof(ct));
	ct.ContextFlags = CONTEXT_FULL;

	if (!GetThreadContext(cpdiTarget.hThread, &ct)) {
		return false;
	}

	return true;
}

bool Debugger2::SetReg(CONTEXT &ct) {
	return SetThreadContext(cpdiTarget.hThread, &ct);
}

bool Debugger2::BreakOnEntryPoint() {
	cpdiTarget = deTarget.u.CreateProcessInfo;
	return SetBreakPoint(cpdiTarget.lpStartAddress);
}

bool Debugger2::SetBreakPoint(LPVOID lpAddress) {
	std::vector<BYTE> vb(1);

	if (!ReadMemory(lpAddress, vb)) {
		return false;
	}

	std::vector<BYTE> vbp;
	vbp.push_back(0xCC);

	if (!WriteMemory(lpAddress, vbp)) {
		return false;
	}

	vBreakPoints.push_back(lpAddress);
	vBreakPointCodes.push_back(vb);
	return true;
}

bool Debugger2::DeleteBreakPoint(LPVOID lpAddress) {
	for (size_t i = 0; i < vBreakPoints.size(); i++) {
		if (vBreakPoints[i] == lpAddress) {
			if (!WriteMemory(lpAddress, vBreakPointCodes[i])) {
				return false;
			}
			vBreakPoints.erase(vBreakPoints.begin() + i);
			vBreakPointCodes.erase(vBreakPointCodes.begin() + i);
			return true;
		}
	}
	return false;
}

bool Debugger2::ReadMemory(LPVOID lpAddress, std::vector<BYTE> &vMemory) {
	SIZE_T st;

	if (!ReadProcessMemory(cpdiTarget.hProcess, lpAddress, &vMemory[0], vMemory.size(), &st)) {
		return false;
	}

	return true;
}

bool Debugger2::WriteMemory(LPVOID lpAddress, std::vector<BYTE> &vMemory) {
	DWORD dw;

	if (!VirtualProtectEx(cpdiTarget.hProcess, lpAddress, vMemory.size(), PAGE_EXECUTE_READWRITE, &dw)) {
		return false;
	}

	SIZE_T st;

	if (!WriteProcessMemory(cpdiTarget.hProcess, lpAddress, &vMemory[0], vMemory.size(), &st)) {
		return false;
	}

	if (!VirtualProtectEx(cpdiTarget.hProcess, lpAddress, vMemory.size(), dw, &dw)) {
		return false;
	}

	return true;
}

void Debugger2::SingleStep() {
	CONTEXT ct;
	memset(&ct, 0, sizeof(ct));
	ct.ContextFlags = CONTEXT_FULL;

	if (GetThreadContext(cpdiTarget.hThread, &ct)) {
		std::vector<BYTE> b(6);

	
		if (ReadMemory((LPVOID)ct.Rip, b)) {
			
			if (b[0] == 0xE8) {
				//SetBreakPoint((LPVOID)(ct.Rip + 0x05));
				//return;
			}

			if (b[0] == 0xFF && b[1] == 0x15) {
				std::vector<BYTE> addr(8);
				ReadMemory((LPVOID)(*(DWORD *)&b[2] + ct.Rip + 0x06), addr);
				printf("%llX : API CALL %llX\n", ct.Rip, *(ULONG_PTR *)&addr[0]);
				SetBreakPoint((LPVOID)(ct.Rip + 0x06));
				return;
			}
		}
		else {
			//printf("%llX (ERROR)\n", ct.Rip);
		}

		ct.EFlags |= 0x00000100;
		SetThreadContext(cpdiTarget.hThread, &ct);
	}
	else {
		printf("*Single Step Error");
	}
}