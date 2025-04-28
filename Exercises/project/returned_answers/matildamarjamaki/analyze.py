
import ROOT
from ROOT import TF1, TFile, TCanvas, TLegend
import numpy as np

LUMINOSITY = 10000  # fb^-1

def read_metadata(file):
    try:
        sigma = float(file.Get("cross_section").GetTitle())  # fb
        n_gen = int(file.Get("n_generated").GetTitle())
        return sigma, n_gen
    except Exception as e:
        raise RuntimeError(f"Error reading metadata: {e}")

# Open ROOT files safely
f_sig = TFile.Open("signal.root")
f_bkg = TFile.Open("background.root")
if not f_sig or f_sig.IsZombie():
    raise FileNotFoundError("Failed to open signal.root")
if not f_bkg or f_bkg.IsZombie():
    raise FileNotFoundError("Failed to open background.root")

# Get histograms and metadata
h_sig = f_sig.Get("h_mass")
h_bkg = f_bkg.Get("h_mass")
if not h_sig or not h_bkg:
    raise RuntimeError("Histogram 'h_mass' not found in input files.")
SIGMA_SIGNAL, N_GEN_SIGNAL = read_metadata(f_sig)
SIGMA_BKG, N_GEN_BKG = read_metadata(f_bkg)

# Normalize histograms
scale_sig = (SIGMA_SIGNAL * LUMINOSITY) / N_GEN_SIGNAL
scale_bkg = (SIGMA_BKG * LUMINOSITY) / N_GEN_BKG
h_sig.Scale(scale_sig)
h_bkg.Scale(scale_bkg)

# Create combined histogram
h_sum = h_sig.Clone("h_sum")
h_sum.Add(h_bkg)

# Draw histograms

c = TCanvas("c", "Invariant Mass", 800, 600)
c.SetLogy()  # make y-axis logarithmic

h_sig.SetLineColor(ROOT.kRed)
h_bkg.SetLineColor(ROOT.kBlue)
h_sum.SetLineColor(ROOT.kBlack)
for hist in (h_sig, h_bkg, h_sum):
    hist.SetStats(0)

h_sig.SetTitle("Invariant Mass Spectrum")
h_sig.GetXaxis().SetTitle("M_{#mu#mu} [GeV]")
h_sig.GetYaxis().SetTitle("Events / bin")
h_sig.GetXaxis().SetTitleSize(0.045)
h_sig.GetYaxis().SetTitleSize(0.045)

h_sig.Draw("HIST")
h_bkg.Draw("HIST SAME")
h_sum.Draw("HIST SAME")
h_sig.Draw("E1 SAME")
h_bkg.Draw("E1 SAME")

# Add legend
legend = TLegend(0.55, 0.65, 0.85, 0.85)
legend.AddEntry(h_sig, "Signal (Z → #mu#mu)", "l")
legend.AddEntry(h_bkg, "Background (t#bar{t})", "l")
legend.AddEntry(h_sum, "Total", "l")
legend.Draw()

# Fit signal + background
fit_min, fit_max = 60, 120
fit = TF1("fit", "[0]*exp(-0.01*x) + [1]*exp(-0.5*((x-[2])/[3])**2)", fit_min, fit_max)
fit.SetParameters(100, 1000, 91.2, 2.5)
fit.SetParNames("bkg_norm", "sig_norm", "mean", "sigma")
h_sum.Fit(fit, "R")

# Define mass window and calculate integrals
mass_min, mass_max = 80, 100
bin_width = h_sum.GetBinWidth(1)

N_total = fit.Integral(mass_min, mass_max) / bin_width

# Estimate background
bkg_func = TF1("bkg", "[0]*exp(-0.01*x)", fit_min, fit_max)
bkg_func.SetParameter(0, fit.GetParameter(0))
N_bkg = bkg_func.Integral(mass_min, mass_max) / bin_width

# Signal count and significance
N_sig = N_total - N_bkg
significance = N_sig / np.sqrt(N_bkg) if N_bkg > 0 else 0

# Required luminosity for 5σ
required_lumi = (5 / significance) ** 2 * LUMINOSITY if significance > 0 else float("inf")

# Print results
print(f"N_S = {N_sig:.1f}, N_B = {N_bkg:.1f}")
print(f"Significance: {significance:.2f} sigma")
print(f"Required luminosity for 5σ: {required_lumi:.6f} fb^-1")

# Save canvas
c.SaveAs("invariant_mass.pdf")




# import ROOT
# from ROOT import TF1, TFile, TCanvas, TLegend
# import numpy as np

# LUMINOSITY = 10000  # fb^-1

# def read_metadata(file):
#     sigma = float(file.Get("cross_section").GetTitle())  # fb
#     n_gen = int(file.Get("n_generated").GetTitle())
#     return sigma, n_gen

