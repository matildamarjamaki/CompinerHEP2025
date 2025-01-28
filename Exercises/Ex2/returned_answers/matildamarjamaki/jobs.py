
# Python job script (jobs.py)
import subprocess
import os

# Create the 'outputs' directory if it doesn't exist
if not os.path.exists('outputs'):
    os.makedirs('outputs')

# Run 10 jobs in parallel with different input numbers
processes = []
for i in range(1, 11):
    # Save output to the 'outputs' directory
    with open(f"outputs/output_{i}.txt", "w") as outfile:
        print(f"Starting job {i}...")
        processes.append(subprocess.Popen(["./hello", str(i)], stdout=outfile))

# Wait for all jobs to finish
for p in processes:
    p.wait()

print("All jobs completed. Outputs are in the 'outputs' directory.")