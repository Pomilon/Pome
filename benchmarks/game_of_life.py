import time
import sys

def create_grid(size):
    return [[(x + y) % 2 == 0 for x in range(size)] for y in range(size)]

def count_neighbors(grid, x, y, size):
    count = 0
    for dy in range(-1, 2):
        for dx in range(-1, 2):
            if dx == 0 and dy == 0: continue
            nx, ny = (x + dx) % size, (y + dy) % size
            if grid[ny][nx]: count += 1
    return count

def step(grid, size):
    new_grid = [[False for _ in range(size)] for _ in range(size)]
    for y in range(size):
        for x in range(size):
            neighbors = count_neighbors(grid, x, y, size)
            if grid[y][x]:
                new_grid[y][x] = neighbors in (2, 3)
            else:
                new_grid[y][x] = neighbors == 3
    return new_grid

if __name__ == "__main__":
    if 'N' not in globals():
        N = 50
    iterations = 50
    grid = create_grid(N)
    start = time.time()
    for _ in range(iterations):
        grid = step(grid, N)
    end = time.time()
    alive_count = sum(sum(row) for row in grid)
    print(alive_count)
    print(f"Time: {end - start}s")
