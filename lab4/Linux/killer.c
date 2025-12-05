#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <cstring>

class ProcessKiller {
public:
    bool killById(int pid) {
        if (kill(pid, SIGKILL) == 0) {
            std::cout << "Terminated process with PID " << pid << std::endl;
            return true;
        } else {
            std::cerr << "Failed to terminate PID " << pid << std::endl;
            return false;
        }
    }
    
    bool killByName(const std::string& name) {
        std::vector<int> pids = getPidsByName(name);
        
        if (pids.empty()) {
            std::cout << "No processes found: " << name << std::endl;
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
    
    std::vector<int> getPidsByName(const std::string& name) {
        std::vector<int> pids;
        std::string command = "pgrep -f " + name + " 2>/dev/null";
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                int pid = atoi(buffer);
                if (pid > 0) {
                    pids.push_back(pid);
                }
            }
            pclose(pipe);
        }
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
        const char* envVar = std::getenv("PROC_TO_KILL");
        
        if (envVar == nullptr) {
            std::cout << "PROC_TO_KILL not set" << std::endl;
            return;
        }
        
        std::string procList = envVar;
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
