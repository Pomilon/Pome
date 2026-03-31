import sys
import time

class Node:
    __slots__ = ['item', 'left', 'right']
    def __init__(self, item, left, right):
        self.item = item
        self.left = left
        self.right = right

    def check(self):
        if self.left is None:
            return self.item
        return self.item + self.left.check() - self.right.check()

def bottom_up_tree(item, depth):
    if depth > 0:
        return Node(item, bottom_up_tree(2 * item - 1, depth - 1), bottom_up_tree(2 * item, depth - 1))
    return Node(item, None, None)

def run(n):
    min_depth = 4
    max_depth = max(min_depth + 2, n)
    stretch_depth = max_depth + 1

    print(bottom_up_tree(0, stretch_depth).check())

    long_lived_tree = bottom_up_tree(0, max_depth)

    for depth in range(min_depth, max_depth + 1, 2):
        iterations = 2**(max_depth - depth + min_depth)
        check = 0
        for i in range(1, iterations + 1):
            check += bottom_up_tree(i, depth).check()
            check += bottom_up_tree(-i, depth).check()
        print(f"{iterations * 2} {depth} {check}")

    print(long_lived_tree.check())

if __name__ == "__main__":
    # N is injected by runner or default
    if 'N' not in globals():
        N = 12
    start = time.time()
    run(N)
    end = time.time()
    print(f"Time: {end - start}s")
