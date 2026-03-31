import time
import sys

def matrix_mul(A, B, size):
    C = [[0.0] * size for _ in range(size)]
    for i in range(size):
        for k in range(size):
            aik = A[i][k]
            for j in range(size):
                C[i][j] += aik * B[k][j]
    return C

if __name__ == "__main__":
    if 'N' not in globals():
        N = 100
    A = [[float(i + j) for j in range(N)] for i in range(N)]
    B = [[float(i - j) for j in range(N)] for i in range(N)]
    start = time.time()
    C = matrix_mul(A, B, N)
    end = time.time()
    print(C[0][0])
    print(f"Time: {end - start}s")
