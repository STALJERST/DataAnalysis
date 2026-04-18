#include "RenderWindow.h"
#include <tlhelp32.h>
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <iomanip>

RenderWindow::RenderWindow() {
    m_hwnd = NULL;
    m_hListView = NULL;
}

bool RenderWindow::Create(const wchar_t* title, int width, int height) {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = RenderWindow::StaticWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MyRendererClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(0, L"MyRendererClass", title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, hInstance, this);

    return m_hwnd != NULL;
}

LRESULT CALLBACK RenderWindow::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RenderWindow* pThis = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (RenderWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    } else {
        pThis = (RenderWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (pThis) return pThis->HandleMessage(uMsg, wParam, lParam);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void RenderWindow::LoadProcesses() {
    m_allData.clear();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &processEntry)) {
            do {
                DataItem item;
                item.processName = processEntry.szExeFile;
                item.processID = processEntry.th32ProcessID;
                item.creationTime = {0, 0};
                item.launchTimeStr = L"Немає доступу";
                item.uptimeStr = L"-";

                // Використовуємо LIMITED_INFORMATION, щоб система пустила нас до більшості процесів
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, item.processID);
                if (hProc) {
                    FILETIME ftCreation, ftExit, ftKernel, ftUser;
                    if (GetProcessTimes(hProc, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
                        item.creationTime = ftCreation;

                        // 1. Форматуємо час запуску (Години:Хвилини:Секунди)
                        SYSTEMTIME stUTC, stLocal;
                        FileTimeToSystemTime(&ftCreation, &stUTC);
                        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

                        wchar_t buf[64];
                        swprintf(buf, 64, L"%02d:%02d:%02d", stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
                        item.launchTimeStr = buf;

                        // 2. Рахуємо час роботи (Uptime)
                        FILETIME ftNow;
                        GetSystemTimeAsFileTime(&ftNow);
                        ULARGE_INTEGER uCreation, uNow;
                        uCreation.LowPart = ftCreation.dwLowDateTime;
                        uCreation.HighPart = ftCreation.dwHighDateTime;
                        uNow.LowPart = ftNow.dwLowDateTime;
                        uNow.HighPart = ftNow.dwHighDateTime;

                        if (uNow.QuadPart >= uCreation.QuadPart) {
                            // Переводимо 100-наносекундні інтервали у секунди
                            ULONGLONG totalSeconds = (uNow.QuadPart - uCreation.QuadPart) / 10000000ULL;
                            ULONGLONG hours = totalSeconds / 3600;
                            ULONGLONG mins = (totalSeconds % 3600) / 60;
                            ULONGLONG secs = totalSeconds % 60;

                            if (hours > 0) swprintf(buf, 64, L"%llu год %llu хв", hours, mins);
                            else if (mins > 0) swprintf(buf, 64, L"%llu хв %llu с", mins, secs);
                            else swprintf(buf, 64, L"%llu с", secs);

                            item.uptimeStr = buf;
                        }
                    }
                    CloseHandle(hProc);
                }
                m_allData.push_back(item);
            } while (Process32NextW(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);
    }
    FilterData(); // Відфільтрує і одразу відсортує
}

LRESULT RenderWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            SetupControls();
            LoadProcesses();
            return 0;
        }
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            int sideWidth = std::max(width / 6, 160);
            int mainX = sideWidth + 10;
            int mainWidth = width - mainX - 10;

            int topOffset = 65;
            int listHeight = (height - topOffset - 30) / 3;

            SetWindowPos(m_hSearchCombo, NULL, 5, 5, sideWidth - 10, 150, SWP_NOZORDER);
            SetWindowPos(m_hSearchEdit, NULL, 5, 35, sideWidth - 10, 25, SWP_NOZORDER);
            SetWindowPos(m_hListView, NULL, 5, 95, sideWidth - 10, height - 95 - 40, SWP_NOZORDER);

            SetWindowPos(m_hToolsCombo, NULL, 5, height - 35, sideWidth - 85, 150, SWP_NOZORDER);
            SetWindowPos(m_hExecuteToolBtn, NULL, sideWidth - 75, height - 36, 70, 24, SWP_NOZORDER);

            SetWindowPos(m_hSortCombo, NULL, 5, 65, sideWidth - 10, 150, SWP_NOZORDER);

            SetWindowPos(m_hPidLabel, NULL, mainX, 5, mainWidth, 20, SWP_NOZORDER);
            SetWindowPos(m_hScanEdit, NULL, mainX, 30, 80, 25, SWP_NOZORDER);
            SetWindowPos(m_hTypeCombo, NULL, mainX + 90, 30, 150,25, SWP_NOZORDER);
            SetWindowPos(m_hScanBtn, NULL, mainX + 200, 30, 140, 25, SWP_NOZORDER);
            SetWindowPos(m_hNextScanBtn, NULL, mainX + 350, 30, 140, 25, SWP_NOZORDER);

            SetWindowPos(m_hResultsList, NULL, mainX, topOffset, mainWidth, listHeight, SWP_NOZORDER);
            SetWindowPos(m_hSavedList, NULL, mainX, topOffset + listHeight + 5, mainWidth, listHeight, SWP_NOZORDER);
            SetWindowPos(m_hStaticList, NULL, mainX, topOffset + (listHeight * 2) + 10, mainWidth, listHeight, SWP_NOZORDER);
            return 0;
        }

        case WM_NOTIFY: {
            LPNMHDR lpnmh = (LPNMHDR)lParam;
            if (lpnmh->idFrom == IDC_STATIC_LIST && lpnmh->code == LVN_GETDISPINFOW) {
                NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)lParam;
                if (pDispInfo->item.mask & LVIF_TEXT) {
                    int row = pDispInfo->item.iItem;
                    int col = pDispInfo->item.iSubItem;

                    if (row >= 0 && row < m_staticItems.size()) {
                        if (col == 0) {
                            lstrcpynW(pDispInfo->item.pszText, m_staticItems[row].name.c_str(), pDispInfo->item.cchTextMax);
                        } else if (col == 1) {
                            if (m_staticItems[row].resolvedAddress == 0) {
                                swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"??? (Не знайдено)");
                            } else {
                                swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"0x%p", (void*)m_staticItems[row].resolvedAddress);
                            }
                        } else if (col == 2) {
                            if (m_staticItems[row].resolvedAddress != 0) {
                                int currentVal = 0;
                                HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, m_selectedPID);
                                if (hProc) {
                                    ReadProcessMemory(hProc, (LPCVOID)m_staticItems[row].resolvedAddress, &currentVal, sizeof(int), NULL);
                                    CloseHandle(hProc);
                                    swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%d", currentVal);
                                }
                            } else {
                                swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"-");
                            }
                        }
                    }
                }
            }

            if (lpnmh->idFrom == 1001 && lpnmh->code == LVN_GETDISPINFOW) {
                NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)lParam;
                if (pDispInfo->item.mask & LVIF_TEXT) {
                    int row = pDispInfo->item.iItem;
                    int col = pDispInfo->item.iSubItem;
                    if (row < 0 || row >= m_filteredData.size()) return 0;

                    if (col == 0) {
                        lstrcpynW(pDispInfo->item.pszText, m_filteredData[row].processName.c_str(), pDispInfo->item.cchTextMax);
                    } else if (col == 1) {
                        std::wstring valStr = std::to_wstring(m_filteredData[row].processID);
                        lstrcpynW(pDispInfo->item.pszText, valStr.c_str(), pDispInfo->item.cchTextMax);
                    }
                }
            }

            if (lpnmh->idFrom == IDC_RESULTS_LIST && lpnmh->code == LVN_GETDISPINFOW) {
                NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)lParam;
                if (pDispInfo->item.mask & LVIF_TEXT) {
                    int row = pDispInfo->item.iItem;
                    int col = pDispInfo->item.iSubItem;

                    if (row >= 0 && row < m_scanResults.size()) {
                        if (col == 0) {
                            swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"0x%p", (void*)m_scanResults[row]);
                        } else if (col == 1) {
                            HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, m_selectedPID);
                            if (hProc) {
                                switch (m_currentScanType) {
                                    case TYPE_INT: {
                                        int val = 0; ReadProcessMemory(hProc, (LPCVOID)m_scanResults[row], &val, sizeof(int), NULL);
                                        swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%d", val); break;
                                    }
                                    case TYPE_FLOAT: {
                                        float val = 0.0f; ReadProcessMemory(hProc, (LPCVOID)m_scanResults[row], &val, sizeof(float), NULL);
                                        swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%.2f", val); break;
                                    }
                                    case TYPE_BYTE: {
                                        BYTE val = 0; ReadProcessMemory(hProc, (LPCVOID)m_scanResults[row], &val, sizeof(BYTE), NULL);
                                        swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%u", val); break;
                                    }
                                }
                                CloseHandle(hProc);
                            }
                        }
                    }
                }
            }

            if (lpnmh->idFrom == 1001 && lpnmh->code == NM_RCLICK) {
                LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
                if (pnmitem->iItem >= 0 && pnmitem->iItem < m_filteredData.size()) {
                    m_selectedPID = m_filteredData[pnmitem->iItem].processID;
                    UpdatePidLabel();
                }
            }


            if (lpnmh->idFrom == IDC_SAVED_LIST && lpnmh->code == LVN_GETDISPINFOW) {
                NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)lParam;
                if (pDispInfo->item.mask & LVIF_TEXT) {
                    int row = pDispInfo->item.iItem;
                    int col = pDispInfo->item.iSubItem;
                    if (row >= 0 && row < m_savedAddresses.size()) {
                        if (col == 0) {
                            swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"0x%p", (void*)m_savedAddresses[row]);
                        } else if (col == 1) {
                            int currentVal = 0;
                            HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, m_selectedPID);
                            if (hProc) {
                                ReadProcessMemory(hProc, (LPCVOID)m_savedAddresses[row], &currentVal, sizeof(int), NULL);
                                CloseHandle(hProc);
                            }
                            swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%d", currentVal);
                        }
                    }
                }
            }

            if (lpnmh->idFrom == 1001 && lpnmh->code == LVN_GETDISPINFOW) {
                NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)lParam;
                if (pDispInfo->item.mask & LVIF_TEXT) {
                    int row = pDispInfo->item.iItem;
                    int col = pDispInfo->item.iSubItem;
                    if (row < 0 || row >= m_filteredData.size()) return 0;

                    if (col == 0) {
                        lstrcpynW(pDispInfo->item.pszText, m_filteredData[row].processName.c_str(), pDispInfo->item.cchTextMax);
                    } else if (col == 1) {
                        std::wstring valStr = std::to_wstring(m_filteredData[row].processID);
                        lstrcpynW(pDispInfo->item.pszText, valStr.c_str(), pDispInfo->item.cchTextMax);
                    } else if (col == 2) {
                        lstrcpynW(pDispInfo->item.pszText, m_filteredData[row].launchTimeStr.c_str(), pDispInfo->item.cchTextMax);
                    } else if (col == 3) {
                        lstrcpynW(pDispInfo->item.pszText, m_filteredData[row].uptimeStr.c_str(), pDispInfo->item.cchTextMax);
                    }
                }
            }

            if (lpnmh->idFrom == IDC_SAVED_LIST && lpnmh->code == NM_RCLICK) {
                LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
                if (pnmitem->iItem >= 0 && pnmitem->iItem < m_savedAddresses.size()) {
                    ShowMemoryDump(m_savedAddresses[pnmitem->iItem]);
                }
            }

            if (lpnmh->idFrom == IDC_RESULTS_LIST && lpnmh->code == NM_DBLCLK) {
                LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
                if (pnmitem->iItem >= 0 && pnmitem->iItem < m_scanResults.size()) {
                    m_savedAddresses.push_back(m_scanResults[pnmitem->iItem]);
                    ListView_SetItemCount(m_hSavedList, m_savedAddresses.size());
                    InvalidateRect(m_hSavedList, NULL, TRUE);
                }
            }
            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == IDC_SEARCH_EDIT && wmEvent == EN_CHANGE) { FilterData(); }
            else if (wmId == IDC_SEARCH_COMBO && wmEvent == CBN_SELCHANGE) {
                FilterData();
                SetFocus(m_hSearchEdit);
            }
            else if (wmId == IDC_SORT_COMBO && wmEvent == CBN_SELCHANGE) {
                SortProcesses();
            }
            else if (wmId == IDC_SCAN_BUTTON || wmId == IDC_NEXT_SCAN_BUTTON) {
                if (m_selectedPID == 0) {
                    MessageBoxW(m_hwnd, L"Спочатку виберіть процес!", L"Увага", MB_ICONWARNING);
                    return 0;
                }

                wchar_t buf[32];
                GetWindowTextW(m_hScanEdit, buf, 32);
                m_currentScanType = (ScanDataType)SendMessageW(m_hTypeCombo, CB_GETCURSEL, 0, 0);

                SetCursor(LoadCursor(NULL, IDC_WAIT));

                if (m_currentScanType == TYPE_INT) {
                    int val = _wtoi(buf);
                    if (wmId == IDC_SCAN_BUTTON) m_scanResults = MemoryEngine::Scan<int>(m_selectedPID, val);
                    else m_scanResults = MemoryEngine::NextScan<int>(m_selectedPID, m_scanResults, val);
                }
                else if (m_currentScanType == TYPE_FLOAT) {
                    float val = (float)_wtof(buf);
                    if (wmId == IDC_SCAN_BUTTON) m_scanResults = MemoryEngine::Scan<float>(m_selectedPID, val);
                    else m_scanResults = MemoryEngine::NextScan<float>(m_selectedPID, m_scanResults, val);
                }
                else if (m_currentScanType == TYPE_BYTE) {
                    BYTE val = (BYTE)_wtoi(buf);
                    if (wmId == IDC_SCAN_BUTTON) m_scanResults = MemoryEngine::Scan<BYTE>(m_selectedPID, val);
                    else m_scanResults = MemoryEngine::NextScan<BYTE>(m_selectedPID, m_scanResults, val);
                }

                SetCursor(LoadCursor(NULL, IDC_ARROW));
                ListView_SetItemCount(m_hResultsList, m_scanResults.size());
                InvalidateRect(m_hResultsList, NULL, TRUE);
            }
            else if (wmId == IDC_BTN_EXECUTE_TOOL) {
                int selectedTool = SendMessageW(m_hToolsCombo, CB_GETCURSEL, 0, 0);

                if (selectedTool == 0) {
                    // Запустити манекен
                    const char* dummySourceCode = R"(#include <iostream>
#include <windows.h>
#include <conio.h>

int main() {
    int playerMoney = 100;

    while(true) {
        std::cout <<" money: " << std::dec << playerMoney << " | adress: 0x" << std::hex << &playerMoney << std::dec << "\n";
        std::cout << "[SPACE] -10\n";

        if(_getch() == ' ') playerMoney -= 10;
        system("cls");
    }
    return 0;
})";

                    std::ofstream outFile("GeneratedDummy.cpp");
                    if (outFile.is_open()) { outFile << dummySourceCode; outFile.close(); }
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    if (system("g++ GeneratedDummy.cpp -o GeneratedDummy.exe") == 0) {
                        STARTUPINFOW si = { sizeof(si) }; PROCESS_INFORMATION pi;
                        if (CreateProcessW(L"GeneratedDummy.exe", NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
                            LoadProcesses(); m_selectedPID = pi.dwProcessId; UpdatePidLabel();
                            CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
                        }
                    }
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                }
                else if (selectedTool == 1) {
                    // Додати AOB
                    StaticSignature mockAOB;
                    mockAOB.name = L"Тестова сигнатура";
                    mockAOB.pattern = "\x89\x45\xFC\x8B\x0D\x00\x00\x00\x00";
                    mockAOB.mask = "xxxxx????";
                    mockAOB.resolvedAddress = 0;
                    m_staticItems.push_back(mockAOB);
                    ListView_SetItemCount(m_hStaticList, m_staticItems.size());
                    InvalidateRect(m_hStaticList, NULL, TRUE);
                }
                else if (selectedTool == 2) {
                    // Оновити AOB ВИКОРИСТОВУЮЧИ ЯДРО!
                    if (m_selectedPID == 0) return 0;
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    for (auto& item : m_staticItems) {
                        item.resolvedAddress = MemoryEngine::PatternScan(m_selectedPID, item.pattern.c_str(), item.mask.c_str());
                    }
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                    InvalidateRect(m_hStaticList, NULL, TRUE);
                }
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void RenderWindow::SetupControls() {
    m_hSearchCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 0, 150, m_hwnd, (HMENU)IDC_SEARCH_COMBO, NULL, NULL);
    SendMessageW(m_hSearchCombo, CB_ADDSTRING, 0, (LPARAM)L"Шукати по назві"); SendMessageW(m_hSearchCombo, CB_ADDSTRING, 0, (LPARAM)L"Шукати по PID");
    SendMessageW(m_hSearchCombo, CB_SETCURSEL, 0, 0);

    m_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_SEARCH_EDIT, NULL, NULL);
    m_hListView = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA, 0, 0, 0, 0, m_hwnd, (HMENU)1001, NULL, NULL);
    ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW lvc = {0}; lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0; lvc.cx = 100; lvc.pszText = (LPWSTR)L"Назва"; SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
    lvc.iSubItem = 1; lvc.cx = 60; lvc.pszText = (LPWSTR)L"PID"; SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

    m_hPidLabel = CreateWindowExW(0, L"STATIC", L"Вибраний процес (PID): Немає", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_PID_LABEL, NULL, NULL);
    m_hScanEdit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"100", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_SCAN_EDIT, NULL, NULL);

    m_hTypeCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 0, 150, m_hwnd, (HMENU)IDC_TYPE_COMBO, NULL, NULL);
    SendMessageW(m_hTypeCombo, CB_ADDSTRING, 0, (LPARAM)L"4 Байти (Int)");
    SendMessageW(m_hTypeCombo, CB_ADDSTRING, 0, (LPARAM)L"Float (Дробове)");
    SendMessageW(m_hTypeCombo, CB_ADDSTRING, 0, (LPARAM)L"1 Байт (Byte)"); SendMessageW(m_hTypeCombo, CB_SETCURSEL, 0, 0);

    m_hScanBtn = CreateWindowExW(0, L"BUTTON", L"Перше сканування", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_SCAN_BUTTON, NULL, NULL);
    m_hNextScanBtn = CreateWindowExW(0, L"BUTTON", L"Відсіяти (Next)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_NEXT_SCAN_BUTTON, NULL, NULL);

    m_hResultsList = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_RESULTS_LIST, NULL, NULL);
    ListView_SetExtendedListViewStyle(m_hResultsList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNW lvc1 = {0}; lvc1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; lvc1.fmt = LVCFMT_LEFT;
    lvc1.cx = 140; lvc1.pszText = (LPWSTR)L"Адреса"; SendMessageW(m_hResultsList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc1);
    lvc1.cx = 100; lvc1.pszText = (LPWSTR)L"Значення"; SendMessageW(m_hResultsList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc1);

    m_hSavedList = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_SAVED_LIST, NULL, NULL);
    ListView_SetExtendedListViewStyle(m_hSavedList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNW lvcSaved = {0}; lvcSaved.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; lvcSaved.fmt = LVCFMT_LEFT;
    lvcSaved.cx = 140; lvcSaved.pszText = (LPWSTR)L"Збережена Адреса"; SendMessageW(m_hSavedList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvcSaved);
    lvcSaved.cx = 100; lvcSaved.pszText = (LPWSTR)L"Поточне Значення"; SendMessageW(m_hSavedList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvcSaved);

    m_hToolsCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 0, 150, m_hwnd, (HMENU)IDC_TOOLS_COMBO, NULL, NULL);
    SendMessageW(m_hToolsCombo, CB_ADDSTRING, 0, (LPARAM)L"Запустити манекен");
    SendMessageW(m_hToolsCombo, CB_ADDSTRING, 0, (LPARAM)L"Додати тест AOB");
    SendMessageW(m_hToolsCombo, CB_ADDSTRING, 0, (LPARAM)L"Оновити посилання"); SendMessageW(m_hToolsCombo, CB_SETCURSEL, 0, 0);

    m_hExecuteToolBtn = CreateWindowExW(0, L"BUTTON", L"Виконати", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_BTN_EXECUTE_TOOL, NULL, NULL);

    m_hStaticList = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA, 0, 0, 0, 0, m_hwnd, (HMENU)IDC_STATIC_LIST, NULL, NULL);
    ListView_SetExtendedListViewStyle(m_hStaticList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    LVCOLUMNW lvcStat = {0}; lvcStat.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; lvcStat.fmt = LVCFMT_LEFT;
    lvcStat.cx = 120; lvcStat.pszText = (LPWSTR)L"Назва"; SendMessageW(m_hStaticList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvcStat);
    lvcStat.cx = 150; lvcStat.pszText = (LPWSTR)L"Знайдена Адреса"; SendMessageW(m_hStaticList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvcStat);
    lvcStat.cx = 80; lvcStat.pszText = (LPWSTR)L"Значення"; SendMessageW(m_hStaticList, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvcStat);

    m_hSortCombo = CreateWindowExW(0, WC_COMBOBOXW, L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                   0, 0, 0, 150, m_hwnd, (HMENU)IDC_SORT_COMBO, NULL, NULL);
    SendMessageW(m_hSortCombo, CB_ADDSTRING, 0, (LPARAM)L"За алфавітом");
    SendMessageW(m_hSortCombo, CB_ADDSTRING, 0, (LPARAM)L"За PID");
    SendMessageW(m_hSortCombo, CB_ADDSTRING, 0, (LPARAM)L"Найстаріші (FIFO)");
    SendMessageW(m_hSortCombo, CB_ADDSTRING, 0, (LPARAM)L"Найновіші (LIFO)");
    SendMessageW(m_hSortCombo, CB_SETCURSEL, 0, 0);

    lvc.iSubItem = 2; lvc.cx = 75; lvc.pszText = (LPWSTR)L"Запуск";
    SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);

    lvc.iSubItem = 3; lvc.cx = 90; lvc.pszText = (LPWSTR)L"Робота";
    SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);
}

