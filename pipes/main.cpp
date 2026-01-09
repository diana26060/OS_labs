#include <windows.h>
#include <iostream>
#include <string>

HANDLE Start(const char* path, HANDLE in, HANDLE out) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = in;
    si.hStdOutput = out;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (CreateProcessA(NULL, (LPSTR)path, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        return pi.hProcess;
    }
    return NULL;
}

int main() {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE p1[2], p2[2], p3[2], p4[2];
    CreatePipe(&p1[0], &p1[1], &sa, 0);
    CreatePipe(&p2[0], &p2[1], &sa, 0);
    CreatePipe(&p3[0], &p3[1], &sa, 0);
    CreatePipe(&p4[0], &p4[1], &sa, 0);

    HANDLE h[4];
    h[0] = Start("process_M.exe", p1[0], p2[1]);
    h[1] = Start("process_A.exe", p2[0], p3[1]);
    h[2] = Start("process_P.exe", p3[0], p4[1]);
    h[3] = Start("process_S.exe", p4[0], GetStdHandle(STD_OUTPUT_HANDLE));

    CloseHandle(p1[0]);
    CloseHandle(p2[0]); CloseHandle(p2[1]);
    CloseHandle(p3[0]); CloseHandle(p3[1]);
    CloseHandle(p4[0]); CloseHandle(p4[1]);

    std::cout << "Enter nums: ";
    std::string in;
    std::getline(std::cin, in);
    in += "\n";
    DWORD wr;
    WriteFile(p1[1], in.c_str(), (DWORD)in.length(), &wr, NULL);
    CloseHandle(p1[1]);

    WaitForSingleObject(h[3], INFINITE);
    for (int i = 0; i < 4; i++) CloseHandle(h[i]);
    return 0;
}