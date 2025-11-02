#include <iostream>
#include <vector>
#include <pthread.h>
#include <chrono>
#include <random>
#include <atomic>
#include <algorithm>
#include <iomanip>

class PthreadMatrixMultiplier {
private:
    int N;
    std::vector<std::vector<int>> A, B, C;
    std::atomic<int> active_threads{0};

    struct ThreadData {
        PthreadMatrixMultiplier* multiplier;
        int startRow, endRow, startCol, endCol;
    };

public:
    PthreadMatrixMultiplier(int size) : N(size) {
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

    static void* multiplyBlockWrapper(void* arg) {
        ThreadData* data = static_cast<ThreadData*>(arg);
        data->multiplier->multiplyBlock(data->startRow, data->endRow, data->startCol, data->endCol);
        delete data;
        return nullptr;
    }

    void sequentialMultiply() {
        auto start = std::chrono::high_resolution_clock::now();
        
        multiplyBlock(0, N, 0, N);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "Sequential time: " << duration.count() << " microseconds" << std::endl;
    }

    bool parallelMultiply(int k) {
        if (k <= 0 || k > N) {
            std::cout << "Invalid block size!" << std::endl;
            return false;
        }

        int numBlocks = (N + k - 1) / k;
        int totalThreads = numBlocks * numBlocks;
        
        // Защита от слишком большого количества потоков
        if (totalThreads > 1000) {
            std::cout << "Block size " << k << " would create " << totalThreads 
                      << " threads - skipping (too many threads)" << std::endl;
            return false;
        }

        std::cout << "Testing block size: " << std::setw(3) << k 
                  << " (creating " << std::setw(3) << totalThreads << " threads)... " << std::flush;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<pthread_t> threads;
        threads.reserve(totalThreads);

        for (int i = 0; i < numBlocks; i++) {
            for (int j = 0; j < numBlocks; j++) {
                ThreadData* data = new ThreadData{
                    this,
                    i * k,
                    std::min((i + 1) * k, N),
                    j * k,
                    std::min((j + 1) * k, N)
                };
                
                pthread_t thread;
                int result = pthread_create(&thread, nullptr, multiplyBlockWrapper, data);
                if (result == 0) {
                    threads.push_back(thread);
                } else {
                    delete data;
                    std::cout << "Failed to create thread! Error: " << result << std::endl;
                    return false;
                }
            }
        }

        for (pthread_t thread : threads) {
            pthread_join(thread, nullptr);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Time: " << std::setw(8) << duration.count() << " microseconds" << std::endl;
        return true;
    }

    // Метод для проверки корректности умножения
    bool verifyMultiplication() {
        std::vector<std::vector<int>> reference(N, std::vector<int>(N, 0));
        
        // Вычисляем эталонный результат последовательно
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                }
                reference[i][j] = sum;
            }
        }

        // Сравниваем с параллельным результатом
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (C[i][j] != reference[i][j]) {
                    std::cout << "Verification failed at [" << i << "][" << j << "]: " 
                              << C[i][j] << " != " << reference[i][j] << std::endl;
                    return false;
                }
            }
        }
        return true;
    }

    // Метод для тестирования всех размеров блоков
    void testAllBlockSizes() {
        std::cout << "\nTesting parallel multiplication with different block sizes:" << std::endl;
        std::cout << "Matrix size: " << N << "x" << N << std::endl;
        std::cout << "==============================================" << std::endl;
        
        // Тестируем разные размеры блоков
        std::vector<int> blockSizes;
        
        // Добавляем размеры блоков от больших к маленьким
        for (int k = N; k >= 50; k -= 50) {
            if (k > 0) blockSizes.push_back(k);
        }
        
        // Добавляем некоторые конкретные размеры
        blockSizes.insert(blockSizes.end(), {25, 20, 10, 5, 2, 1});
        
        // Сортируем по убыванию для лучшего визуального представления
        std::sort(blockSizes.rbegin(), blockSizes.rend());
        
        // Удаляем дубликаты
        blockSizes.erase(std::unique(blockSizes.begin(), blockSizes.end()), blockSizes.end());
        
        for (int k : blockSizes) {
            if (k <= N) {
                PthreadMatrixMultiplier testMultiplier(N);
                if (!testMultiplier.parallelMultiply(k)) {
                    continue;
                }
                
                // Проверяем корректность для небольшой матрицы
                if (N <= 100) {
                    if (!testMultiplier.verifyMultiplication()) {
                        std::cout << "ERROR: Multiplication verification failed for block size " << k << std::endl;
                    }
                }
            }
        }
    }
};

int main() {
    std::cout << "=== Matrix Multiplication Benchmark (Linux pthread) ===" << std::endl;
    
    // Тестируем с разными размерами матриц
    std::vector<int> matrixSizes = {100, 200, 500};
    
    for (int N : matrixSizes) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "MATRIX SIZE: " << N << "x" << N << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        PthreadMatrixMultiplier multiplier(N);
        
        // Последовательное умножение для сравнения
        std::cout << "Sequential multiplication:" << std::endl;
        multiplier.sequentialMultiply();
        
        // Параллельное умножение с разными размерами блоков
        multiplier.testAllBlockSizes();
        
        // Проверяем корректность для последнего запуска
        if (N <= 100) {
            std::cout << "\nVerifying multiplication correctness..." << std::endl;
            if (multiplier.verifyMultiplication()) {
                std::cout << "Multiplication verified successfully!" << std::endl;
            } else {
                std::cout << "Multiplication verification failed!" << std::endl;
            }
        }
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Benchmark completed!" << std::endl;
    return 0;
}
