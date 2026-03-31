import time
import sys

def compute_mean(data):
    return sum(data) / len(data)

def compute_variance(data, mean):
    return sum((x - mean)**2 for x in data) / len(data)

def bubble_sort(data):
    n = len(data)
    for i in range(n):
        for j in range(0, n - i - 1):
            if data[j] > data[j+1]:
                data[j], data[j+1] = data[j+1], data[j]

if __name__ == "__main__":
    if 'N' not in globals():
        N = 2000
    data = [(i * 1103515245 + 12345) % 1000 for i in range(N)]
    start = time.time()
    mean = compute_mean(data)
    variance = compute_variance(data, mean)
    # Using bubble sort for consistency
    bubble_sort(data)
    median = data[len(data) // 2]
    end = time.time()
    print(f"{mean} {variance} {median}")
    print(f"Time: {end - start}s")
