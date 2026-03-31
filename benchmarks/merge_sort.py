import time
import sys

def merge(left, right):
    result = []
    i = 0
    j = 0
    while i < len(left) and j < len(right):
        if left[i] <= right[j]:
            result.append(left[i])
            i += 1
        else:
            result.append(right[j])
            j += 1
    result.extend(left[i:])
    result.extend(right[j:])
    return result

def merge_sort(arr):
    if len(arr) <= 1:
        return arr
    mid = len(arr) // 2
    left = merge_sort(arr[:mid])
    right = merge_sort(arr[mid:])
    return merge(left, right)

if __name__ == "__main__":
    if 'N' not in globals():
        N = 5000
    data = [N - i for i in range(N)]
    start = time.time()
    sorted_data = merge_sort(data)
    end = time.time()
    print(f"{len(sorted_data)} {sorted_data[0]} {sorted_data[-1]}")
    print(f"Time: {end - start}s")
