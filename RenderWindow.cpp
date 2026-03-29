#include "RenderWindow.h"

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

    m_hwnd = CreateWindowExW(
        0, L"MyRendererClass", title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, this
    );

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

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



void RenderWindow::AddData(const std::wstring& name, DWORD pid) {
    m_allData.push_back({name, pid});
    ListView_SetItemCount(m_hListView, m_allData.size()); // Оновлюємо віртуальний список
}

void RenderWindow::LoadProcesses() {
    m_allData.clear(); // Очищаємо головну базу

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &processEntry)) {
            do {
                m_allData.push_back({processEntry.szExeFile, processEntry.th32ProcessID});
            } while (Process32NextW(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);
    }

    // Після завантаження всіх процесів відразу запускаємо фільтр (щоб відобразити їх)
    FilterData();
}

LRESULT RenderWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            SetupControls();
            LoadProcesses(); // Отримуємо та виводимо процеси під час запуску
            return 0;
        }
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            int sideWidth = std::max(width / 6, 160); // Ліва панель
            int mainX = sideWidth + 10;
            int mainWidth = width - mainX - 10;
            int listWidth = width / 6;

            int listHeight = (height - 110) / 2;

            // --- Ліва панель ---
            SetWindowPos(m_hSearchCombo, NULL, 5, 5, sideWidth - 10, 25, SWP_NOZORDER);
            SetWindowPos(m_hSearchEdit, NULL, 5, 35, sideWidth - 10, 25, SWP_NOZORDER);
            SetWindowPos(m_hListView, NULL, 5, 65, sideWidth - 10, height - 65 - 40, SWP_NOZORDER);
            SetWindowPos(m_hLaunchBtn, NULL, 5, height - 35, sideWidth - 10, 30, SWP_NOZORDER);

            // Адаптуємо колонки лівого списку
            ListView_SetColumnWidth(m_hListView, 0, listWidth - 70 - 15);
            ListView_SetColumnWidth(m_hListView, 1, 60);

            // --- Права панель ---
            SetWindowPos(m_hPidLabel, NULL, mainX, 5, mainWidth, 20, SWP_NOZORDER);
            SetWindowPos(m_hScanEdit, NULL, mainX, 30, 80, 25, SWP_NOZORDER);
            SetWindowPos(m_hTypeCombo, NULL, mainX + 90, 30, 120, 25, SWP_NOZORDER);
            SetWindowPos(m_hScanBtn, NULL, mainX + 220, 30, 140, 25, SWP_NOZORDER);
            SetWindowPos(m_hNextScanBtn, NULL, mainX + 370, 30, 120, 25, SWP_NOZORDER);

            // Два списки ділять висоту навпіл
            SetWindowPos(m_hResultsList, NULL, mainX, 65, mainWidth, listHeight, SWP_NOZORDER);
            SetWindowPos(m_hSavedList, NULL, mainX, 65 + listHeight + 10, mainWidth, listHeight, SWP_NOZORDER);

            return 0;
        }

        case WM_NOTIFY: {
            LPNMHDR lpnmh = (LPNMHDR)lParam;

            if (lpnmh->idFrom == 1001 && lpnmh->code == LVN_GETDISPINFOW) {
            NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)lParam;

            if (pDispInfo->item.mask & LVIF_TEXT) {
                int row = pDispInfo->item.iItem;
                int col = pDispInfo->item.iSubItem;

                // ВАЖЛИВО: Тепер ми беремо дані з m_filteredData!
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
                        if (col == 0) { // Вивід адреси у HEX форматі
                            swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"0x%p", (void*)m_scanResults[row]);
                        } else if (col == 1) { // Вивід значення (перевіряємо ще раз RPM)
                            int currentVal = 0;
                            HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, m_selectedPID);
                            ReadProcessMemory(hProc, (LPCVOID)m_scanResults[row], &currentVal, sizeof(int), NULL);
                            CloseHandle(hProc);
                            swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%d", currentVal);
                        }
                    } else if (col == 1) {
                        HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, m_selectedPID);
                        if (hProc) {
                            // ЧИТАЄМО І ВИВОДИМО ЗАЛЕЖНО ВІД ПОТОЧНОГО ТИПУ
                            switch (m_currentScanType) {
                                case TYPE_INT: {
                                    int val = 0;
                                    ReadProcessMemory(hProc, (LPCVOID)m_scanResults[row], &val, sizeof(int), NULL);
                                    swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%d", val);
                                    break;
                                }
                                case TYPE_FLOAT: {
                                    float val = 0.0f;
                                    ReadProcessMemory(hProc, (LPCVOID)m_scanResults[row], &val, sizeof(float), NULL);
                                    swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%.2f", val); // %.2f - дві цифри після коми
                                    break;
                                }
                                case TYPE_BYTE: {
                                    BYTE val = 0;
                                    ReadProcessMemory(hProc, (LPCVOID)m_scanResults[row], &val, sizeof(BYTE), NULL);
                                    swprintf(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, L"%u", val); // %u - беззнакове ціле
                                    break;
                                }
                            }
                            CloseHandle(hProc);
                        }
                    }

                }
            }

            if (lpnmh->idFrom == 1001 && lpnmh->code == NM_RCLICK) {
                LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
                // ВАЖЛИВО: Отримуємо PID також з m_filteredData!

                if (pnmitem->iItem >= 0 && pnmitem->iItem < m_filteredData.size()) {
                    m_selectedPID = m_filteredData[pnmitem->iItem].processID;
                    UpdatePidLabel();

                    // 2. Створюємо пусте контекстне меню
                    HMENU hMenu = CreatePopupMenu();

                    // 3. Додаємо пункти меню
                    AppendMenuW(hMenu, MF_STRING, IDM_READ_MEM, L"Читати пам'ять");
                    AppendMenuW(hMenu, MF_STRING, IDM_WRITE_MEM, L"Записати пам'ять");

                    // Можна додати розділювач (лінію)
                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                    AppendMenuW(hMenu, MF_STRING, 4003, L"Властивості (для прикладу)");

                    // 4. Отримуємо координати курсора миші на екрані (TrackPopupMenu вимагає екранні координати)
                    POINT pt;
                    GetCursorPos(&pt);

                    // 5. Показуємо меню.
                    // TPM_RETURNCMD означає, що меню поверне ID вибраного пункту, або 0, якщо клікнули повз.
                    // Ми відправляємо результат прямо у наше вікно (m_hwnd).
                    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN,
                                   pt.x, pt.y, 0, m_hwnd, NULL);

                    // 6. Обов'язково видаляємо меню з пам'яті після того, як воно закрилося
                    DestroyMenu(hMenu);
                }
            }
            // ПОДВІЙНИЙ КЛІК ПО ЗБЕРЕЖЕНИХ АДРЕСАХ (Запис нового значення)
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
            // ПРАВИЙ КЛІК ПО ЗБЕРЕЖЕНИМ АДРЕСАМ (Виклик Memory Dump)
            if (lpnmh->idFrom == IDC_SAVED_LIST && lpnmh->code == NM_RCLICK) {
                LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;

                if (pnmitem->iItem >= 0 && pnmitem->iItem < m_savedAddresses.size()) {
                    // Викликаємо наш новий метод, передаючи йому адресу!
                    ShowMemoryDump(m_savedAddresses[pnmitem->iItem]);
                }
            }

            if (lpnmh->idFrom == IDC_RESULTS_LIST && lpnmh->code == NM_DBLCLK) {
                LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
                if (pnmitem->iItem >= 0 && pnmitem->iItem < m_scanResults.size()) {
                    // Копіюємо адресу вниз
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

            // Якщо користувач ВВІВ ТЕКСТ у поле пошуку
            if (wmId == IDC_SEARCH_EDIT && wmEvent == EN_CHANGE) {
                FilterData();
            }
            // Якщо користувач ЗМІНИВ КРИТЕРІЙ у списку (Назва/PID)
            else if (wmId == IDC_SEARCH_COMBO && wmEvent == CBN_SELCHANGE) {
                FilterData();
                SetFocus(m_hSearchEdit); // Повертаємо фокус назад на поле вводу для зручності
            }
            // Перевіряємо, чи вибрали щось із нашого контекстного меню
            if (wmId == IDM_READ_MEM) {
                std::wstring msg = L"Вибрано ЧИТАННЯ для PID: " + std::to_wstring(m_selectedPID);
                MessageBoxW(m_hwnd, msg.c_str(), L"Дія контекстного меню", MB_OK | MB_ICONINFORMATION);
            }
            else if (wmId == IDM_WRITE_MEM) {
                std::wstring msg = L"Вибрано ЗАПИС для PID: " + std::to_wstring(m_selectedPID);
                MessageBoxW(m_hwnd, msg.c_str(), L"Дія контекстного меню", MB_OK | MB_ICONINFORMATION);
            }
            else if (wmId == IDC_SCAN_BUTTON || wmId == IDC_NEXT_SCAN_BUTTON) {
                if (m_selectedPID == 0) {
                    MessageBoxW(m_hwnd, L"Спочатку виберіть процес!", L"Увага", MB_ICONWARNING);
                    return 0;
                }

                wchar_t buf[32];
                GetWindowTextW(m_hScanEdit, buf, 32);

                // Дізнаємось вибраний тип
                int selectedType = SendMessageW(m_hTypeCombo, CB_GETCURSEL, 0, 0);
                m_currentScanType = (ScanDataType)selectedType; // Запам'ятовуємо

                SetCursor(LoadCursor(NULL, IDC_WAIT));

                // Робимо магію залежно від типу
                if (m_currentScanType == TYPE_INT) {
                    int val = _wtoi(buf);
                    if (wmId == IDC_SCAN_BUTTON) m_scanResults = ScanMemory<int>(m_selectedPID, val);
                    else m_scanResults = NextScanMemory<int>(m_selectedPID, val);
                }
                else if (m_currentScanType == TYPE_FLOAT) {
                    float val = (float)_wtof(buf); // Читаємо як float
                    if (wmId == IDC_SCAN_BUTTON) m_scanResults = ScanMemory<float>(m_selectedPID, val);
                    else m_scanResults = NextScanMemory<float>(m_selectedPID, val);
                }
                else if (m_currentScanType == TYPE_BYTE) {
                    BYTE val = (BYTE)_wtoi(buf); // 1 байт - це число від 0 до 255
                    if (wmId == IDC_SCAN_BUTTON) m_scanResults = ScanMemory<BYTE>(m_selectedPID, val);
                    else m_scanResults = NextScanMemory<BYTE>(m_selectedPID, val);
                }

                SetCursor(LoadCursor(NULL, IDC_ARROW));

                ListView_SetItemCount(m_hResultsList, m_scanResults.size());
                InvalidateRect(m_hResultsList, NULL, TRUE);
            }

            else if (wmId == IDC_BTN_LAUNCH_TEST) {
    // 1. Наш заготовлений код манекена у вигляді тексту (Raw String)
    const char* dummySourceCode = R"(
#include <iostream>
#include <windows.h>
#include <conio.h>

int main() {
    SetConsoleOutputCP(1251);
    int playerMoney = 100;

    std::cout << "maneken\n";
    std::cout << "PID: " << GetCurrentProcessId() << "\n\n";

    while (true) {
        std::cout << "Баланс: " << playerMoney << " | Адреса (HEX): 0x" << std::hex << &playerMoney << std::dec << "\n";
        std::cout << "[ПРОБІЛ] -10 монет | [R] Скинути | [ESC] Вихід\n";

        char key = _getch();
        if (key == ' ') playerMoney -= 10;
        else if (key == 'r' || key == 'R') playerMoney = 100;
        else if (key == 27) break;

        system("cls");
        std::cout << "maneken\n";
        std::cout << "PID: " << GetCurrentProcessId() << "\n\n";
    }
    return 0;
}
    )";

    // 2. Створюємо вихідний файл .cpp на диску
    std::ofstream outFile("GeneratedDummy.cpp");
    if (outFile.is_open()) {
        outFile << dummySourceCode;
        outFile.close();
    }
                else {
        MessageBoxW(m_hwnd, L"Не вдалося створити файл GeneratedDummy.cpp", L"Помилка", MB_ICONERROR);
        return 0;
    }

    // Змінюємо курсор, бо компіляція займе 1-2 секунди
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    // 3. Викликаємо компілятор g++ для збірки нашого коду
    // Функція system() блокує виконання нашої програми, поки компіляція не завершиться
    int compileResult = system("g++ GeneratedDummy.cpp -o GeneratedDummy.exe");

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    if (compileResult != 0) {
        MessageBoxW(m_hwnd, L"Помилка компіляції! Переконайтеся, що g++ додано до PATH вашої системи.", L"Помилка", MB_ICONERROR);
        return 0;
    }

    // 4. Якщо компіляція успішна - запускаємо створений GeneratedDummy.exe
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (CreateProcessW(L"GeneratedDummy.exe", NULL, NULL, NULL, FALSE,
                       CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {

        // Оновлюємо список процесів
        LoadProcesses();

        // Автоматично наводимо сканер на новий процес
        m_selectedPID = pi.dwProcessId;

        UpdatePidLabel();

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        std::wstring msg = L"Манекен згенеровано, скомпільовано і запущено!\nPID: " + std::to_wstring(m_selectedPID);
        MessageBoxW(m_hwnd, msg.c_str(), L"Успіх", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(m_hwnd, L"Не вдалося запустити згенерований файл GeneratedDummy.exe", L"Помилка", MB_ICONERROR);
    }
}

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void RenderWindow::SetupControls() {
    // 1. Створюємо випадаючий список для вибору критерію
    m_hSearchCombo = CreateWindowExW(0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 0, 150, // Висота 150 - це висота списку, коли він відкритий
        m_hwnd, (HMENU)IDC_SEARCH_COMBO, NULL, NULL);

    SendMessageW(m_hSearchCombo, CB_ADDSTRING, 0, (LPARAM)L"Шукати по назві");
    SendMessageW(m_hSearchCombo, CB_ADDSTRING, 0, (LPARAM)L"Шукати по PID");
    SendMessageW(m_hSearchCombo, CB_SETCURSEL, 0, 0); // Вибираємо перший пункт за замовчуванням

    // 2. Створюємо поле для вводу тексту
    m_hSearchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        m_hwnd, (HMENU)IDC_SEARCH_EDIT, NULL, NULL);

    // 3. Створюємо наш список (той самий код)
    m_hListView = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA,
        0, 0, 0, 0, m_hwnd, (HMENU)1001, NULL, NULL);

    ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;

    m_hPidLabel = CreateWindowExW(0, L"STATIC", L"Вибраний процес (PID): Немає",
    WS_CHILD | WS_VISIBLE | SS_LEFT, // SS_LEFT - вирівнювання по лівому краю
    0, 0, 0, 0, m_hwnd, (HMENU)IDC_PID_LABEL, NULL, NULL);

    lvc.iSubItem = 0; lvc.cx = 100; lvc.pszText = (LPWSTR)L"Назва";
    SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);

    lvc.iSubItem = 1; lvc.cx = 60; lvc.pszText = (LPWSTR)L"PID";
    SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

    m_hScanEdit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"100",
            WS_CHILD | WS_VISIBLE | ES_NUMBER, // Тільки цифри
            0, 0, 0, 0, m_hwnd, (HMENU)IDC_SCAN_EDIT, NULL, NULL);

    // 2. Кнопка сканування
    m_hScanBtn = CreateWindowExW(0, L"BUTTON", L"Перше сканування",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, m_hwnd, (HMENU)IDC_SCAN_BUTTON, NULL, NULL);

    // 3. Список результатів (теж віртуальний LVS_OWNERDATA)
    m_hResultsList = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA,
        0, 0, 0, 0, m_hwnd, (HMENU)IDC_RESULTS_LIST, NULL, NULL);

    m_hNextScanBtn = CreateWindowExW(0, L"BUTTON", L"Відсіяти (Next)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, m_hwnd, (HMENU)IDC_NEXT_SCAN_BUTTON, NULL, NULL);


    ListView_SetExtendedListViewStyle(m_hResultsList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // Колонки для списку результатів
    LVCOLUMNW lvc1 = {0};
    lvc1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvc1.fmt = LVCFMT_LEFT;

    lvc1.cx = 140; lvc1.pszText = (LPWSTR)L"Адреса";
    SendMessageW(m_hResultsList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc1);

    lvc1.cx = 100; lvc1.pszText = (LPWSTR)L"Значення";
    SendMessageW(m_hResultsList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc1);



    // Список збережених адрес
    m_hSavedList = CreateWindowExW(0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA,
        0, 0, 0, 0, m_hwnd, (HMENU)IDC_SAVED_LIST, NULL, NULL);

    ListView_SetExtendedListViewStyle(m_hSavedList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    m_hLaunchBtn = CreateWindowExW(0, L"BUTTON", L" Запустити манекен",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, m_hwnd, (HMENU)IDC_BTN_LAUNCH_TEST, NULL, NULL);

    LVCOLUMNW lvcSaved = {0};
    lvcSaved.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvcSaved.fmt = LVCFMT_LEFT;

    lvcSaved.cx = 140; lvcSaved.pszText = (LPWSTR)L"Збережена Адреса";
    SendMessageW(m_hSavedList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvcSaved);

    lvcSaved.cx = 100; lvcSaved.pszText = (LPWSTR)L"Поточне Значення";
    SendMessageW(m_hSavedList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvcSaved);


    m_hTypeCombo = CreateWindowExW(0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 0, 150, m_hwnd, (HMENU)IDC_TYPE_COMBO, NULL, NULL);

    SendMessageW(m_hTypeCombo, CB_ADDSTRING, 0, (LPARAM)L"4 Байти (Int)");
    SendMessageW(m_hTypeCombo, CB_ADDSTRING, 0, (LPARAM)L"Float (Дробове)");
    SendMessageW(m_hTypeCombo, CB_ADDSTRING, 0, (LPARAM)L"1 Байт (Byte)");
    SendMessageW(m_hTypeCombo, CB_SETCURSEL, 0, 0); // За замовчуванням - Int

}


void RenderWindow::FilterData() {
    // 1. Читаємо текст, який ввів користувач
    int len = GetWindowTextLengthW(m_hSearchEdit);
    std::wstring query(len, L'\0');
    GetWindowTextW(m_hSearchEdit, &query[0], len + 1);
    if (len > 0) query.resize(len);

    // Робимо запит маленькими літерами для незалежності від регістру
    std::transform(query.begin(), query.end(), query.begin(), ::towlower);

    // 2. Дізнаємось, що вибрано в Комбобоксі (0 - Назва, 1 - PID)
    int filterMode = SendMessageW(m_hSearchCombo, CB_GETCURSEL, 0, 0);

    // 3. Очищаємо список для відображення
    m_filteredData.clear();

    // 4. Фільтруємо
    for (const auto& item : m_allData) {
        if (query.empty()) {
            // Якщо поле порожнє - додаємо все
            m_filteredData.push_back(item);
        } else {
            if (filterMode == 0) { // Пошук по НАЗВІ
                std::wstring nameLower = item.processName;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::towlower);

                if (nameLower.find(query) != std::wstring::npos) {
                    m_filteredData.push_back(item);
                }
            }
            else if (filterMode == 1) { // Пошук по PID
                std::wstring pidStr = std::to_wstring(item.processID);
                if (pidStr.find(query) != std::wstring::npos) {
                    m_filteredData.push_back(item);
                }
            }
        }
    }

    // 5. Оновлюємо віртуальний список новими даними
    ListView_SetItemCount(m_hListView, m_filteredData.size());
    InvalidateRect(m_hListView, NULL, TRUE); // Кажемо вікну перемалюватися
}

void RenderWindow::UpdatePidLabel() {
    if (m_selectedPID == 0) {
        SetWindowTextW(m_hPidLabel, L"Вибраний процес (PID): Немає");
    } else {
        std::wstring text = L" Цільовий процес (PID): " + std::to_wstring(m_selectedPID);
        SetWindowTextW(m_hPidLabel, text.c_str());
    }
}



void RenderWindow::ShowMemoryDump(uintptr_t targetAddress) {
    if (m_selectedPID == 0) return;

    // Читаємо 256 байт. Відступимо на 32 байти назад від нашої адреси,
    // щоб цільове значення було десь посередині екрану.
    const SIZE_T dumpSize = 256;
    uintptr_t readAddress = targetAddress - 32;
    std::vector<BYTE> buffer(dumpSize, 0);

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, m_selectedPID);
    if (!hProcess) {
        MessageBoxW(m_hwnd, L"Не вдалося відкрити процес для читання!", L"Помилка", MB_ICONERROR);
        return;
    }

    SIZE_T bytesRead = 0;
    ReadProcessMemory(hProcess, (LPCVOID)readAddress, buffer.data(), dumpSize, &bytesRead);
    CloseHandle(hProcess);

    // ФОРМАТУВАННЯ ТЕКСТУ (Магія Hex Editor'а)
    std::wstringstream ss;
    for (SIZE_T i = 0; i < bytesRead; i += 16) {
        // 1. Колонка адреси
        ss << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << (readAddress + i) << L" | ";

        // 2. Колонка HEX байтів
        for (SIZE_T j = 0; j < 16; ++j) {
            if (i + j < bytesRead) {
                ss << std::hex << std::setw(2) << std::setfill(L'0') << (int)buffer[i + j] << L" ";
            } else {
                ss << L"   "; // Порожнє місце, якщо байти закінчилися
            }
            if (j == 7) ss << L" "; // Додатковий пробіл посередині для зручності
        }

        ss << L" | ";

        // 3. Колонка ASCII (зрозумілий текст)
        for (SIZE_T j = 0; j < 16; ++j) {
            if (i + j < bytesRead) {
                char c = buffer[i + j];
                // Виводимо тільки друковані символи (коди від 32 до 126)
                if (c >= 32 && c <= 126) {
                    ss << (wchar_t)c;
                } else {
                    ss << L"."; // Заміняємо незрозумілі байти крапкою
                }
            }
        }
        ss << L"\r\n"; // Перехід на новий рядок
    }

    // Реєструємо клас вікна (якщо ще не зареєстрований)
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MemoryDumpProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"MemoryDumpClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    // Створюємо контекст і передаємо туди наш готовий текст
    DumpContext* ctx = new DumpContext{ ss.str() };

    // Створюємо вікно
    CreateWindowExW(
        0, L"MemoryDumpClass", L"Memory Dump",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 400, // Розмір вікна
        m_hwnd, NULL, GetModuleHandle(NULL), ctx
    );
}

