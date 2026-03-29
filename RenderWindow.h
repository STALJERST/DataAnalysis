#ifndef DATAANALIS_RENDERWINDOW_H
#define DATAANALIS_RENDERWINDOW_H

#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <iomanip>

#define IDM_READ_MEM 4001
#define IDM_WRITE_MEM 4002
#define IDC_SEARCH_EDIT 5001
#define IDC_SEARCH_COMBO 5002
#define IDC_SCAN_EDIT    6001
#define IDC_SCAN_BUTTON  6002
#define IDC_RESULTS_LIST 6003
#define IDC_BTN_LAUNCH_TEST 6004
#define IDC_PID_LABEL 6005
#define IDC_SAVED_LIST 6006
#define IDC_NEXT_SCAN_BUTTON 6008
#define IDC_TYPE_COMBO 6009


enum ScanDataType {
    TYPE_INT = 0,    // 4 байти (Ціле число)
    TYPE_FLOAT = 1,  // 4 байти (Дробове)
    TYPE_BYTE = 2    // 1 байт (Значення 0-255)
};

struct DataItem {
    std::wstring processName; // Використовуємо wstring для безпечного зберігання
    DWORD processID;          // Для PID краще використовувати DWORD
};

class RenderWindow {
public:
    RenderWindow();
    bool Create(const wchar_t* title, int width, int height);
    HWND getHWND() { return m_hwnd; };

    void AddData(const std::wstring& name, DWORD pid);
    void LoadProcesses(); // Метод для завантаження процесів
    void ShowMemoryDump(uintptr_t address);

private:
    HWND m_hwnd;
    HWND m_hListView;
    HWND m_hSearchEdit;  // Поле для вводу тексту
    HWND m_hSearchCombo; // Випадаючий список (По назві / По PID)
    HWND m_hScanEdit;
    HWND m_hScanBtn;
    HWND m_hResultsList; // Список знайдених адрес
    HWND m_hLaunchBtn;
    HWND m_hSavedList;     // Нижній список (Збережені адреси)
    HWND m_hNextScanBtn;
    HWND m_hPidLabel;
    HWND m_hTypeCombo; // Вікно вибору типу

    ScanDataType m_currentScanType = TYPE_INT;

    std::vector<DataItem> m_allData;      // ГОЛОВНА база всіх процесів
    std::vector<DataItem> m_filteredData; // ВІДФІЛЬТРОВАНА база для відображення
    std::vector<uintptr_t> m_scanResults;
    std::vector<uintptr_t> m_savedAddresses;

    template <typename T>
    std::vector<uintptr_t> ScanMemory(DWORD pid, T targetValue) {
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

                    // Зверніть увагу: тепер ми віднімаємо sizeof(T)
                    for (SIZE_T i = 0; i <= bytesRead - sizeof(T); i++) {
                        T val = *(T*)&buffer[i]; // Інтерпретуємо байти як наш тип T
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
    std::vector<uintptr_t> NextScanMemory(DWORD pid, T targetValue) {
        std::vector<uintptr_t> filtered;
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) return filtered;

        T currentValue;
        SIZE_T bytesRead;
        for (uintptr_t addr : m_scanResults) {
            if (ReadProcessMemory(hProcess, (LPCVOID)addr, &currentValue, sizeof(T), &bytesRead)) {
                if (currentValue == targetValue) {
                    filtered.push_back(addr);
                }
            }
        }
        CloseHandle(hProcess);
        return filtered;
    }

    DWORD m_selectedPID = 0; // Зберігаємо PID процесу, по якому клікнули

    void SetupControls();
    void FilterData();
    void RunFirstScan();
    void UpdatePidLabel();
    uintptr_t PatternScan(DWORD pid, const char* pattern, const char* mask);
    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MemoryDumpProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    struct DumpContext {
        std::wstring text;
    };
};


#endif //DATAANALIS_RENDERWINDOW_H