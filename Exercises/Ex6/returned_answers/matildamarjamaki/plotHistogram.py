
import ROOT

def plot_histogram():
    file = ROOT.TFile("random_numbers.root", "READ")
    if not file or file.IsZombie():
        print("Error: Cannot open file random_numbers.root")
        return

    tree = file.Get("tree")
    if not tree:
        print("Error: Cannot find TTree in file")
        return

    hist = ROOT.TH1D("hist", "Histogram of Random Numbers;Value;Frequency", 50, -4, 4)

    tree.Draw("random_number >> hist")

    hist.SetLineColor(ROOT.kBlack)
    hist.SetLineWidth(3)
    hist.SetFillColor(ROOT.kYellow)

    canvas = ROOT.TCanvas("canvas", "Histogram", 800, 600)
    canvas.SetFillColor(ROOT.kWhite)

    hist.Draw()

    fit_func = ROOT.TF1("fitFunc", "gaus", -4, 4)
    hist.Fit(fit_func, "R")

    canvas.SaveAs("histogram.png")

    file.Close()

if __name__ == "__main__":
    plot_histogram()
