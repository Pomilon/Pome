import sys
import subprocess
import time
import os
import resource

# Configuration
POME_BIN = "./build_new/pome"
BENCH_DIR = "benchmarks"

def run_benchmark(script_path):
    print(f"Running {script_path}...")
    
    start_time = time.time()
    
    # Use resource usage tracking
    rusage_before = resource.getrusage(resource.RUSAGE_CHILDREN)
    
    try:
        proc = subprocess.run([POME_BIN, script_path], capture_output=True, text=True)
        rc = proc.returncode
        output = proc.stdout
        err = proc.stderr
    except Exception as e:
        print(f"Failed to run: {e}")
        return

    end_time = time.time()
    rusage_after = resource.getrusage(resource.RUSAGE_CHILDREN)

    wall_time = end_time - start_time
    # Note: RUSAGE_CHILDREN accumulates for all children, so this is diff
    # But if we run sequentially, diff is approx correct for this process if it's the only child.
    # However, Python's resource.getrusage(RUSAGE_CHILDREN) returns usage for *all terminated* child processes.
    # We need to take a snapshot before and after.
    
    user_time = rusage_after.ru_utime - rusage_before.ru_utime
    sys_time = rusage_after.ru_stime - rusage_before.ru_stime
    max_rss = rusage_after.ru_maxrss # This is max for *any* child, not necessarily this one if others ran before and were larger?
    # Actually on Linux ru_maxrss is in KB. 
    
    # A more accurate way for a specific process max-rss is difficult without wrapping it in `/usr/bin/time -v`.
    # Let's try to use `/usr/bin/time -v` if available, parsing stderr.
    
    if rc != 0:
        print(f"Error (Exit {rc}):")
        print(err)
        return

    print(f"  Wall Time: {wall_time:.4f}s")
    # print(f"  User Time: {user_time:.4f}s")
    # print(f"  Sys Time:  {sys_time:.4f}s")
    
    # To get Peak RSS specifically for this run, we might just trust the OS or use a wrapper.
    # Simple Python implementation for now.
    
    # Parse script output for internal timer if available
    for line in output.splitlines():
        if "Time:" in line:
            print(f"  Internal:  {line.strip()}")

    print("-" * 40)

def main():
    if not os.path.exists(POME_BIN):
        print(f"Error: Binary {POME_BIN} not found. Did you build?")
        return

    scripts = [f for f in os.listdir(BENCH_DIR) if f.endswith(".pome")]
    scripts.sort()

    print(f"Benchmarking Pome (Bin: {POME_BIN})")
    print("=" * 40)

    for s in scripts:
        run_benchmark(os.path.join(BENCH_DIR, s))

if __name__ == "__main__":
    main()
