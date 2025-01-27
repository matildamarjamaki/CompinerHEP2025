
# Python job script (jobs.py)
import os
import subprocess
from multiprocessing import Pool

def run_job(i):
    output_dir = "outputs"
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, f"output_{i}.txt")
    with open(output_file, "w") as f:
        subprocess.run(["./hello", str(i)], stdout=f)

if __name__ == "__main__":
    n = 10
    with Pool(n) as pool:
        pool.map(run_job, range(1, n + 1))

    print("All jobs completed. Outputs are in the 'outputs' directory.")