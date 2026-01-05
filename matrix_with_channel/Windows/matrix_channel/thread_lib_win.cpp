#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

template<class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : buffer_size_(size), closed_(false) {}

    void Send(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return closed_ || queue_.size() < buffer_size_; });
        if (closed_) throw std::runtime_error("Channel is closed");
        queue_.push(std::move(value));
        cv_.notify_all();
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return closed_ || !queue_.empty(); });
        if (!queue_.empty()) {
            T value = std::move(queue_.front());
            queue_.pop();
            cv_.notify_all();
            return { std::move(value), true };
        }
        return { T(), false };
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
    }

private:
    std::queue<T> queue_;
    int buffer_size_;
    bool closed_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

class MatrixMultiplier {
private:
    int N;
    std::vector<std::vector<int>> A, B, C;
    std::atomic<int> active_threads{ 0 };

    struct BlockTask {
        int startRow, endRow, startCol, endCol;
    };

    BufferedChannel<BlockTask> taskChannel;
    BufferedChannel<bool> completionChannel;
    std::atomic<bool> stopWorkers{ false };

    void workerThread() {
        while (!stopWorkers) {
            auto taskResult = taskChannel.Recv();
            if (!taskResult.second) break;
            BlockTask task = taskResult.first;
            multiplyBlock(task.startRow, task.endRow, task.startCol, task.endCol);
            completionChannel.Send(true);
        }
    }

public:
    MatrixMultiplier(int size) : N(size), taskChannel(size), completionChannel(size) {
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
        int totalTasks = numBlocks * numBlocks;

        if (totalTasks > 10000) {
            std::cout << "Block size " << k << " would create " << totalTasks
                << " tasks - skipping" << std::endl;
            return;
        }

        std::cout << "Testing block size: " << k << " (creating " << totalTasks << " tasks)" << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        stopWorkers = false;
        int numWorkerThreads = std::thread::hardware_concurrency();
        if (numWorkerThreads == 0) numWorkerThreads = 4;

        std::vector<std::thread> workers;
        workers.reserve(numWorkerThreads);
        for (int i = 0; i < numWorkerThreads; i++) {
            workers.emplace_back(&MatrixMultiplier::workerThread, this);
        }

        int tasksSubmitted = 0;
        for (int i = 0; i < numBlocks; i++) {
            for (int j = 0; j < numBlocks; j++) {
                BlockTask task{
                    i * k,
                    std::min((i + 1) * k, N),
                    j * k,
                    std::min((j + 1) * k, N)
                };
                taskChannel.Send(task);
                tasksSubmitted++;
            }
        }

        for (int i = 0; i < tasksSubmitted; i++) {
            completionChannel.Recv();
        }

        stopWorkers = true;
        taskChannel.Close();

        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Block size: " << k << ", Tasks: " << tasksSubmitted
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