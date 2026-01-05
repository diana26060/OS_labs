#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

template<class T>
class BufferedChannel {
private:
    queue<T> queue_;
    int buffer_size_;
    bool closed_;
    mutex mutex_;
    condition_variable cv_push_, cv_pop_;

public:
    BufferedChannel(int size) : buffer_size_(size), closed_(false) {}

    void Send(T value) {
        unique_lock<mutex> lock(mutex_);
        cv_push_.wait(lock, [this]() { return closed_ || queue_.size() < (size_t)buffer_size_; });
        if (closed_) throw runtime_error("Channel is closed");
        queue_.push(move(value));
        cv_pop_.notify_one();
    }

    pair<T, bool> Recv() {
        unique_lock<mutex> lock(mutex_);
        cv_pop_.wait(lock, [this]() { return closed_ || !queue_.empty(); });
        if (!queue_.empty()) {
            T value = move(queue_.front());
            queue_.pop();
            cv_push_.notify_one();
            return { move(value), true };
        }
        return { T(), false };
    }

    void Close() {
        lock_guard<mutex> lock(mutex_);
        closed_ = true;
        cv_push_.notify_all();
        cv_pop_.notify_all();
    }
};

class MatrixMultiplier {
private:
    int N;
    vector<vector<int>> A, B, C;

    struct BlockTask {
        int startRow, endRow, startCol, endCol;
    };

public:
    MatrixMultiplier(int size) : N(size) {
        A.assign(N, vector<int>(N));
        B.assign(N, vector<int>(N));
        C.assign(N, vector<int>(N, 0));
        initializeMatrices();
    }

    void initializeMatrices() {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(1, 10);
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = dis(gen);
                B[i][j] = dis(gen);
            }
        }
    }

    void multiplyBlock(int startRow, int endRow, int startCol, int endCol) {
        for (int i = startRow; i < endRow; i++) {
            for (int j = startCol; j < endCol; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) sum += A[i][k] * B[k][j];
                C[i][j] = sum;
            }
        }
    }

    void sequentialMultiply() {
        auto start = chrono::high_resolution_clock::now();
        multiplyBlock(0, N, 0, N);
        auto end = chrono::high_resolution_clock::now();
        cout << "Sequential time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
    }

    void parallelMultiply(int blockSize) {
        int numBlocks = (N + blockSize - 1) / blockSize;
        int totalTasks = numBlocks * numBlocks;
        BufferedChannel<BlockTask> taskChannel(totalTasks);
        BufferedChannel<bool> completionChannel(totalTasks);

        auto start = chrono::high_resolution_clock::now();
        int numThreads = min(64, totalTasks);
        vector<thread> threads;

        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([this, &taskChannel, &completionChannel]() {
                while (true) {
                    auto res = taskChannel.Recv();
                    if (!res.second) break;
                    multiplyBlock(res.first.startRow, res.first.endRow, res.first.startCol, res.first.endCol);
                    completionChannel.Send(true);
                }
            });
        }

        for (int i = 0; i < numBlocks; i++) {
            for (int j = 0; j < numBlocks; j++) {
                taskChannel.Send({i * blockSize, min((i + 1) * blockSize, N), j * blockSize, min((j + 1) * blockSize, N)});
            }
        }

        for (int i = 0; i < totalTasks; i++) completionChannel.Recv();
        taskChannel.Close();
        for (auto& t : threads) t.join();

        auto end = chrono::high_resolution_clock::now();
        cout << "Block size: " << blockSize << ", Time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
    }
};

int main() {
    int size = 500;
    MatrixMultiplier m(size);
    m.sequentialMultiply();
    vector<int> blocks = {100, 50, 25};
    for (int b : blocks) m.parallelMultiply(b);
    return 0;
}