void RenderWindow::FilterData() {
    int len = GetWindowTextLengthW(m_hSearchEdit);
    std::wstring query(len, L'\0'); GetWindowTextW(m_hSearchEdit, &query[0], len + 1);
    if (len > 0) query.resize(len);
    std::transform(query.begin(), query.end(), query.begin(), ::towlower);
    int filterMode = SendMessageW(m_hSearchCombo, CB_GETCURSEL, 0, 0);

    m_filteredData.clear();
    for (const auto& item : m_allData) {
        if (query.empty()) m_filteredData.push_back(item);
        else {
            if (filterMode == 0) {
                std::wstring nameLower = item.processName; std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::towlower);
                if (nameLower.find(query) != std::wstring::npos) m_filteredData.push_back(item);
            } else if (filterMode == 1) {
                std::wstring pidStr = std::to_wstring(item.processID);
                if (pidStr.find(query) != std::wstring::npos) m_filteredData.push_back(item);
            }
        }
    }
    ListView_SetItemCount(m_hListView, m_filteredData.size());
    InvalidateRect(m_hListView, NULL, TRUE);
    SortProcesses();
}

void RenderWindow::UpdatePidLabel() {
    if (m_selectedPID == 0) SetWindowTextW(m_hPidLabel, L"Вибраний процес (PID): Немає");
    else { std::wstring text = L" Цільовий процес (PID): " + std::to_wstring(m_selectedPID); SetWindowTextW(m_hPidLabel, text.c_str()); }
}

