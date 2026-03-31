import time
import sys
import math

class Body:
    __slots__ = ['x', 'y', 'z', 'vx', 'vy', 'vz', 'mass']
    def __init__(self, x, y, z, vx, vy, vz, mass):
        self.x = x
        self.y = y
        self.z = z
        self.vx = vx
        self.vy = vy
        self.vz = vz
        self.mass = mass

def advance(bodies, dt):
    for i in range(len(bodies)):
        b = bodies[i]
        for j in range(i + 1, len(bodies)):
            b2 = bodies[j]
            dx = b.x - b2.x
            dy = b.y - b2.y
            dz = b.z - b2.z
            distance = math.sqrt(dx*dx + dy*dy + dz*dz)
            mag = dt / (distance * distance * distance)
            
            b.vx -= dx * b2.mass * mag
            b.vy -= dy * b2.mass * mag
            b.vz -= dz * b2.mass * mag
            
            b2.vx += dx * b.mass * mag
            b2.vy += dy * b.mass * mag
            b2.vz += dz * b.mass * mag
            
    for b in bodies:
        b.x += dt * b.vx
        b.y += dt * b.vy
        b.z += dt * b.vz

if __name__ == "__main__":
    if 'N' not in globals():
        N = 1000
    bodies = [
        Body(0, 0, 0, 0, 0, 0, 1000), # Sun
        Body(10, 0, 0, 0, 1, 0, 1),   # Planet 1
        Body(0, 20, 0, -1, 0, 0, 0.5), # Planet 2
        Body(30, 30, 0, 0.5, -0.5, 0, 0.1) # Planet 3
    ]
    start = time.time()
    for _ in range(N):
        advance(bodies, 0.01)
    end = time.time()
    print(N)
    print(f"Time: {end - start}s")
