#include <windows.h>
#include <string>

int main() {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    char buffer;
    DWORD bytesRead, bytesWritten;
    std::string number;
    while (ReadFile(hStdin, &buffer, 1, &bytesRead, NULL) && bytesRead > 0) {
        if (buffer == ' ' || buffer == '\n' || buffer == '\r') {
            if (!number.empty()) {
                int x = std::stoi(number);
                std::string res = std::to_string(x * x * x) + " ";
                WriteFile(hStdout, res.c_str(), (DWORD)res.length(), &bytesWritten, NULL);
                number.clear();
            }
            if (buffer == '\n') break;
        }
        else if (isdigit(buffer)) number += buffer;
    }
    WriteFile(hStdout, "\n", 1, &bytesWritten, NULL);
    return 0;
}