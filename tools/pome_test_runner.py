import os
import subprocess
import sys

# Configuration
POME_BIN = "./build_new/pome"
TEST_DIR = "test/unit_tests"

def run_test(test_path):
    print(f"Testing {test_path}...", end=" ", flush=True)
    
    # Some tests are expected to fail (like strict mode violation)
    is_fail_test = "fail" in test_path
    
    try:
        proc = subprocess.run([POME_BIN, test_path], capture_output=True, text=True, timeout=5)
        
        if is_fail_test:
            if proc.returncode != 0:
                print("PASS (Expected Failure)")
                return True
            else:
                print("FAIL (Expected error but got 0)")
                return False
        else:
            if proc.returncode == 0:
                print("PASS")
                return True
            else:
                print(f"FAIL (Exit {proc.returncode})")
                print(proc.stderr)
                return False
                
    except subprocess.TimeoutExpired:
        print("FAIL (Timeout)")
        return False
    except Exception as e:
        print(f"FAIL (Error: {e})")
        return False

def main():
    if not os.path.exists(POME_BIN):
        print(f"Error: Binary {POME_BIN} not found.")
        sys.exit(1)

    tests = [os.path.join(TEST_DIR, f) for f in os.listdir(TEST_DIR) if f.endswith(".pome")]
    tests.sort()

    print(f"Pome Test Runner")
    print("=" * 40)

    results = [run_test(t) for f in tests] # Wait, typo in loop variable
    # Fixed below
    
if __name__ == "__main__":
    # Corrected main logic
    if not os.path.exists(POME_BIN):
        print(f"Error: Binary {POME_BIN} not found.")
        sys.exit(1)

    tests = [os.path.join(TEST_DIR, f) for f in os.listdir(TEST_DIR) if f.endswith(".pome")]
    tests.sort()

    passed = 0
    for t in tests:
        if run_test(t):
            passed += 1
            
    print("-" * 40)
    print(f"Result: {passed}/{len(tests)} passed")
    
    if passed == len(tests):
        sys.exit(0)
    else:
        sys.exit(1)
