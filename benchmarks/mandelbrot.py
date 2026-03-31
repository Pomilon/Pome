import time
import sys

def mandelbrot(size):
    total_sum = 0
    for y in range(size):
        ci = (2.0 * y / size) - 1.0
        for x in range(size):
            cr = (2.0 * x / size) - 1.5
            zr = 0.0
            zi = 0.0
            i = 0
            while i < 50 and (zr*zr + zi*zi) < 4.0:
                tr = zr*zr - zi*zi + cr
                zi = 2.0 * zr * zi + ci
                zr = tr
                i += 1
            total_sum += i
    return total_sum

if __name__ == "__main__":
    if 'N' not in globals():
        N = 200
    start = time.time()
    s = mandelbrot(N)
    end = time.time()
    print(s)
    print(f"Time: {end - start}s")
