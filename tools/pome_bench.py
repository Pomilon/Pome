import subprocess
import time
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), ".."))

POME_BIN = sys.argv[1] if len(sys.argv) > 1 else "./build/pome"
PYTHON_BIN = "python3"

import re

def run_bench(cmd, scale=None, is_pome=True):
    tmp_script = None
    try:
        script_path = cmd[-1]
        with open(script_path, 'r') as f:
            content = f.read()
        
        # Unique temp file for safety
        ext = '.pome' if is_pome else '.py'
        tmp_script = f"benchmarks/tmp_bench_{int(time.time() * 1000)}{ext}"
        
        if scale is not None:
            if is_pome:
                # Replace any "var N = ...;" or "if (N == nil) var N = ...;" 
                pattern = r"(if\s*\(N\s*==\s*nil\)\s*)?var\s*N\s*=\s*[^;]+;?"
                new_line = f"var N = {scale};"
                if re.search(pattern, content):
                    content = re.sub(pattern, new_line, content)
                else:
                    content = f"var N = {scale};\n" + content
            else:
                # For python, replace "if 'N' not in globals(): N = ..." or "N = ..."
                pattern = r"(if\s*'N'\s*not\s*in\s*globals\(\):\s*)?N\s*=\s*[^\n]+"
                new_line = f"N = {scale}"
                if re.search(pattern, content, re.MULTILINE):
                    content = re.sub(pattern, new_line, content, count=1, flags=re.MULTILINE)
                else:
                    content = f"N = {scale}\n" + content
        
        with open(tmp_script, 'w') as f:
            f.write(content)
        
        new_cmd = list(cmd)
        new_cmd[-1] = tmp_script
        
        start = time.time()
        # Increased timeout for heavier benchmarks
        proc = subprocess.run(new_cmd, capture_output=True, text=True, timeout=180)
        end = time.time()
        
        if os.path.exists(tmp_script):
            os.remove(tmp_script)

        if proc.returncode != 0:
            return None, proc.stderr
        
        # Try to find "Time: X.XXXs" in output for more precise VM-only time if available
        # Otherwise use wall clock from subprocess
        output = proc.stdout
        for line in output.split('\n'):
            if "Time: " in line and "s" in line:
                try:
                    t_str = line.split("Time: ")[1].split("s")[0]
                    return float(t_str), output
                except:
                    pass
                    
        return end - start, output
    except subprocess.TimeoutExpired:
        if tmp_script and os.path.exists(tmp_script): os.remove(tmp_script)
        return -1, "Timeout"
    except Exception as e:
        if tmp_script and os.path.exists(tmp_script): os.remove(tmp_script)
        return None, str(e)

def normalize_output(output):
    # Remove lines containing purely environmental info
    lines = output.strip().split('\n')
    filtered = []
    for line in lines:
        line = line.strip()
        if not line: continue
        if any(x in line for x in ["Time: ", "GC Info: "]):
            continue
        if line.startswith("import "):
            continue
        
        # Aggressive number normalization: replace all numbers with 4-decimal-place floats
        def num_repl(match):
            try:
                val = float(match.group(0))
                # If it's effectively an integer, show no decimals
                if val == int(val):
                    return str(int(val))
                return "{:.4f}".format(val).rstrip('0').rstrip('.')
            except:
                return match.group(0)
        
        line = re.sub(r"-?\d+(\.\d+)?([eE][-+]?\d+)?", num_repl, line)
        
        filtered.append(line)
    return "\n".join(filtered)

def print_result(name, pome_time, py_time, match, pome_out, py_out):
    match_str = "YES" if match else "NO"
    p_out = normalize_output(pome_out).replace("\n", " ")[:30]
    py_out_norm = normalize_output(py_out).replace("\n", " ")[:30]
    
    if pome_time is None or pome_time <= 0:
        pome_s = "TIMEOUT" if pome_time == -1 else "FAILED"
        print(f"{name:35} | {pome_s:9} | {py_time if py_time is not None else 'N/A':8.4f}s | {'N/A':7} | {match_str:5} | {p_out:30} | {py_out_norm:30}")
    elif py_time is None or py_time <= 0:
        print(f"{name:35} | {pome_time:8.4f}s | {'FAILED':8} | {'N/A':7} | {match_str:5} | {p_out:30} | {py_out_norm:30}")
    else:
        ratio = py_time / pome_time
        print(f"{name:35} | {pome_time:8.4f}s | {py_time:8.4f}s | {ratio:6.2f}x | {match_str:5} | {p_out:30} | {py_out_norm:30}")

