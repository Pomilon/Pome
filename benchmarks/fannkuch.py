import time
import sys

def fannkuch(n):
    p = list(range(n))
    q = list(range(n))
    s = list(range(n))
    sign = 1
    max_flips = 0
    sum_flips = 0
    while True:
        q0 = p[0]
        if q0 != 0:
            for i in range(1, n): q[i] = p[i]
            flips = 1
            while True:
                qq0 = q[q0]
                if qq0 == 0:
                    if flips > max_flips: max_flips = flips
                    sum_flips += sign * flips
                    break
                q[q0] = q0
                if q0 >= 3:
                    i, j = 1, q0 - 1
                    while i < j:
                        q[i], q[j] = q[j], q[i]
                        i += 1; j -= 1
                q0 = qq0
                flips += 1
        if sign == 1:
            p[0], p[1] = p[1], p[0]
            sign = -1
        else:
            p[1], p[2] = p[2], p[1]
            sign = 1
            for i in range(2, n):
                sx = s[i]
                if sx != 0:
                    s[i] = sx - 1
                    break
                if i == n - 1: return sum_flips, max_flips
                s[i] = i
                t = p[0]
                for j in range(i + 1): p[j] = p[j+1]
                p[i+1] = t

if __name__ == "__main__":
    if 'N' not in globals():
        N = 8
    start = time.time()
    res = fannkuch(N)
    end = time.time()
    print(f"{res[0]} {res[1]}")
    print(f"Time: {end - start}s")
