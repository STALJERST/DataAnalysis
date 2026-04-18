#include <windows.h>
#include <commctrl.h>
#include <string>
#include "RenderWindow.h"



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