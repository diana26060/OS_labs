#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

const int N = 1;

void process_M() {
    char buffer;
    DWORD bytesRead;

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string number;
    std::string output;

    while (ReadFile(hStdin, &buffer, 1, &bytesRead, NULL) && bytesRead > 0) {
        if (buffer == ' ' || buffer == '\n' || buffer == '\r') {
            if (!number.empty()) {
                int x = std::stoi(number);
                output += std::to_string(x * 7) + " ";
                number.clear();
            }
            if (buffer == '\n') {
                break;
            }
        }
        else if (buffer >= '0' && buffer <= '9') {
            number += buffer;
        }
    }

    DWORD bytesWritten;
    WriteFile(hStdout, output.c_str(), output.length(), &bytesWritten, NULL);
    WriteFile(hStdout, "\n", 1, &bytesWritten, NULL);

    ExitProcess(0);
}

void process_A() {
    char buffer;
    DWORD bytesRead;

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string number;
    std::string output;

    while (ReadFile(hStdin, &buffer, 1, &bytesRead, NULL) && bytesRead > 0) {
        if (buffer == ' ' || buffer == '\n' || buffer == '\r') {
            if (!number.empty()) {
                int x = std::stoi(number);
                output += std::to_string(x + N) + " ";
                number.clear();
            }
            if (buffer == '\n') {
                break;
            }
        }
        else if (buffer >= '0' && buffer <= '9') {
            number += buffer;
        }
    }

    DWORD bytesWritten;
    WriteFile(hStdout, output.c_str(), output.length(), &bytesWritten, NULL);
    WriteFile(hStdout, "\n", 1, &bytesWritten, NULL);

    ExitProcess(0);
}

void process_P() {
    char buffer;
    DWORD bytesRead;

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string number;
    std::string output;

    while (ReadFile(hStdin, &buffer, 1, &bytesRead, NULL) && bytesRead > 0) {
        if (buffer == ' ' || buffer == '\n' || buffer == '\r') {
            if (!number.empty()) {
                int x = std::stoi(number);
                output += std::to_string(x * x * x) + " ";
                number.clear();
            }
            if (buffer == '\n') {
                break;
            }
        }
        else if (buffer >= '0' && buffer <= '9') {
            number += buffer;
        }
    }

    DWORD bytesWritten;
    WriteFile(hStdout, output.c_str(), output.length(), &bytesWritten, NULL);
    WriteFile(hStdout, "\n", 1, &bytesWritten, NULL);

    ExitProcess(0);
}

void process_S() {
    char buffer;
    DWORD bytesRead;

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string number;
    int sum = 0;

    while (ReadFile(hStdin, &buffer, 1, &bytesRead, NULL) && bytesRead > 0) {
        if (buffer == ' ' || buffer == '\n' || buffer == '\r') {
            if (!number.empty()) {
                int x = std::stoi(number);
                sum += x;
                number.clear();
            }
            if (buffer == '\n') {
                break;
            }
        }
        else if (buffer >= '0' && buffer <= '9') {
            number += buffer;
        }
    }

    std::string output = "sum: " + std::to_string(sum) + "\n";
    DWORD bytesWritten;
    WriteFile(hStdout, output.c_str(), output.length(), &bytesWritten, NULL);

    ExitProcess(0);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "M") {
            process_M();
        }
        else if (mode == "A") {
            process_A();
        }
        else if (mode == "P") {
            process_P();
        }
        else if (mode == "S") {
            process_S();
        }
        return 0;
    }

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    HANDLE hPipe1[2], hPipe2[2], hPipe3[2], hPipe4[2];

    CreatePipe(&hPipe1[0], &hPipe1[1], &sa, 0);
    CreatePipe(&hPipe2[0], &hPipe2[1], &sa, 0);
    CreatePipe(&hPipe3[0], &hPipe3[1], &sa, 0);
    CreatePipe(&hPipe4[0], &hPipe4[1], &sa, 0);

    STARTUPINFOA si[4];
    PROCESS_INFORMATION pi[4];

    for (int i = 0; i < 4; i++) {
        ZeroMemory(&si[i], sizeof(STARTUPINFOA));
        si[i].cb = sizeof(STARTUPINFOA);
        ZeroMemory(&pi[i], sizeof(PROCESS_INFORMATION));
    }

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    //M
    si[0].hStdInput = hPipe1[0];
    si[0].hStdOutput = hPipe2[1];
    si[0].hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si[0].dwFlags = STARTF_USESTDHANDLES;

    //A
    si[1].hStdInput = hPipe2[0];
    si[1].hStdOutput = hPipe3[1];
    si[1].hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si[1].dwFlags = STARTF_USESTDHANDLES;

    //P
    si[2].hStdInput = hPipe3[0];
    si[2].hStdOutput = hPipe4[1];
    si[2].hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si[2].dwFlags = STARTF_USESTDHANDLES;

    //S
    si[3].hStdInput = hPipe4[0];
    si[3].hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si[3].hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si[3].dwFlags = STARTF_USESTDHANDLES;
    std::string cmdLine = exePath;

    CreateProcessA(NULL, (LPSTR)(cmdLine + " M").c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si[0], &pi[0]);
    CreateProcessA(NULL, (LPSTR)(cmdLine + " A").c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si[1], &pi[1]);
    CreateProcessA(NULL, (LPSTR)(cmdLine + " P").c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si[2], &pi[2]);
    CreateProcessA(NULL, (LPSTR)(cmdLine + " S").c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si[3], &pi[3]);

    CloseHandle(hPipe1[0]);
    CloseHandle(hPipe2[0]);
    CloseHandle(hPipe2[1]);
    CloseHandle(hPipe3[0]);
    CloseHandle(hPipe3[1]);
    CloseHandle(hPipe4[0]);
    CloseHandle(hPipe4[1]);

    std::cout << "enter nums: ";
    std::string input;
    std::getline(std::cin, input);

    DWORD bytesWritten;
    std::string data = input + "\n";
    WriteFile(hPipe1[1], data.c_str(), data.length(), &bytesWritten, NULL);
    CloseHandle(hPipe1[1]);

    WaitForSingleObject(pi[0].hProcess, INFINITE);
    WaitForSingleObject(pi[1].hProcess, INFINITE);
    WaitForSingleObject(pi[2].hProcess, INFINITE);
    WaitForSingleObject(pi[3].hProcess, INFINITE);

    for (int i = 0; i < 4; i++) {
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    return 0;
}