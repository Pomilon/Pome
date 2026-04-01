import subprocess
import time
import os

POME_BIN = "./build/pome"
PYTHON_BIN = "python3"
N = 10000000

def run_pome():
    # We need to prepend 'var N = 10000000;' to benchmarks/loop_test_local.pome
    with open("benchmarks/loop_test_local.pome", "r") as f:
        content = f.read()
    
    temp_pome = "benchmarks/temp_loop_local.pome"
    with open(temp_pome, "w") as f:
        f.write(f"var N = {N};\n")
        f.write(content)
        
    start = time.time()
    try:
        proc = subprocess.run([POME_BIN, temp_pome], capture_output=True, text=True)
        if proc.returncode != 0:
            print("Pome failed:")
            print(proc.stderr)
            return None
        # print(proc.stdout)
    except Exception as e:
        print(f"Pome execution error: {e}")
        return None
    finally:
        if os.path.exists(temp_pome):
            os.remove(temp_pome)
            
    end = time.time()
    return end - start

def run_python():
    # Python script already handles N if it's in globals, but for simplicity let's do the same
    with open("benchmarks/loop_test_local.py", "r") as f:
        content = f.read()
        
    temp_py = "benchmarks/temp_loop_local.py"
    with open(temp_py, "w") as f:
        f.write(f"N = {N}\n")
        f.write(content)
        
    start = time.time()
    proc = subprocess.run([PYTHON_BIN, temp_py], capture_output=True, text=True)
    end = time.time()
    
    if os.path.exists(temp_py):
        os.remove(temp_py)
        
    return end - start

def main():
    pome_time = run_pome()
    py_time = run_python()
    
    print("\n--- Loop (Local) 10M iterations ---")
    if pome_time is not None:
        print(f"Pome:   {pome_time:.4f}s")
    else:
        print("Pome:   Failed")
        
    print(f"Python: {py_time:.4f}s")
    
    if pome_time:
        ratio = py_time / pome_time
        print(f"Speedup vs Python: {ratio:.2f}x (Higher is better for Pome)")

if __name__ == "__main__":
    main()
