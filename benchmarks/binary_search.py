import time
import sys

def binary_search(arr, target):
    low = 0
    high = len(arr) - 1
    while low <= high:
        mid = (low + high) // 2
        if arr[mid] == target:
            return mid
        elif arr[mid] < target:
            low = mid + 1
        else:
            high = mid - 1
    return -1

if __name__ == "__main__":
    if 'N' not in globals():
        N = 10000
    data_size = 10000
    data = list(range(data_size))
    start = time.time()
    found = 0
    for i in range(N):
        if binary_search(data, i % data_size) != -1:
            found += 1
    end = time.time()
    print(found)
    print(f"Time: {end - start}s")
