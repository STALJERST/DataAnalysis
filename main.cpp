#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <io.h>    // _setmode
#include <fcntl.h> //_O_U16TEXT
#include <conio.h>

int main() {

    _setmode(_fileno(stdout), _O_U16TEXT);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cout << "Помилка створення знімку!" << std::endl;
        return 1;
    }

    PROCESSENTRY32W processEntry; // Структура, куди запишуться дані про процес
    processEntry.dwSize = sizeof(PROCESSENTRY32W);

    // 2. Отримуємо перший процес
    if (Process32FirstW(hSnapshot, &processEntry)) {  //присвоює початкове значення з якого ітеруватися
        std::cout << "Список запущених процесів:" << std::endl;
        std::cout << "-----------------------------------" << std::endl;

        do {
            // Виводимо назву (szExeFile) та ID процесу (th32ProcessID)
            std::wcout << L"PID: " << processEntry.th32ProcessID
                       << L"\tНазва: " << processEntry.szExeFile << std::endl;

        } while (Process32NextW(hSnapshot, &processEntry)); // 3. Йдемо по списку далі +1
    }

    // Обов'язково закриваємо дескриптор, щоб не було витоку пам'яті
    CloseHandle(hSnapshot);

    return 0;
}