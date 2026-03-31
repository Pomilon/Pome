import time
import math
import sys

def A(i, j):
    return 1.0 / ((i + j) * (i + j + 1) / 2 + i + 1)

def multiplyAv(n, v, Av):
    for i in range(n):
        Av[i] = 0.0
        for j in range(n):
            Av[i] += A(i, j) * v[j]

def multiplyAtv(n, v, Atv):
    for i in range(n):
        Atv[i] = 0.0
        for j in range(n):
            Atv[i] += A(j, i) * v[j]

def multiplyAtAv(n, v, AtAv):
    u = [0.0] * n
    multiplyAv(n, v, u)
    multiplyAtv(n, u, AtAv)

if __name__ == "__main__":
    if 'N' not in globals():
        N = 100
    u = [1.0] * N
    v = [0.0] * N
    start = time.time()
    for _ in range(10):
        multiplyAtAv(N, u, v)
        multiplyAtAv(N, v, u)
    
    vBv = 0.0
    vv = 0.0
    for i in range(N):
        vBv += u[i] * v[i]
        vv += v[i] * v[i]
        
    res = math.sqrt(vBv / vv)
    end = time.time()
    print(res)
    print(f"Time: {end - start}s")
