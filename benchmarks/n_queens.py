import time
import sys

def solve_queens(n, row, cols, diag1, diag2):
    if row == n:
        return 1
    count = 0
    for col in range(n):
        d1 = row + col
        d2 = row - col + n
        if col not in cols and d1 not in diag1 and d2 not in diag2:
            cols.add(col)
            diag1.add(d1)
            diag2.add(d2)
            count += solve_queens(n, row + 1, cols, diag1, diag2)
            cols.remove(col)
            diag1.remove(d1)
            diag2.remove(d2)
    return count

if __name__ == "__main__":
    if 'N' not in globals():
        N = 10
    start = time.time()
    solutions = solve_queens(N, 0, set(), set(), set())
    end = time.time()
    print(solutions)
    print(f"Time: {end - start}s")
