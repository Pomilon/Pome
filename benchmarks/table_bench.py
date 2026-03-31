import time

# N is injected
t = {}
start = time.time()
for i in range(N):
    t["key" + str(i)] = i

total = 0
for j in range(N):
    total += t["key" + str(j)]
end = time.time()
print(f"Time: {end - start:.4f}s. Sum: {total}")