# # Open ROOT files
# f_sig = TFile.Open("signal.root")
# f_bkg = TFile.Open("background.root")
# if not f_sig or f_sig.IsZombie():
#     raise FileNotFoundError("Failed to open signal.root")
# if not f_bkg or f_bkg.IsZombie():
#     raise FileNotFoundError("Failed to open background.root")

# # Get histograms
# h_sig = f_sig.Get("h_mass")
# h_bkg = f_bkg.Get("h_mass")
# if not h_sig or not h_bkg:
#     raise RuntimeError("Histogram 'h_mass' not found in input files.")

# # Metadata
# SIGMA_SIGNAL, N_GEN_SIGNAL = read_metadata(f_sig)
# SIGMA_BKG, N_GEN_BKG = read_metadata(f_bkg)

# print(f"Signal cross section: {SIGMA_SIGNAL:.6f} fb, generated: {N_GEN_SIGNAL}")
# print(f"Background cross section: {SIGMA_BKG:.6f} fb, generated: {N_GEN_BKG}")

# # Normalize
# scale_sig = (SIGMA_SIGNAL * LUMINOSITY) / N_GEN_SIGNAL
# scale_bkg = (SIGMA_BKG * LUMINOSITY) / N_GEN_BKG
# h_sig.Scale(scale_sig)
# h_bkg.Scale(scale_bkg)

# # Combine
# h_sum = h_sig.Clone("h_sum")
# h_sum.Add(h_bkg)

# # Draw
# c = TCanvas("c", "Invariant Mass", 800, 600)
# c.SetLogy()  # make y-axis logarithmic

# h_sig.SetLineColor(ROOT.kRed)
# h_bkg.SetLineColor(ROOT.kBlue)
# h_sum.SetLineColor(ROOT.kBlack)
# for hist in (h_sig, h_bkg, h_sum):
#     hist.SetStats(0)

# h_sum.SetTitle("Invariant Mass Spectrum (Signal + Background)")
# h_sum.GetXaxis().SetTitle("M_{#mu#mu} [GeV]")
# h_sum.GetYaxis().SetTitle("Events / bin")
# h_sum.GetXaxis().SetTitleSize(0.045)
# h_sum.GetYaxis().SetTitleSize(0.045)
# h_sum.Draw("E1")

# # Add histograms separately
# h_sig.Draw("HIST SAME")
# h_bkg.Draw("HIST SAME")

# # Legend
# legend = TLegend(0.55, 0.65, 0.85, 0.85)
# legend.AddEntry(h_sig, "Signal (Z → #mu#mu)", "l")
# legend.AddEntry(h_bkg, "Background (t#bar{t})", "l")
# legend.AddEntry(h_sum, "Total", "lep")
# legend.Draw()

# # --- Fitting ---

# # Background shape: fit exponential on h_bkg first
# fit_min, fit_max = 60, 120

# bkg_model = TF1("bkg_model", "[0]*exp([1]*x + [2]*x*x)", 60, 120)
# bkg_model.SetParameters(1, -0.01, 0)
# h_bkg.Fit(bkg_model, "R")

# # Get background parameters
# bkg_norm = bkg_model.GetParameter(0)
# bkg_slope = bkg_model.GetParameter(1)

# # Now build signal+background fit model
# fit_model = TF1("fit_model", "[0]*exp([1]*x + [2]*x*x) + [3]*exp(-0.5*((x-[4])/[5])**2)", 60, 120)
# fit_model.SetParameters(1, -0.01, 0, 1000, 91.2, 2.5)
# fit_model.SetParNames("bkg_norm", "bkg_slope1", "bkg_slope2", "sig_norm", "mean", "sigma")

# # Fix background shape to what was found
# fit_model.FixParameter(1, bkg_slope)

# # Fit signal+background
# h_sum.Fit(fit_model, "R")

# # Mass window
# mass_min, mass_max = 80, 100
# bin_width = h_sum.GetBinWidth(1)

# # Integrals
# N_total = fit_model.Integral(mass_min, mass_max) / bin_width

# # Background-only contribution
# bkg_only = TF1("bkg_only", "[0]*exp([1]*x)", fit_min, fit_max)
# bkg_only.SetParameter(0, fit_model.GetParameter(0))
# bkg_only.SetParameter(1, fit_model.GetParameter(1))
# N_bkg = bkg_only.Integral(mass_min, mass_max) / bin_width

# # Signal estimation
# N_sig = N_total - N_bkg
# significance = N_sig / np.sqrt(N_bkg) if N_bkg > 0 else 0

# # Required luminosity for 5σ
# required_lumi = (5 / significance) ** 2 * LUMINOSITY if significance > 0 else float("inf")

# # Results
# print(f"N_S = {N_sig:.1f}, N_B = {N_bkg:.1f}")
# print(f"Significance: {significance:.2f} sigma")
# print(f"Required luminosity for 5σ: {required_lumi:.6f} fb^-1")

# # Save plot
# c.SaveAs("invariant_mass.pdf")
