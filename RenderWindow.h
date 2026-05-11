#ifndef DATAANALIS_RENDERWINDOW_H
#define DATAANALIS_RENDERWINDOW_H

#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h> // Для роботи з процесами
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <algorithm> // Для std::transform
#include <cwctype>   // Для ::towlower
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

    std::vector<DataItem> m_allData;      // ГОЛОВНА база всіх процесів
    std::vector<DataItem> m_filteredData; // ВІДФІЛЬТРОВАНА база для відображення
    std::vector<uintptr_t> m_scanResults;
    std::vector<uintptr_t> m_savedAddresses;
    std::vector<uintptr_t> ScanMemoryForInt(DWORD pid, int targetValue);
    std::vector<uintptr_t> NextScanMemoryForInt(DWORD pid, int targetValue);

    DWORD m_selectedPID = 0; // Зберігаємо PID процесу, по якому клікнули

    void SetupControls();
    void FilterData();
    void RunFirstScan();
    void UpdatePidLabel();

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MemoryDumpProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    struct DumpContext {
        std::wstring text;
    };
};


#endif //DATAANALIS_RENDERWINDOW_H