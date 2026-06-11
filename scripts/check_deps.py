"""
check_deps.py
Called by the Makefile before any analysis script runs.
Exits 0 if all packages are present, 1 if any are missing.
"""
import importlib.util, sys

REQUIRED = ["pandas", "matplotlib", "numpy"]
missing = [p for p in REQUIRED if importlib.util.find_spec(p) is None]

if missing:
    print(f"\n[ERROR] Missing Python packages: {', '.join(missing)}")
    print("        Run:  make install_req\n")
    sys.exit(1)

sys.exit(0)
