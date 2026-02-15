import sys
import subprocess
import time
import os

POME_BIN = "./build_new/pome"
PYTHON_BIN = "python3"

def run_pome():
    print(f"Running Pome Loop (10M iterations)...")
    start = time.time()
    try:
        # Using run_timeout tool logic or just subprocess directly since we fixed the loop bug
        proc = subprocess.run([POME_BIN, "benchmarks/loop_test.pome"], capture_output=True, text=True)
        if proc.returncode != 0:
            print("Pome failed:")
            print(proc.stderr)
            return None
    except Exception as e:
        print(f"Pome execution error: {e}")
        return None
    end = time.time()
    return end - start

def run_python():
    print(f"Running Python Loop (10M iterations)...")
    # We run the python script which has internal timing, but let's time the process too for fairness (startup time)
    start = time.time()
    proc = subprocess.run([PYTHON_BIN, "benchmarks/loop_test.py"], capture_output=True, text=True)
    end = time.time()
    # print(proc.stdout)
    return end - start

def main():
    pome_time = run_pome()
    py_time = run_python()
    
    print("\n--- Results ---")
    if pome_time is not None:
        print(f"Pome:   {pome_time:.4f}s")
    else:
        print("Pome:   Failed")
        
    print(f"Python: {py_time:.4f}s")
    
    if pome_time:
        ratio = py_time / pome_time
        print(f"Speedup vs Python: {ratio:.2f}x (Higher is better for Pome)")
        if pome_time < py_time:
            print("Pome is FASTER!")
        else:
            print("Pome is slower.")

if __name__ == "__main__":
    main()