void RenderWindow::ShowMemoryDump(uintptr_t targetAddress) {
    if (m_selectedPID == 0) return;
    const SIZE_T dumpSize = 256; uintptr_t readAddress = targetAddress - 32; std::vector<BYTE> buffer(dumpSize, 0);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, m_selectedPID);
    if (!hProcess) return;
    SIZE_T bytesRead = 0; ReadProcessMemory(hProcess, (LPCVOID)readAddress, buffer.data(), dumpSize, &bytesRead);
    CloseHandle(hProcess);

    std::wstringstream ss;
    for (SIZE_T i = 0; i < bytesRead; i += 16) {
        ss << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << (readAddress + i) << L" | ";
        for (SIZE_T j = 0; j < 16; ++j) {
            if (i + j < bytesRead) ss << std::hex << std::setw(2) << std::setfill(L'0') << (int)buffer[i + j] << L" ";
            else ss << L"   ";
            if (j == 7) ss << L" ";
        }
        ss << L" | ";
        for (SIZE_T j = 0; j < 16; ++j) {
            if (i + j < bytesRead) {
                char c = buffer[i + j];
                if (c >= 32 && c <= 126) ss << (wchar_t)c; else ss << L".";
            }
        }
        ss << L"\r\n";
    }

    WNDCLASSW wc = {}; wc.lpfnWndProc = MemoryDumpProc; wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"MemoryDumpClass"; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); RegisterClassW(&wc);
    DumpContext* ctx = new DumpContext{ ss.str() };
    CreateWindowExW(0, L"MemoryDumpClass", L"Memory Dump", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 700, 400, m_hwnd, NULL, GetModuleHandle(NULL), ctx);
}

