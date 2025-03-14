
import numpy as np

masses = np.arange(120, 131, 1)
feyn_widths = []
hdecay_widths = []

for MH in masses:
    with open(f"FeynHiggs_{MH}.txt") as f:
        for line in f:
            if "Total width" in line:
                feyn_widths.append(float(line.split()[-1]))
    
    with open(f"HDECAY_{MH}.txt") as f:
        for line in f:
            if "Higgs width" in line:
                hdecay_widths.append(float(line.split()[-1]))

np.savetxt("higgs_widths.txt", np.column_stack((masses, feyn_widths, hdecay_widths)))