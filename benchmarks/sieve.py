import time
import sys

def sieve(limit):
    primes = []
    is_prime = [True] * (limit + 1)
    is_prime[0] = is_prime[1] = False
    
    for p in range(2, int(limit**0.5) + 1):
        if is_prime[p]:
            for i in range(p * p, limit + 1, p):
                is_prime[i] = False
                
    for i in range(2, limit + 1):
        if is_prime[i]:
            primes.append(i)
    return primes

if __name__ == "__main__":
    if 'N' not in globals():
        N = 100000
    start = time.time()
    p = sieve(N)
    end = time.time()
    print(len(p))
    print(f"Time: {end - start}s")
