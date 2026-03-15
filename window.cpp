#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h> // Для роботи з процесами
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <algorithm> // Для std::transform
#include <cwctype>   // Для ::towlower

#define IDM_READ_MEM 4001
#define IDM_WRITE_MEM 4002
#define IDC_SEARCH_EDIT 5001
#define IDC_SEARCH_COMBO 5002
// Структура для зберігання даних про процес
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

private:
    HWND m_hwnd;
    HWND m_hListView;
    HWND m_hSearchEdit;  // Поле для вводу тексту
    HWND m_hSearchCombo; // Випадаючий список (По назві / По PID)

    std::vector<DataItem> m_allData;      // ГОЛОВНА база всіх процесів
    std::vector<DataItem> m_filteredData; // ВІДФІЛЬТРОВАНА база для відображення

    DWORD m_selectedPID = 0; // Зберігаємо PID процесу, по якому клікнули

    void SetupControls();
    void FilterData();

    static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

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
            // Змінюємо розмір списку при розширенні вікна
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);

            // Вираховуємо 1/6 ширини
            int listWidth = width / 6;

            SetWindowPos(m_hSearchCombo, NULL, 5, 5, listWidth - 10, 25, SWP_NOZORDER);
            SetWindowPos(m_hSearchEdit, NULL, 5, 35, listWidth - 10, 25, SWP_NOZORDER);
            SetWindowPos(m_hListView, NULL, 5, 65, listWidth - 10, height - 70, SWP_NOZORDER);

            // Адаптуємо колонки
            ListView_SetColumnWidth(m_hListView, 0, listWidth - 70 - 15);
            ListView_SetColumnWidth(m_hListView, 1, 60);
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

            if (lpnmh->idFrom == 1001 && lpnmh->code == NM_RCLICK) {
                LPNMITEMACTIVATE pnmitem = (LPNMITEMACTIVATE)lParam;
                // ВАЖЛИВО: Отримуємо PID також з m_filteredData!

                if (pnmitem->iItem >= 0 && pnmitem->iItem < m_filteredData.size()) {
                    m_selectedPID = m_filteredData[pnmitem->iItem].processID;

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

    lvc.iSubItem = 0; lvc.cx = 100; lvc.pszText = (LPWSTR)L"Назва";
    SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);

    lvc.iSubItem = 1; lvc.cx = 60; lvc.pszText = (LPWSTR)L"PID";
    SendMessageW(m_hListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Ініціалізація загальних контролів (обов'язково для ListView)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    RenderWindow window;

    if (!window.Create(L"Data analysis", 1000, 650)) {
        return 0;
    }

    ShowWindow(window.getHWND(), nCmdShow);
    UpdateWindow(window.getHWND());

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}