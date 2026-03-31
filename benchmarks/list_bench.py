import time

# N is injected
start = time.time()
l = []
for i in range(N):
    l.append(i)

total = 0
for x in l:
    total += x

end = time.time()
print(f"Time: {end - start:.4f}s. Sum: {total}")
