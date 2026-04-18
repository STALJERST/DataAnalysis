#ifndef DATAANALYSIS_MEMORYENGINE_H
#define DATAANALYSIS_MEMORYENGINE_H

#include <windows.h>
#include <vector>
#include <string>
#include <tlhelp32.h>

class MemoryEngine {
public:
    static uintptr_t PatternScan(DWORD pid, const char* pattern, const char* mask);


    static void SuspendProcess(DWORD targetPID, bool suspend);

    template <typename T>
    static std::vector<uintptr_t> Scan(DWORD pid, T targetValue) {
        std::vector<uintptr_t> foundAddresses;
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProcess) return foundAddresses;

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        uintptr_t currentAddr = (uintptr_t)sysInfo.lpMinimumApplicationAddress;
        uintptr_t maxAddr = (uintptr_t)sysInfo.lpMaximumApplicationAddress;
        MEMORY_BASIC_INFORMATION mbi;

        while (currentAddr < maxAddr) {
            if (VirtualQueryEx(hProcess, (LPCVOID)currentAddr, &mbi, sizeof(mbi)) == 0) break;

            if (mbi.State == MEM_COMMIT && (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
                std::vector<BYTE> buffer(mbi.RegionSize);
                SIZE_T bytesRead = 0;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                    for (SIZE_T i = 0; i <= bytesRead - sizeof(T); i++) {
                        T val = *(T*)&buffer[i];
                        if (val == targetValue) {
                            foundAddresses.push_back((uintptr_t)mbi.BaseAddress + i);
                        }
                    }
                }
            }
            currentAddr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        }
        CloseHandle(hProcess);
        return foundAddresses;
    }

    template <typename T>
    static std::vector<uintptr_t> NextScan(DWORD pid, const std::vector<uintptr_t>& previousResults, T targetValue) {
        std::vector<uintptr_t> filtered;
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) return filtered;

        T currentValue;
        SIZE_T bytesRead;
        for (uintptr_t addr : previousResults) {
            if (ReadProcessMemory(hProcess, (LPCVOID)addr, &currentValue, sizeof(T), &bytesRead)) {
                if (currentValue == targetValue) {
                    filtered.push_back(addr);
                }
            }
        }
        CloseHandle(hProcess);
        return filtered;
    }
};

#endif // DATAANALYSIS_MEMORYENGINE_H