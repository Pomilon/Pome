import time

class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y
    def dist(self):
        return self.x * self.x + self.y * self.y

# N is injected
i = 0
total = 0
start = time.time()
while i < N:
    p = Point(i, i + 1)
    total += p.dist()
    i += 1
end = time.time()
print(total)
print(f"Time: {end - start:.4f}s")
