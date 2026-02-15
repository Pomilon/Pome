import time

print("Starting Python Loop Benchmark...")
start = time.time()
i = 0
limit = 10000000
while i < limit:
    i = i + 1
end = time.time()
print(f"Done. Time: {end - start:.4f}s")
