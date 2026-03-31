import time
import sys

def quick_sort(arr):
    if len(arr) <= 1:
        return arr
    pivot = arr[0]
    left = [x for x in arr[1:] if x < pivot]
    right = [x for x in arr[1:] if x >= pivot]
    return quick_sort(left) + [pivot] + quick_sort(right)

if __name__ == "__main__":
    if 'N' not in globals():
        N = 1000
    data = [(i * 1103515245 + 12345) % N for i in range(N)]
    start = time.time()
    sorted_data = quick_sort(data)
    end = time.time()
    print(len(sorted_data))
    print(f"Time: {end - start}s")
