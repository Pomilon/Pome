import time
import sys

def bubble_sort(arr):
    n = len(arr)
    for i in range(n):
        for j in range(0, n - i - 1):
            if arr[j] > arr[j + 1]:
                arr[j], arr[j + 1] = arr[j + 1], arr[j]

if __name__ == "__main__":
    if 'N' not in globals():
        N = 1000
    data = [N - i for i in range(N)]
    start = time.time()
    bubble_sort(data)
    end = time.time()
    print(f"{data[0]} {data[N-1]}")
    print(f"Time: {end - start}s")
