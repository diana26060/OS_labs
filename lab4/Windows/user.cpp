#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <windows.h>
#include <tlhelp32.h>
#include <process.h>
#include <locale>
#include <codecvt>
#include <algorithm>

class ProcessManager {
public:
    std::wstring stringToWstring(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    bool isProcessRunning(const std::string& name) {
        std::wstring searchName = stringToWstring(name);
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::towlower);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wstring processName = pe32.szExeFile;
                std::transform(processName.begin(), processName.end(), processName.begin(), ::towlower);

                if (processName.find(searchName) != std::wstring::npos) {
                    CloseHandle(hSnapshot);
                    return true;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
        return false;
    }

    bool isProcessRunning(int pid) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProcess == NULL) {
            return false;
        }

        DWORD exitCode;
        GetExitCodeProcess(hProcess, &exitCode);
        CloseHandle(hProcess);

        return (exitCode == STILL_ACTIVE);
    }

    bool startProcess(const std::string& command) {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        char cmd[1024];
        strcpy_s(cmd, command.c_str());

        if (!CreateProcessA(
            NULL,
            cmd,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi)
            ) {
            std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return true;
    }

    int getProcessId(const std::string& name) {
        std::wstring searchName = stringToWstring(name);
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::towlower);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return -1;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wstring processName = pe32.szExeFile;
                std::transform(processName.begin(), processName.end(), processName.begin(), ::towlower);

                if (processName.find(searchName) != std::wstring::npos) {
                    int pid = pe32.th32ProcessID;
                    CloseHandle(hSnapshot);
                    return pid;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
        return -1;
    }
};

void demoKiller() {
    ProcessManager pm;

    std::cout << "\n Setting PROC_TO_KILL environment variable " << std::endl;
    SetEnvironmentVariableA("PROC_TO_KILL", "notepad.exe,calc.exe");

    std::cout << "\n Starting test processes " << std::endl;
    pm.startProcess("notepad.exe");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    pm.startProcess("calc.exe");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n Checking processes before kill " << std::endl;
    if (pm.isProcessRunning("notepad.exe")) {
        std::cout << "Process 'notepad.exe' is running" << std::endl;
    }

    std::cout << "\n Testing --name parameter " << std::endl;
    system("killer.exe --name notepad");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n Testing --env parameter " << std::endl;
    system("killer.exe --env");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n Checking processes after kill " << std::endl;
    if (!pm.isProcessRunning("notepad.exe")) {
        std::cout << "Process 'notepad.exe' terminated" << std::endl;
    }

    std::cout << "\n Testing --id parameter " << std::endl;
    std::cout << "Starting test process..." << std::endl;
    pm.startProcess("timeout 30");

    std::this_thread::sleep_for(std::chrono::seconds(2));

    int testPid = pm.getProcessId("timeout.exe");
    if (testPid > 0) {
        std::cout << "Test process PID: " << testPid << std::endl;

        if (pm.isProcessRunning(testPid)) {
            std::cout << "Process " << testPid << " is running" << std::endl;
        }

        std::string cmd = "killer.exe --id " + std::to_string(testPid);
        system(cmd.c_str());

        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!pm.isProcessRunning(testPid)) {
            std::cout << "Process " << testPid << " terminated" << std::endl;
        }
    }

    std::cout << "\n Cleaning up" << std::endl;
    SetEnvironmentVariableA("PROC_TO_KILL", NULL);

    std::cout << "\n Demonstration completed " << std::endl;
}

int main() {
    std::cout << " Process Killer Demonstration " << std::endl;

    demoKiller();

    return 0;
}