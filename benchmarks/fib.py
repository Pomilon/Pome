import time

def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

# N is injected
print(f"Calculating Python Fib({N})...")
start = time.time()
res = fib(N)
end = time.time()
print(f"Fib({N}) = {res}")
print(f"Time: {end - start:.4f}s")
