import time
import sys

def dijkstra(nodes, edges, start_node):
    distances = {node: 1000000000 for node in nodes}
    visited = {}
    distances[start_node] = 0
    
    for _ in range(len(nodes)):
        min_dist = 1000000000
        u = -1
        for n in nodes:
            if n not in visited and distances[n] < min_dist:
                min_dist = distances[n]
                u = n
        
        if u == -1: break
        visited[u] = True
        
        neighbors = edges.get(u)
        if neighbors:
            for v, weight in neighbors:
                if distances[u] + weight < distances[v]:
                    distances[v] = distances[u] + weight
    return distances

if __name__ == "__main__":
    if 'N' not in globals():
        N = 200
    nodes = list(range(N))
    edges = {}
    for i in range(N):
        neighbors = []
        for j in range(1, 6):
            v = (i + j) % N
            neighbors.append((v, j * 1.5))
        edges[i] = neighbors
        
    start = time.time()
    d = dijkstra(nodes, edges, 0)
    end = time.time()
    print(d[N-1])
    print(f"Time: {end - start}s")