def main():
    # (Name, pome_file, py_file, scales)
    benchmarks = [
        ("Fib (Recursive)", "benchmarks/fib.pome", "benchmarks/fib.py", [30, 35]),
        ("Loop (Local)", "benchmarks/loop_test_local.pome", "benchmarks/loop_test_local.py", [5000000]),
        ("Object Creation", "benchmarks/object_creation.pome", "benchmarks/object_creation.py", [100000]),
        ("List Operations", "benchmarks/list_bench.pome", "benchmarks/list_bench.py", [100000]),
        ("Table Operations", "benchmarks/table_bench.pome", "benchmarks/table_bench.py", [10000]),
        ("Binary Tree", "benchmarks/binary_tree.pome", "benchmarks/binary_tree.py", [12, 14]),
        ("Merge Sort", "benchmarks/merge_sort.pome", "benchmarks/merge_sort.py", [5000, 10000]),
        ("Quick Sort", "benchmarks/quick_sort.pome", "benchmarks/quick_sort.py", [5000, 10000]),
        ("Sieve of Eratosthenes", "benchmarks/sieve.pome", "benchmarks/sieve.py", [100000, 200000]),
        ("N-Body Simulation", "benchmarks/nbody.pome", "benchmarks/nbody.py", [1000, 2000]),
        ("Mandelbrot Set", "benchmarks/mandelbrot.pome", "benchmarks/mandelbrot.py", [200, 300]),
        ("Dijkstra Graph", "benchmarks/dijkstra.pome", "benchmarks/dijkstra.py", [200, 400]),
        ("Graph BFS", "benchmarks/graph_bfs.pome", "benchmarks/graph_bfs.py", [10000, 20000]),
        ("Matrix Multiplication", "benchmarks/matrix_mul.pome", "benchmarks/matrix_mul.py", [100, 150]),
        ("Spectral Norm", "benchmarks/spectral_norm.pome", "benchmarks/spectral_norm.py", [100, 200]),
        ("Fannkuch-Redux", "benchmarks/fannkuch.pome", "benchmarks/fannkuch.py", [8, 9]),
        ("Monte Carlo Pi", "benchmarks/monte_carlo_pi.pome", "benchmarks/monte_carlo_pi.py", [100000, 200000]),
        ("Game of Life", "benchmarks/game_of_life.pome", "benchmarks/game_of_life.py", [50, 70]),
        ("JSON Parse", "benchmarks/json_parse.pome", "benchmarks/json_parse.py", [1000, 2000]),
        ("N-Queens", "benchmarks/n_queens.pome", "benchmarks/n_queens.py", [10, 11]),
        ("Word Count", "benchmarks/word_count.pome", "benchmarks/word_count.py", [1000, 2000]),
        ("Linked List Stress", "benchmarks/linked_list.pome", "benchmarks/linked_list.py", [10000, 20000]),
        ("Data Analysis", "benchmarks/data_analysis.pome", "benchmarks/data_analysis.py", [1000, 2000]),
        ("H-Tree Recursion", "benchmarks/h_tree.pome", "benchmarks/h_tree.py", [10, 11]),
        ("Bubble Sort", "benchmarks/bubble_sort.pome", "benchmarks/bubble_sort.py", [1000, 2000]),
        ("Binary Search", "benchmarks/binary_search.pome", "benchmarks/binary_search.py", [10000, 20000]),
    ]

    print(f"{'Benchmark (Scale)':35} | {'Pome':9} | {'Python':9} | {'Ratio':7} | {'Match'} | {'Pome Out':30} | {'Py Out':30}")
    print("-" * 155)

    for name, pome_file, py_file, scales in benchmarks:
        for scale in scales:
            label = f"{name} ({scale})"
            pome_t, pome_out = run_bench([POME_BIN, pome_file], scale, True)
            py_t, py_out = run_bench([PYTHON_BIN, py_file], scale, False)

            match = False
            pome_norm = normalize_output(pome_out)
            py_norm = normalize_output(py_out)
            if pome_t is not None and py_t is not None:
                # Compare full strings but allow some fuzzy matching for known environmental noise
                match = pome_norm == py_norm

            if pome_t is None:
                p_disp = pome_out.replace("\n", " ")[:30] if pome_out else "N/A"
                print(f"{label:35} | {'FAILED':9} | {py_t if py_t is not None and py_t > 0 else 'N/A':8.4f}s | {'N/A':7} | {'NO':5} | {p_disp:30} | {'N/A':30}")
            elif py_t is None:
                p_disp = pome_norm.replace("\n", " ")[:30]
                print(f"{label:35} | {pome_t:8.4f}s | {'FAILED':8} | {'N/A':7} | {'NO':5} | {p_disp:30} | {'FAILED':30}")
            else:
                p_disp = pome_norm.replace("\n", " ")[:30]
                py_disp = py_norm.replace("\n", " ")[:30]
                ratio = py_t / pome_t
                print(f"{label:35} | {pome_t:8.4f}s | {py_t:8.4f}s | {ratio:6.2f}x | {'YES' if match else 'NO':5} | {p_disp:30} | {py_disp:30}")


if __name__ == "__main__":
    main()
