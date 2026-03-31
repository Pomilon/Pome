import time

def run():
    i = 0
    s = 0
    # N is injected
    while i < N:
        s += i
        i += 1
    return s

if 'N' not in globals():
    N = 1000000

start = time.time()
final_sum = run()
end = time.time()
print(final_sum)
print(f"Time: {end - start:.4f}s")
