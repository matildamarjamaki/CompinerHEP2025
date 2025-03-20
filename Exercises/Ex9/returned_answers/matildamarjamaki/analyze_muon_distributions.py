
import numpy as np
import matplotlib.pyplot as plt

# Load the data
filename = "muon_distributions.txt"
data = np.loadtxt(filename)

# Extract transverse momentum (pT) and pseudorapidity (eta)
pT = data[:, 0]
eta = data[:, 1]

# Plot transverse momentum distribution
plt.figure(figsize=(10, 5))
plt.hist(pT, bins=50, range=(0, 10), density=True, alpha=0.7, color='b', label='Muons $p_T$')
plt.xlabel("Transverse Momentum $p_T$ [GeV]")
plt.ylabel("Probability Density")
plt.title("Muon Transverse Momentum Distribution")
plt.legend()
plt.grid()
plt.savefig("muon_pT_distribution.png")
plt.show()

# Plot pseudorapidity distribution
plt.figure(figsize=(10, 5))
plt.hist(eta, bins=50, range=(-3, 3), density=True, alpha=0.7, color='r', label='Muons $\eta$')
plt.xlabel("Pseudorapidity $\eta$")
plt.ylabel("Probability Density")
plt.title("Muon Pseudorapidity Distribution")
plt.legend()
plt.grid()
plt.savefig("muon_eta_distribution.png")
plt.show()
