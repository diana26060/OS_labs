#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>

class MatrixMultiplier {
private:
    int N;
    std::vector<std::vector<int>> A, B, C;
    std::atomic<int> active_threads{ 0 };

public:
    MatrixMultiplier(int size) : N(size) {
        A.resize(N, std::vector<int>(N));
        B.resize(N, std::vector<int>(N));
        C.resize(N, std::vector<int>(N, 0));
        initializeMatrices();
    }

    void initializeMatrices() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 10);

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = dis(gen);
                B[i][j] = dis(gen);
            }
        }
    }

    void multiplyBlock(int startRow, int endRow, int startCol, int endCol) {
        active_threads++;
        for (int i = startRow; i < endRow; i++) {
            for (int j = startCol; j < endCol; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }
        active_threads--;
    }

    void sequentialMultiply() {
        auto start = std::chrono::high_resolution_clock::now();

        multiplyBlock(0, N, 0, N);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Sequential time: " << duration.count() << " microseconds" << std::endl;
    }

    void parallelMultiply(int k) {
        if (k <= 0 || k > N) {
            std::cout << "Invalid block size!" << std::endl;
            return;
        }

        int numBlocks = (N + k - 1) / k;
        int totalThreads = numBlocks * numBlocks;

        if (totalThreads > 10000) {
            std::cout << "Block size " << k << " would create " << totalThreads
                << " threads - skipping (too many threads)" << std::endl;
            return;
        }

        std::cout << "Testing block size: " << k << " (creating " << totalThreads << " threads)" << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        threads.reserve(totalThreads);

        for (int i = 0; i < numBlocks; i++) {
            for (int j = 0; j < numBlocks; j++) {
                int startRow = i * k;
                int endRow = std::min((i + 1) * k, N);
                int startCol = j * k;
                int endCol = std::min((j + 1) * k, N);

                threads.emplace_back(&MatrixMultiplier::multiplyBlock, this,
                    startRow, endRow, startCol, endCol);
            }
        }

        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Block size: " << k << ", Threads: " << threads.size()
            << ", Time: " << duration.count() << " microseconds" << std::endl;
    }
};

int main() {
    int N = 500;
    std::cout << "Matrix size: " << N << "x" << N << std::endl;

    MatrixMultiplier multiplier(N);

    multiplier.sequentialMultiply();

    std::cout << "\nParallel multiplication with different block sizes:" << std::endl; 
    std::vector<int> blockSizes = { 50, 100, 125, 250, 500 };

    for (int k : blockSizes) {
        if (k <= N) {
            MatrixMultiplier testMultiplier(N);
            testMultiplier.parallelMultiply(k);
        }
    }

    return 0;
}
