#include <iostream>
#include <vector>
#include <windows.h>
#include <ctime>
#include <cstdlib>

using namespace std;

const int MAX_THREADS = 64; 

class MatrixMultiplier {
private:
    int N;
    vector<vector<int>> A, B, C;

    struct ThreadData {
        MatrixMultiplier* multiplier;
        int startRow, endRow, startCol, endCol;
    };


public:
    MatrixMultiplier(int size) : N(size) {
        A.resize(N);
        B.resize(N);
        C.resize(N);
        for (int i = 0; i < N; i++) {
            A[i].resize(N, 0);
            B[i].resize(N, 0);
            C[i].resize(N, 0);
        }
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
        if (startRow < 0 || endRow > N || startCol < 0 || endCol > N) {
            std::cout << "invalid block boundaries" << endl;
            return;
        }

        for (int i = startRow; i < endRow; i++) {
            for (int j = startCol; j < endCol; j++) {
                int sum = 0;
                for (int k = 0; k < N; k++) {
                    if (i < A.size() && k < A[i].size() && k < B.size() && j < B[k].size()) {
                        sum += A[i][k] * B[k][j];
                    }
                }
                if (i < C.size() && j < C[i].size()) {
                    C[i][j] = sum;
                }
            }
        }
    }

    static DWORD WINAPI ThreadFunction(LPVOID param) {
        ThreadData* data = static_cast<ThreadData*>(param);
        if (data && data->multiplier) {
            data->multiplier->multiplyBlock(data->startRow, data->endRow, data->startCol, data->endCol);
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
        if (blockSize <= 0 || blockSize > N) {
            cout << "Invalid block size: " << blockSize << endl;
            return;
        }

        int numBlocks = (N + blockSize - 1) / blockSize;
        int totalThreads = numBlocks * numBlocks;

        if (totalThreads > 100) {
            cout << "Too many threads (" << totalThreads << ") for block size " << blockSize << endl;
            return;
        }

        cout << "Block size: " << blockSize << ", Threads: " << totalThreads << " - ";

        DWORD startTime = GetTickCount();

        vector<HANDLE> threads;
        threads.reserve(totalThreads);

        for (int i = 0; i < numBlocks; i++) {
            for (int j = 0; j < numBlocks; j++) {
                ThreadData* data = new ThreadData();
                data->multiplier = this;
                data->startRow = i * blockSize;
                data->endRow = min((i + 1) * blockSize, N);
                data->startCol = j * blockSize;
                data->endCol = min((j + 1) * blockSize, N);

                if (data->startRow >= N || data->startCol >= N) {
                    delete data;
                    continue;
                }

                HANDLE thread = CreateThread(
                    NULL,
                    0,
                    ThreadFunction,
                    data,
                    0,
                    NULL
                );

                if (thread == NULL) {
                    cout << "Failed to create thread!" << endl;
                    delete data;
                }
                else {
                    threads.push_back(thread);
                }
            }
        }

        for (size_t i = 0; i < threads.size(); i += MAX_THREADS) {
            DWORD count = min(threads.size() - i, MAX_THREADS);
            WaitForMultipleObjects(count, &threads[i], TRUE, INFINITE);
        }
        for (HANDLE thread : threads) {
            CloseHandle(thread);
        }

        DWORD endTime = GetTickCount();
        cout << "Time: " << (endTime - startTime) << " ms" << endl;
    }
};

int main() {
    cout << " Windows Matrix Multiplication " << endl;

    int matrixSize = 10;
    cout << "Matrix size: " << matrixSize << "x" << matrixSize << endl << endl;

    MatrixMultiplier multiplier(matrixSize);

    multiplier.sequentialMultiply();

    cout << endl << "Parallel multiplication:" << endl;

   
    int blockSizes[] = { 5, 2, 1 };

    for (int blockSize : blockSizes) {
        if (blockSize <= matrixSize) {
            MatrixMultiplier testMultiplier(matrixSize);
            testMultiplier.parallelMultiply(blockSize);
        }
    }

    cout << endl << "program completed successfully" << endl;
    system("pause");
    return 0;
}