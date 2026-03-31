import time
import sys

class Node:
    __slots__ = ['val', 'next']
    def __init__(self, val, next_node):
        self.val = val
        self.next = next_node

def create_list(n):
    head = None
    for i in range(n):
        head = Node(i, head)
    return head

def sum_list(head):
    total = 0
    curr = head
    while curr:
        total += curr.val
        curr = curr.next
    return total

def reverse_list(head):
    prev = None
    curr = head
    while curr:
        nxt = curr.next
        curr.next = prev
        prev = curr
        curr = nxt
    return prev

if __name__ == "__main__":
    if 'N' not in globals():
        N = 10000
    start = time.time()
    l = create_list(N)
    s1 = sum_list(l)
    l = reverse_list(l)
    s2 = sum_list(l)
    end = time.time()
    print(f"{s1} {s2}")
    print(f"Time: {end - start}s")
