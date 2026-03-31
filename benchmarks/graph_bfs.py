import time
import sys
from collections import deque

def graph_bfs(nodes, edges, start_node):
    visited = {start_node: True}
    queue = deque([start_node])
    count = 0
    while queue:
        u = queue.popleft()
        count += 1
        neighbors = edges.get(u)
        if neighbors:
            for v in neighbors:
                if v not in visited:
                    visited[v] = True
                    queue.append(v)
    return count

if __name__ == "__main__":
    if 'N' not in globals():
        N = 10000
    nodes = list(range(N))
    edges = {i: [(i + j) % N for j in range(1, 4)] for i in range(N)}
    start = time.time()
    c = graph_bfs(nodes, edges, 0)
    end = time.time()
    print(c)
    print(f"Time: {end - start}s")
