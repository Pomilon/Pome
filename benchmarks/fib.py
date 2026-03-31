import time

def fib(n):
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

# N is injected
start = time.time()
res = fib(N)
end = time.time()
print(res)
print(f"Time: {end - start}s")
