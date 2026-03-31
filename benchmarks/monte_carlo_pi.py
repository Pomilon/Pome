import time
import sys

class Random:
    def __init__(self, seed):
        self.seed = seed
    def next(self):
        self.seed = (self.seed * 1103515245 + 12345) % 2147483648
        return self.seed / 2147483648.0

def estimate_pi(n):
    rng = Random(42)
    inside = 0
    for _ in range(n):
        x = rng.next()
        y = rng.next()
        if x*x + y*y <= 1.0:
            inside += 1
    return 4.0 * inside / n

if __name__ == "__main__":
    if 'N' not in globals():
        N = 100000
    start = time.time()
    pi = estimate_pi(N)
    end = time.time()
    print(pi)
    print(f"Time: {end - start}s")
