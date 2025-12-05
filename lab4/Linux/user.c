#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

class ProcessManager {
public:
    bool isProcessRunning(const std::string& name) {
        std::string command = "pgrep -f " + name + " > /dev/null 2>&1";
        return system(command.c_str()) == 0;
    }
    
    bool isProcessRunning(int pid) {
        return (kill(pid, 0) == 0);
    }
    
    bool startProcess(const std::string& command) {
        pid_t pid = fork();
        
        if (pid == 0) {
            execl("/bin/sh", "sh", "-c", command.c_str(), NULL);
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            std::cerr << "Fork failed" << std::endl;
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return true;
    }
    
    int getProcessId(const std::string& name) {
        std::string command = "pgrep -f " + name + " | head -1";
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                int pid = atoi(buffer);
                pclose(pipe);
                return pid;
            }
            pclose(pipe);
        }
        return -1;
    }
};

void demoKiller() {
    ProcessManager pm;
    
    std::cout << "\n Setting PROC_TO_KILL environment variable " << std::endl;
    setenv("PROC_TO_KILL", "sleep,gedit", 1);
    
    std::cout << "\n Starting test processes " << std::endl;
    system("sleep 100 &");
    system("sleep 200 &");
    system("gedit --version 2>&1 | head -1");
    
    std::cout << "\nChecking processes before kill " << std::endl;
    if (pm.isProcessRunning("sleep")) {
        std::cout << "Process 'sleep' is running" << std::endl;
    }
    
    std::cout << "\n Testing --name parameter " << std::endl;
    system("./killer --name sleep");
    
    std::cout << "\nTesting --env parameter " << std::endl;
    system("./killer --env");
    
    std::cout << "\n Checking processes after kill " << std::endl;
    if (!pm.isProcessRunning("sleep")) {
        std::cout << "Process 'sleep' terminated" << std::endl;
    }
    
    std::cout << "\nTesting --id parameter " << std::endl;
    int testPid = fork();
    if (testPid == 0) {
        sleep(300);
        exit(0);
    } else if (testPid > 0) {
        std::cout << "Test process PID: " << testPid << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (pm.isProcessRunning(testPid)) {
            std::cout << "Process " << testPid << " is running" << std::endl;
        }
        
        std::string cmd = "./killer --id " + std::to_string(testPid);
        system(cmd.c_str());
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!pm.isProcessRunning(testPid)) {
            std::cout << "Process " << testPid << " terminated" << std::endl;
        }
    }
    
    std::cout << "\nCleaning up " << std::endl;
    unsetenv("PROC_TO_KILL");
    
    std::cout << "\n Demonstration completed " << std::endl;
}

int main() {
    std::cout << " Process Killer Demonstration " << std::endl;
    
    demoKiller();
    
    return 0;
}
