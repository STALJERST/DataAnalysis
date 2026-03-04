#include <windows.h>
#include <commctrl.h>
#include <wchar.h>
#include <io.h>    // _setmode
#include <fcntl.h> //_O_U16TEXT
#include <stdio.h>

void InsertColumns(HWND hWndListView) {
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT; // Вирівнювання по лівому краю

    // Перша колонка
    lvc.iSubItem = 0;
    lvc.cx = 150;          // Ширина колонки в пікселях
    lvc.pszText = (LPSTR)"Назва програми";
    ListView_InsertColumn(hWndListView, 0, &lvc);

    // Друга колонка
    lvc.iSubItem = 1;
    lvc.cx = 100;
    lvc.pszText = (LPSTR)"Ідентифікатор (PID)";
    ListView_InsertColumn(hWndListView, 1, &lvc);
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;



        HWND hWndListView = CreateWindowEx(
            0,
            WC_LISTVIEW,                // Клас List-View (SysListView32)
            "text",                        // Текст вікна (не використовується для списків)
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS, // LVS_REPORT - режим таблиці
            10, 10,                     // Координати x, y
            400, 300,                   // Ширина та висота
            hwnd,                 // Дескриптор батьківського вікна (головного вікна)
            (HMENU)hInst,      // Ідентифікатор елемента керування
            GetModuleHandle(NULL),
            NULL
        );

        // Додаємо розширені стилі: виділення всього рядка та сітку
        ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        InsertColumns(hWndListView);
    }
        return 0;

    case WM_SIZE:
    {

    }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



// Точка входу для Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    _setmode(_fileno(stdout), _O_U16TEXT);

    // 1. Реєстрація класу вікна
    WNDCLASS wc = {};
    wc.style = //
     CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc  = WindowProc;//
    //wc.cbClsExtra;
    //wc.cbWndExtra;
    wc.hInstance = hInstance;//
    //wc.hIcon;//
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);//
    //wc.hbrBackground;//
    //wc.lpszMenuName;//
    wc.lpszClassName = "3DGR";//

    RegisterClass(&wc);

    // 2. Створення вікна
     HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        "My First Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 800,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );



    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}