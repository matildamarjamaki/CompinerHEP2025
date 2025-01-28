
# Bash job script (jobs.sh)
#!/bin/bash

# Run 10 jobs in parallel with different input numbers
for i in {1..10}; do
    ./hello $i > output_$i.txt &  # Save output in output_1.txt, output_2.txt, ..., output_10.txt
done

wait  # Wait for all jobs to finish