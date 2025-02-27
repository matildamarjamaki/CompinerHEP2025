
import ROOT
import numpy as np

def generate_tree():
    N = 1000

    file = ROOT.TFile("random_numbers.root", "RECREATE")

    tree = ROOT.TTree("tree", "Tree of Random Numbers")
    random_number = np.array([0.0], dtype=np.float64)
    tree.Branch("random_number", random_number, "random_number/D")

    rand_gen = ROOT.TRandom3()

    for _ in range(N):
        random_number[0] = rand_gen.Gaus(0, 1)
        tree.Fill()

    tree.Write()
    file.Close()

if __name__ == "__main__":
    generate_tree()
