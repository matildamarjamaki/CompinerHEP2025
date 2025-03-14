
#!/bin/bash

# Run FeynHiggs for different Higgs masses
for MH in {120..130}; do
    echo "Running FeynHiggs for MH = $MH GeV"
    FeynHiggs myinput.in -mh $MH > FeynHiggs_$MH.txt
done

# Run HDECAY for the same mass range
for MH in {120..130}; do
    echo "Running HDECAY for MH = $MH GeV"
    ./hdecay < HDECAY_input_$MH.in > HDECAY_$MH.txt
done