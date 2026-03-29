#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h> // Для роботи з процесами
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <algorithm> // Для std::transform
#include <cwctype>   // Для ::towlower
#include "RenderWindow.h"

void SuspendProcess(DWORD targetPID, bool suspend) {
    // Робимо знімок усіх ПОТОКІВ у системі
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot == INVALID_HANDLE_VALUE) return;

    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hThreadSnapshot, &threadEntry)) {
        do {
            // Шукаємо потоки, які належать саме НАШОМУ процесу
            if (threadEntry.th32OwnerProcessID == targetPID) {
                // Відкриваємо потік з правами на зупинку/запуск
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                if (hThread) {
                    if (suspend) {
                        SuspendThread(hThread); // Заморозити
                    } else {
                        ResumeThread(hThread);  // Розморозити
                    }
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hThreadSnapshot, &threadEntry));
    }
    CloseHandle(hThreadSnapshot);
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