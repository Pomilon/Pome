import time

def run():
    i = 0
    # N is injected
    while i < N:
        i += 1

start = time.time()
run()
end = time.time()
print(f"Time: {end - start:.4f}s")
