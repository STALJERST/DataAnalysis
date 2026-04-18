#include "MemoryEngine.h"



uintptr_t MemoryEngine::PatternScan(DWORD pid, const char* pattern, const char* mask) {
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return 0;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    uintptr_t currentAddr = (uintptr_t)sysInfo.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)sysInfo.lpMaximumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;

    size_t patternLen = strlen(mask);

    while (currentAddr < maxAddr) {
        if (VirtualQueryEx(hProcess, (LPCVOID)currentAddr, &mbi, sizeof(mbi)) == 0) break;

        if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_EXECUTE_READ || mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            std::vector<BYTE> buffer(mbi.RegionSize);
            SIZE_T bytesRead = 0;

            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                for (SIZE_T i = 0; i <= bytesRead - patternLen; i++) {
                    bool found = true;
                    for (SIZE_T j = 0; j < patternLen; j++) {
                        if (mask[j] == 'x' && buffer[i + j] != (BYTE)pattern[j]) {
                            found = false;
                            break;
                        }
                    }
                    if (found) {
                        CloseHandle(hProcess);
                        return (uintptr_t)mbi.BaseAddress + i;
                    }
                }
            }
        }
        currentAddr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    CloseHandle(hProcess);
    return 0;
}

void MemoryEngine::SuspendProcess(DWORD targetPID, bool suspend) {
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot == INVALID_HANDLE_VALUE) return;

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hThreadSnapshot, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID == targetPID) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                if (hThread) {
                    if (suspend) SuspendThread(hThread);
                    else ResumeThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hThreadSnapshot, &threadEntry));
    }
    CloseHandle(hThreadSnapshot);
}