LRESULT CALLBACK RenderWindow::MemoryDumpProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit = NULL; static HFONT hFont = NULL;
    switch (uMsg) {
        case WM_NCCREATE: {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam; DumpContext* ctx = (DumpContext*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx); return TRUE;
        }
        case WM_CREATE: {
            DumpContext* ctx = (DumpContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            hEdit = CreateWindowExW(0, L"EDIT", ctx->text.c_str(), WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
            hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE); delete ctx; return 0;
        }
        case WM_SIZE: { SetWindowPos(hEdit, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER); return 0; }
        case WM_DESTROY: { if (hFont) DeleteObject(hFont); return 0; }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void RenderWindow::SortProcesses() {
    int sortMode = SendMessageW(m_hSortCombo, CB_GETCURSEL, 0, 0);

    // Використовуємо лямбда-функцію для кастомного сортування
    std::sort(m_filteredData.begin(), m_filteredData.end(), [sortMode](const DataItem& a, const DataItem& b) {
        if (sortMode == 0) {
            // За алфавітом
            return a.processName < b.processName;
        }
        else if (sortMode == 1) {
            // За PID
            return a.processID < b.processID;
        }
        else if (sortMode == 2) {
            // Найстаріші (FIFO - перші запущені)
            ULARGE_INTEGER ta, tb;
            ta.LowPart = a.creationTime.dwLowDateTime; ta.HighPart = a.creationTime.dwHighDateTime;
            tb.LowPart = b.creationTime.dwLowDateTime; tb.HighPart = b.creationTime.dwHighDateTime;

            if (ta.QuadPart == 0) return false; // Ховаємо процеси без доступу вниз
            if (tb.QuadPart == 0) return true;
            return ta.QuadPart < tb.QuadPart;
        }
        else {
            // Найновіші (LIFO - останні запущені)
            ULARGE_INTEGER ta, tb;
            ta.LowPart = a.creationTime.dwLowDateTime; ta.HighPart = a.creationTime.dwHighDateTime;
            tb.LowPart = b.creationTime.dwLowDateTime; tb.HighPart = b.creationTime.dwHighDateTime;
            return ta.QuadPart > tb.QuadPart;
        }
    });

    ListView_SetItemCount(m_hListView, m_filteredData.size());
    InvalidateRect(m_hListView, NULL, TRUE);
}
