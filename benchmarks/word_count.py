import time
import sys

def word_count(text, iterations):
    counts = {}
    for _ in range(iterations):
        word = ""
        for c in text:
            if c in (" ", "\n", "\t"):
                if word:
                    counts[word] = counts.get(word, 0) + 1
                    word = ""
            else:
                word += c
        if word:
            counts[word] = counts.get(word, 0) + 1
    return counts

if __name__ == "__main__":
    if 'N' not in globals():
        N = 1000
    text = "the quick brown fox jumps over the lazy dog and the quick brown fox jumps again and again"
    start = time.time()
    counts = word_count(text, N)
    end = time.time()
    print(counts.get('the'))
    print(f"Time: {end - start}s")
