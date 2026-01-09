#include <windows.h>
#include <string>

int main() {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    char buffer;
    DWORD bytesRead, bytesWritten;
    std::string number;
    int sum = 0;
    while (ReadFile(hStdin, &buffer, 1, &bytesRead, NULL) && bytesRead > 0) {
        if (buffer == ' ' || buffer == '\n' || buffer == '\r') {
            if (!number.empty()) {
                sum += std::stoi(number);
                number.clear();
            }
            if (buffer == '\n') break;
        }
        else if (isdigit(buffer)) number += buffer;
    }
    std::string res = "sum: " + std::to_string(sum) + "\n";
    WriteFile(hStdout, res.c_str(), (DWORD)res.length(), &bytesWritten, NULL);
    return 0;
}