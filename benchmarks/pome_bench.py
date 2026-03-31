import subprocess
import time
import os
import sys

POME_BIN = sys.argv[1] if len(sys.argv) > 1 else "./build_new/pome"
PYTHON_BIN = "python3"

def run_bench(cmd, scale=None, is_pome=True):
    try:
        # Create a temporary file with the scale variable injected
        script_path = cmd[-1]
        with open(script_path, 'r') as f:
            content = f.read()
        
        tmp_script = f"benchmarks/tmp_bench{'.pome' if is_pome else '.py'}"
        with open(tmp_script, 'w') as f:
            if scale is not None:
                if is_pome:
                    f.write(f"var N = {scale};\n")
                else:
                    f.write(f"N = {scale}\n")
            f.write(content)
        
        new_cmd = list(cmd)
        new_cmd[-1] = tmp_script
        
        start = time.time()
        proc = subprocess.run(new_cmd, capture_output=True, text=True, timeout=120)
        end = time.time()
        
        if os.path.exists(tmp_script):
            os.remove(tmp_script)

        if proc.returncode != 0:
            return None, proc.stderr
        return end - start, proc.stdout
    except subprocess.TimeoutExpired:
        if os.path.exists(tmp_script): os.remove(tmp_script)
        return -1, "Timeout"
    except Exception as e:
        if 'tmp_script' in locals() and os.path.exists(tmp_script): os.remove(tmp_script)
        return None, str(e)

def print_result(name, pome_time, py_time):
    if pome_time is None or pome_time <= 0:
        pome_s = "TIMEOUT" if pome_time == -1 else "FAILED"
        print(f"{name:25} | {pome_s:9} | {py_time if py_time is not None else 'N/A':8} | {'N/A'}")
    elif py_time is None or py_time <= 0:
        print(f"{name:25} | {pome_time:8.4f}s | {'FAILED':8} | {'N/A'}")
    else:
        ratio = py_time / pome_time
        print(f"{name:25} | {pome_time:8.4f}s | {py_time:8.4f}s | {ratio:6.2f}x")

def main():
    # (Name, pome_file, py_file, scales)
    benchmarks = [
        ("Fib (Recursive)", "benchmarks/fib.pome", "benchmarks/fib.py", [10, 20, 30, 35, 40]),
        ("Loop (Local)", "benchmarks/loop_test_local.pome", "benchmarks/loop_test_local.py", [1000000, 5000000]),
        ("Object Creation", "benchmarks/object_creation.pome", "benchmarks/object_creation.py", [50000, 100000, 200000]),
        ("List Operations", "benchmarks/list_bench.pome", "benchmarks/list_bench.py", [50000, 100000, 200000]),
        ("Table Operations", "benchmarks/table_bench.pome", "benchmarks/table_bench.py", [5000, 10000, 20000]),
    ]

    print(f"{'Benchmark (Scale)':25} | {'Pome':9} | {'Python':9} | {'Ratio':7}")
    print("-" * 65)

    for name, pome_file, py_file, scales in benchmarks:
        for scale in scales:
            label = f"{name} ({scale})"
            pome_t, pome_err = run_bench([POME_BIN, pome_file], scale, True)
            py_t, py_err = run_bench([PYTHON_BIN, py_file], scale, False)

            if pome_t is None:
                print(f"{label:25} | {'FAILED':9} | {py_t if py_t is not None else 'N/A':8} | {'N/A'}")
                if pome_err: print(f"  Pome Error: {pome_err.strip()}")
            elif py_t is None:
                print(f"{label:25} | {pome_t:8.4f}s | {'FAILED':8} | {'N/A'}")
                if py_err: print(f"  Python Error: {py_err.strip()}")
            else:
                print_result(label, pome_t, py_t)


if __name__ == "__main__":
    main()
