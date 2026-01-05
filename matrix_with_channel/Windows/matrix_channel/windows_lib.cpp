#include <iostream>
#include <vector>
#include <windows.h>
#include <ctime>
#include <cstdlib>
#include <queue>
#include <atomic>

using namespace std;

const int MAX_THREADS = 64;

template<class T>
class BufferedChannel {
private:
    queue<T> queue_;
    int buffer_size_;
    bool closed_;
    CRITICAL_SECTION cs_;
    CONDITION_VARIABLE cv_push_;
    CONDITION_VARIABLE cv_pop_;

public:
    BufferedChannel(int size) : buffer_size_(size), closed_(false) {
        InitializeCriticalSection(&cs_);
        InitializeConditionVariable(&cv_push_);
        InitializeConditionVariable(&cv_pop_);
    }

    ~BufferedChannel() {
        DeleteCriticalSection(&cs_);
    }

    void Send(T value) {
        EnterCriticalSection(&cs_);
        while (!closed_ && queue_.size() >= (size_t)buffer_size_) {
            SleepConditionVariableCS(&cv_push_, &cs_, INFINITE);
        }
        if (closed_) {
            LeaveCriticalSection(&cs_);
            throw runtime_error("Channel is closed");
        }
        queue_.push(move(value));
        WakeConditionVariable(&cv_pop_);
        LeaveCriticalSection(&cs_);
    }

    pair<T, bool> Recv() {
        EnterCriticalSection(&cs_);
        while (!closed_ && queue_.empty()) {
            SleepConditionVariableCS(&cv_pop_, &cs_, INFINITE);
        }
        if (!queue_.empty()) {
            T value = move(queue_.front());
            queue_.pop();
            WakeConditionVariable(&cv_push_);
            LeaveCriticalSection(&cs_);
            return { move(value), true };
        }
        LeaveCriticalSection(&cs_);
        return { T(), false };
    }

    void Close() {
        EnterCriticalSection(&cs_);
        closed_ = true;
        WakeAllConditionVariable(&cv_push_);
        WakeAllConditionVariable(&cv_pop_);
        LeaveCriticalSection(&cs_);
    }
};

class MatrixMultiplier {
private:
    int N;
    vector<vector<int>> A, B, C;

    struct BlockTask {
        int startRow, endRow, startCol, endCol;
    };

    struct ThreadData {
        MatrixMultiplier* multiplier;
        BufferedChannel<BlockTask>* taskChannel;
        BufferedChannel<bool>* completionChannel;
    };

public:
    MatrixMultiplier(int size) : N(size) {
        A.assign(N, vector<int>(N));
        B.assign(N, vector<int>(N));
        C.assign(N, vector<int>(N, 0));
        initializeMatrices();
    }

    void initializeMatrices() {
        srand(static_cast<unsigned int>(time(NULL)));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                A[i][j] = rand() % 10 + 1;
                B[i][j] = rand() % 10 + 1;
            }
        }
    }

    void multiplyBlock(int startRow, int endRow, int startCol, int endCol) {
        for (int i = startRow; i < endRow; i++) {
            for (int j = startCol; j < endCol; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) {
                    sum += A[i][k] * B[k][j];
                }
                C[i][j] = sum;
            }
        }
    }

    static DWORD WINAPI WorkerFunction(LPVOID param) {
        ThreadData* data = static_cast<ThreadData*>(param);
        while (true) {
            auto res = data->taskChannel->Recv();
            if (!res.second) break;
            BlockTask task = res.first;
            data->multiplier->multiplyBlock(task.startRow, task.endRow, task.startCol, task.endCol);
            data->completionChannel->Send(true);
        }
        delete data;
        return 0;
    }

    void sequentialMultiply() {
        cout << "Sequential multiplication..." << endl;
        DWORD startTime = GetTickCount();
        multiplyBlock(0, N, 0, N);
        DWORD endTime = GetTickCount();
        cout << "Sequential time: " << (endTime - startTime) << " ms" << endl;
    }

    void parallelMultiply(int blockSize) {
        int numBlocks = (N + blockSize - 1) / blockSize;
        int totalTasks = numBlocks * numBlocks;

        BufferedChannel<BlockTask> taskChannel(totalTasks);
        BufferedChannel<bool> completionChannel(totalTasks);

        cout << "Block size: " << blockSize << ", Tasks: " << totalTasks << " - ";
        DWORD startTime = GetTickCount();

        int numThreads = min(MAX_THREADS, totalTasks);
        vector<HANDLE> threads;

        for (int i = 0; i < numThreads; i++) {
            ThreadData* data = new ThreadData{ &this[0], &taskChannel, &completionChannel };
            HANDLE h = CreateThread(NULL, 0, WorkerFunction, data, 0, NULL);
            if (h) threads.push_back(h);
        }

        for (int i = 0; i < numBlocks; i++) {
            for (int j = 0; j < numBlocks; j++) {
                taskChannel.Send({ i * blockSize, min((i + 1) * blockSize, N),
                                  j * blockSize, min((j + 1) * blockSize, N) });
            }
        }

        for (int i = 0; i < totalTasks; i++) {
            completionChannel.Recv();
        }

        taskChannel.Close();

        for (HANDLE h : threads) {
            WaitForSingleObject(h, INFINITE);
            CloseHandle(h);
        }

        DWORD endTime = GetTickCount();
        cout << "Time: " << (endTime - startTime) << " ms" << endl;
    }
};

int main() {
    cout << "Windows Matrix Multiplication with Channel " << endl;
    int matrixSize = 20;
    cout << "Matrix size: " << matrixSize << "x" << matrixSize << endl;

    MatrixMultiplier multiplier(matrixSize);
    multiplier.sequentialMultiply();

    int blockSizes[] = { 10, 5, 2 };
    for (int bs : blockSizes) {
        multiplier.parallelMultiply(bs);
    }

    system("pause");
    return 0;
}