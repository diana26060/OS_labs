#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

const int N = 1;

void process_M(int read_fd, int write_fd) {
    char buffer;
    std::string number;
    
    while (read(read_fd, &buffer, 1) > 0) {
        if (buffer == ' ' || buffer == '\n') {
            if (!number.empty()) {
                int x = std::stoi(number);
                int result = x * 7;
                std::string output = std::to_string(result) + " ";
                write(write_fd, output.c_str(), output.length());
                number.clear();
            }
            if (buffer == '\n') {
                write(write_fd, "\n", 1);
                break;
            }
        } else {
            number += buffer;
        }
    }
    
    close(read_fd);
    close(write_fd);
    exit(0);
}

void process_A(int read_fd, int write_fd) {
    char buffer;
    std::string number;
    
    while (read(read_fd, &buffer, 1) > 0) {
        if (buffer == ' ' || buffer == '\n') {
            if (!number.empty()) {
                int x = std::stoi(number);
                int result = x + N;
                std::string output = std::to_string(result) + " ";
                write(write_fd, output.c_str(), output.length());
                number.clear();
            }
            if (buffer == '\n') {
                write(write_fd, "\n", 1);
                break;
            }
        } else {
            number += buffer;
        }
    }
    
    close(read_fd);
    close(write_fd);
    exit(0);
}

void process_P(int read_fd, int write_fd) {
    char buffer;
    std::string number;
    
    while (read(read_fd, &buffer, 1) > 0) {
        if (buffer == ' ' || buffer == '\n') {
            if (!number.empty()) {
                int x = std::stoi(number);
                int result = x * x * x;
                std::string output = std::to_string(result) + " ";
                write(write_fd, output.c_str(), output.length());
                number.clear();
            }
            if (buffer == '\n') {
                write(write_fd, "\n", 1);
                break;
            }
        } else {
            number += buffer;
        }
    }
    
    close(read_fd);
    close(write_fd);
    exit(0);
}

void process_S(int read_fd, int write_fd) {
    char buffer;
    std::string number;
    int sum = 0;
    
    while (read(read_fd, &buffer, 1) > 0) {
        if (buffer == ' ' || buffer == '\n') {
            if (!number.empty()) {
                int x = std::stoi(number);
                sum += x;
                number.clear();
            }
            if (buffer == '\n') {
                break;
            }
        } else {
            number += buffer;
        }
    }
    
    std::string output = "Сумма: " + std::to_string(sum) + "\n";
    write(write_fd, output.c_str(), output.length());
    
    close(read_fd);
    close(write_fd);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string mode = argv[1];
        if (mode == "M") {
            process_M(STDIN_FILENO, STDOUT_FILENO);
        } else if (mode == "A") {
            process_A(STDIN_FILENO, STDOUT_FILENO);
        } else if (mode == "P") {
            process_P(STDIN_FILENO, STDOUT_FILENO);
        } else if (mode == "S") {
            process_S(STDIN_FILENO, STDOUT_FILENO);
        }
        return 0;
    }
    
    int pipe1[2], pipe2[2], pipe3[2], pipe4[2];
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || 
        pipe(pipe3) == -1 || pipe(pipe4) == -1) {
        perror("pipe");
        return 1;
    }
    
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        process_M(pipe1[0], pipe2[1]);
    }
    
    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipe2[1]);
        close(pipe3[0]);
        process_A(pipe2[0], pipe3[1]);
    }
    
    pid_t pid3 = fork();
    if (pid3 == 0) {
        close(pipe3[1]);
        close(pipe4[0]);
        process_P(pipe3[0], pipe4[1]);
    }
    
    pid_t pid4 = fork();
    if (pid4 == 0) {
        close(pipe4[1]);
        process_S(pipe4[0], STDOUT_FILENO);
    }
    
    close(pipe1[0]);
    close(pipe2[0]);
    close(pipe2[1]);
    close(pipe3[0]);
    close(pipe3[1]);
    close(pipe4[0]);
    close(pipe4[1]);
    
    std::string input;
    std::cout << "Введите числа через пробел: ";
    std::getline(std::cin, input);
    
    for (char c : input) {
        write(pipe1[1], &c, 1);
    }
    write(pipe1[1], "\n", 1);
    close(pipe1[1]);
    
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    waitpid(pid4, NULL, 0);
    
    return 0;
}
