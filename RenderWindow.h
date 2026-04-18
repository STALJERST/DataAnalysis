#ifndef DATAANALIS_RENDERWINDOW_H
#define DATAANALIS_RENDERWINDOW_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

#include "MemoryEngine.h"

#define IDM_READ_MEM 4001
#define IDM_WRITE_MEM 4002
#define IDC_SEARCH_EDIT 5001
#define IDC_SEARCH_COMBO 5002
#define IDC_SORT_COMBO 5003
#define IDC_SCAN_EDIT    6001
#define IDC_SCAN_BUTTON  6002
#define IDC_RESULTS_LIST 6003
#define IDC_BTN_LAUNCH_TEST 6004
#define IDC_PID_LABEL 6005
#define IDC_SAVED_LIST 6006
#define IDC_NEXT_SCAN_BUTTON 6007
#define IDC_TYPE_COMBO 6008
#define IDC_STATIC_LIST 6009
#define IDC_TOOLS_COMBO 6010
#define IDC_BTN_EXECUTE_TOOL 6011

enum ScanDataType {
    TYPE_INT = 0,
    TYPE_FLOAT = 1,
    TYPE_BYTE = 2
};

struct DataItem {
    std::wstring processName;
    DWORD processID;
    FILETIME creationTime;
    std::wstring launchTimeStr;
    std::wstring uptimeStr;
};

struct StaticSignature {
    std::wstring name;
    std::string pattern;
    std::string mask;
    uintptr_t resolvedAddress;
};

class RenderWindow {
public:
    RenderWindow();
    bool Create(const wchar_t* title, int width, int height);
    HWND getHWND() { return m_hwnd; };

private:
    HWND m_hwnd;
    HWND m_hListView;
    HWND m_hSearchEdit;
    HWND m_hSearchCombo;
    HWND m_hScanEdit;
    HWND m_hScanBtn;
    HWND m_hResultsList;
    HWND m_hSavedList;
    HWND m_hNextScanBtn;
    HWND m_hPidLabel;
    HWND m_hTypeCombo;
    HWND m_hToolsCombo;
    HWND m_hExecuteToolBtn;
    HWND m_hStaticList;
    HWND m_hSortCombo;

    DWORD m_selectedPID = 0;
    ScanDataType m_currentScanType = TYPE_INT;

    std::vector<DataItem> m_allData;
    std::vector<DataItem> m_filteredData;
    std::vector<uintptr_t> m_scanResults;
    std::vector<uintptr_t> m_savedAddresses;
    std::vector<StaticSignature> m_staticItems;

    void SetupControls();
    void LoadProcesses();
    void FilterData();
    void UpdatePidLabel();
    void ShowMemoryDump(uintptr_t address);
    void SortProcesses();

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MemoryDumpProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    struct DumpContext {
        std::wstring text;
    };
};

#endif // DATAANALIS_RENDERWINDOW_H