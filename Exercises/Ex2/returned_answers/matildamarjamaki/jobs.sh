
# Bash job script (jobs.sh)
#!/bin/bash

n=10
output_dir="outputs"
mkdir -p "$output_dir"

for ((i=1; i<=n; i++)); do
    ./hello $i > "$output_dir/output_$i.txt" &
done

wait

echo "All jobs completed. Outputs are in the '$output_dir' directory."