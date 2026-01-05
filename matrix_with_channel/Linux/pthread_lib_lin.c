#include <iostream>
#include <vector>
#include <pthread.h>
#include <chrono>
#include <random>
#include <queue>

using namespace std;

template<class T>
class BufferedChannel {
private:
    queue<T> queue_;
    int buffer_size_;
    bool closed_;
    pthread_mutex_t mutex_;
    pthread_cond_t cv_push_, cv_pop_;

public:
    BufferedChannel(int size) : buffer_size_(size), closed_(false) {
        pthread_mutex_init(&mutex_, NULL);
        pthread_cond_init(&cv_push_, NULL);
        pthread_cond_init(&cv_pop_, NULL);
    }

    ~BufferedChannel() {
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&cv_push_);
        pthread_cond_destroy(&cv_pop_);
    }

    void Send(T value) {
        pthread_mutex_lock(&mutex_);
        while (!closed_ && queue_.size() >= (size_t)buffer_size_) {
            pthread_cond_wait(&cv_push_, &mutex_);
        }
        if (closed_) {
            pthread_mutex_unlock(&mutex_);
            return;
        }
        queue_.push(move(value));
        pthread_cond_signal(&cv_pop_);
        pthread_mutex_unlock(&mutex_);
    }

    pair<T, bool> Recv() {
        pthread_mutex_lock(&mutex_);
        while (!closed_ && queue_.empty()) {
            pthread_cond_wait(&cv_pop_, &mutex_);
        }
        if (!queue_.empty()) {
            T value = move(queue_.front());
            queue_.pop();
            pthread_cond_signal(&cv_push_);
            pthread_mutex_unlock(&mutex_);
            return { move(value), true };
        }
        pthread_mutex_unlock(&mutex_);
        return { T(), false };
    }

    void Close() {
        pthread_mutex_lock(&mutex_);
        closed_ = true;
        pthread_cond_broadcast(&cv_push_);
        pthread_cond_broadcast(&cv_pop_);
        pthread_mutex_unlock(&mutex_);
    }
};

class PthreadMatrixMultiplier {
private:
    int N;
    vector<vector<int>> A, B, C;
    struct BlockTask { int startRow, endRow, startCol, endCol; };
    struct ThreadData {
        PthreadMatrixMultiplier* multiplier;
        BufferedChannel<BlockTask>* taskChannel;
        BufferedChannel<bool>* completionChannel;
    };

public:
    PthreadMatrixMultiplier(int size) : N(size) {
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

    static void* Worker(void* arg) {
        ThreadData* data = (ThreadData*)arg;
        while (true) {
            auto res = data->taskChannel->Recv();
            if (!res.second) break;
            data->multiplier->multiplyBlock(res.first.startRow, res.first.endRow, res.first.startCol, res.first.endCol);
            data->completionChannel->Send(true);
        }
        return NULL;
    }

    void parallelMultiply(int blockSize) {
        int numBlocks = (N + blockSize - 1) / blockSize;
        int totalTasks = numBlocks * numBlocks;
        BufferedChannel<BlockTask> taskChannel(totalTasks);
        BufferedChannel<bool> completionChannel(totalTasks);

        auto start = chrono::high_resolution_clock::now();
        int numThreads = min(64, totalTasks);
        vector<pthread_t> threads(numThreads);
        ThreadData data = {this, &taskChannel, &completionChannel};

        for (int i = 0; i < numThreads; i++) pthread_create(&threads[i], NULL, Worker, &data);

        for (int i = 0; i < numBlocks; i++) {
            for (int j = 0; j < numBlocks; j++) {
                taskChannel.Send({i * blockSize, min((i + 1) * blockSize, N), j * blockSize, min((j + 1) * blockSize, N)});
            }
        }

        for (int i = 0; i < totalTasks; i++) completionChannel.Recv();
        taskChannel.Close();
        for (int i = 0; i < numThreads; i++) pthread_join(threads[i], NULL);

        auto end = chrono::high_resolution_clock::now();
        cout << "Block size: " << blockSize << ", Time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
    }
};

int main() {
    PthreadMatrixMultiplier m(500);
    m.parallelMultiply(100);
    m.parallelMultiply(50);
    return 0;
}
