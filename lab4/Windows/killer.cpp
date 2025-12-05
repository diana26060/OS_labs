#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <windows.h>
#include <tlhelp32.h>
#include <process.h>
#include <locale>
#include <codecvt>

class ProcessKiller {
public:
    bool killById(int pid) {
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess == NULL) {
            std::cerr << "Cannot open process " << pid << std::endl;
            return false;
        }

        BOOL result = TerminateProcess(hProcess, 1);
        CloseHandle(hProcess);

        if (!result) {
            std::cerr << "Failed to terminate " << pid << std::endl;
            return false;
        }

        std::cout << "Terminated process " << pid << std::endl;
        return true;
    }

    bool killByName(const std::string& name) {
        std::vector<int> pids = getPidsByName(name);

        if (pids.empty()) {
            std::cout << "No processes: " << name << std::endl;
            return false;
        }

        bool success = true;
        for (int pid : pids) {
            if (!killById(pid)) {
                success = false;
            }
        }

        return success;
    }

    std::wstring stringToWstring(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    std::string wstringToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    std::vector<int> getPidsByName(const std::string& name) {
        std::vector<int> pids;
        std::wstring searchName = stringToWstring(name);
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::towlower);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return pids;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                std::wstring processName = pe32.szExeFile;
                std::transform(processName.begin(), processName.end(), processName.begin(), ::towlower);

                if (processName.find(searchName) != std::wstring::npos) {
                    pids.push_back(pe32.th32ProcessID);
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
        return pids;
    }

    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (getline(ss, token, delimiter)) {
            token.erase(std::remove(token.begin(), token.end(), '\"'), token.end());
            token.erase(std::remove(token.begin(), token.end(), '\''), token.end());
            token.erase(std::remove(token.begin(), token.end(), ' '), token.end());

            if (!token.empty()) {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    void killFromEnvironment() {
        char buffer[4096];
        DWORD size = GetEnvironmentVariableA("PROC_TO_KILL", buffer, sizeof(buffer));

        if (size == 0) {
            std::cout << "PROC_TO_KILL not set" << std::endl;
            return;
        }

        std::string procList = buffer;
        std::cout << "PROC_TO_KILL: " << procList << std::endl;

        std::vector<std::string> processes = split(procList, ',');

        for (const auto& procName : processes) {
            std::cout << "Killing: " << procName << std::endl;
            killByName(procName);
        }
    }
};

void printHelp() {
    std::cout << "Process Killer" << std::endl;
    std::cout << "--id <PID>    - Kill by ID" << std::endl;
    std::cout << "--name <NAME> - Kill by name" << std::endl;
    std::cout << "--env         - Kill from PROC_TO_KILL" << std::endl;
    std::cout << "--help        - Help" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    ProcessKiller killer;
    std::string option = argv[1];

    if (option == "--id" && argc == 3) {
        int pid = std::stoi(argv[2]);
        return killer.killById(pid) ? 0 : 1;
    }
    else if (option == "--name" && argc == 3) {
        std::string name = argv[2];
        return killer.killByName(name) ? 0 : 1;
    }
    else if (option == "--env") {
        killer.killFromEnvironment();
        return 0;
    }
    else if (option == "--help") {
        printHelp();
        return 0;
    }
    else {
        std::cerr << "Invalid arguments" << std::endl;
        printHelp();
        return 1;
    }

    return 0;
}