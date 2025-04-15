
import uproot
import matplotlib.pyplot as plt

f = uproot.open("output.root")
pileup = f["h_pileup"]

# Get data
edges = pileup.axes[0].edges()
values = pileup.values()

# Plot
plt.bar(edges[:-1], values, width=1.0, align="edge")
plt.xlabel("Pileup (PV_npvs)")
plt.ylabel("Events")
plt.title("Pileup Distribution for Drell-Yan Events (HLT_IsoMu24)")
plt.grid(True)
plt.savefig("pileup_distribution.png")
plt.show()