LRESULT CALLBACK RenderWindow::MemoryDumpProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit = NULL;
    static HFONT hFont = NULL;

    switch (uMsg) {
        case WM_NCCREATE: {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            DumpContext* ctx = (DumpContext*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
            return TRUE;
        }

        case WM_CREATE: {
            DumpContext* ctx = (DumpContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

            // Створюємо багаторядкове поле вводу ТІЛЬКИ ДЛЯ ЧИТАННЯ (ES_READONLY)
            hEdit = CreateWindowExW(0, L"EDIT", ctx->text.c_str(),
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                0, 0, 0, 0, hwnd, NULL, NULL, NULL);

            // МАГІЯ: Створюємо моноширинний шрифт Consolas
            hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

            // Застосовуємо шрифт до текстового поля
            SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            delete ctx; // Очищаємо пам'ять
            return 0;
        }

        case WM_SIZE: {
            // Розтягуємо текстове поле на все вікно
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            SetWindowPos(hEdit, NULL, 0, 0, width, height, SWP_NOZORDER);
            return 0;
        }

        case WM_DESTROY: {
            if (hFont) DeleteObject(hFont); // Видаляємо шрифт із пам'яті
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


uintptr_t RenderWindow::PatternScan(DWORD pid, const char* pattern, const char* mask) {
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return 0;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    uintptr_t currentAddr = (uintptr_t)sysInfo.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)sysInfo.lpMaximumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;

    size_t patternLen = strlen(mask);

    // Проходимося по всіх блоках пам'яті
    while (currentAddr < maxAddr) {
        if (VirtualQueryEx(hProcess, (LPCVOID)currentAddr, &mbi, sizeof(mbi)) == 0) break;

        // Шукаємо ТІЛЬКИ в пам'яті, де лежить ВИКОНУВАНИЙ КОД (PAGE_EXECUTE_READ або PAGE_EXECUTE_READWRITE)
        if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_EXECUTE_READ || mbi.Protect & PAGE_EXECUTE_READWRITE)) {

            std::vector<BYTE> buffer(mbi.RegionSize);
            SIZE_T bytesRead = 0;

            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {

                // Проходимося по буферу і намагаємося накласти нашу маску
                for (SIZE_T i = 0; i < bytesRead - patternLen; i++) {
                    bool found = true;

                    for (SIZE_T j = 0; j < patternLen; j++) {
                        // Якщо маска 'x' і байти не збігаються - це не наш код
                        if (mask[j] == 'x' && buffer[i + j] != (BYTE)pattern[j]) {
                            found = false;
                            break;
                        }

                    }

                    if (found) {
                        CloseHandle(hProcess);
                        return (uintptr_t)mbi.BaseAddress + i; // Повертаємо адресу в пам'яті
                    }
                }
            }
        }
        currentAddr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }

    CloseHandle(hProcess);
    return 0; // Не знайдено
}