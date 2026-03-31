import time
import sys

def h_tree(x, y, size, depth):
    if depth == 0:
        return 1
    count = 1
    x0, x1 = x - size / 2.0, x + size / 2.0
    y0, y1 = y - size / 2.0, y + size / 2.0
    
    count += h_tree(x0, y0, size / 2.0, depth - 1)
    count += h_tree(x0, y1, size / 2.0, depth - 1)
    count += h_tree(x1, y0, size / 2.0, depth - 1)
    count += h_tree(x1, y1, size / 2.0, depth - 1)
    return count

if __name__ == "__main__":
    if 'N' not in globals():
        N = 10
    start = time.time()
    nodes = h_tree(512.0, 512.0, 512.0, N)
    end = time.time()
    print(nodes)
    print(f"Time: {end - start}s")
