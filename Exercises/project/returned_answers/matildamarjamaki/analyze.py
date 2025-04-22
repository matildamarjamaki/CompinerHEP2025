
# import ROOT
# import numpy as np

# # -------------------
# # CONFIGURATION
# # -------------------
# lumi = 1000  # pb⁻¹ = 1 fb⁻¹
# signal_xsec = 2.0  # mb (will be overwritten by ROOT file)
# background_xsec = 0.5  # mb (ditto)
# nevents_signal = 100000
# nevents_background = 100000
# mass_window = (80, 100)  # GeV

# # -------------------
# # LOAD HISTOGRAMS
# # -------------------
# f_sig = ROOT.TFile.Open("signal.root")
# f_bkg = ROOT.TFile.Open("background.root")
# h_sig = f_sig.Get("h_mass")
# h_bkg = f_bkg.Get("h_mass")

# h_sig.SetDirectory(0)
# h_bkg.SetDirectory(0)

# f_sig.Close()
# f_bkg.Close()

# # -------------------
# # GET CROSS SECTIONS
# # -------------------
# with open("signal.out") as f:
#     for line in f:
#         if "Cross section:" in line:
#             signal_xsec = float(line.split(":")[1].split()[0])

# with open("background.out") as f:
#     for line in f:
#         if "Cross section:" in line:
#             background_xsec = float(line.split(":")[1].split()[0])

# # -------------------
# # NORMALIZATION
# # -------------------
# pb_conversion = 1e9  # 1 mb = 1e9 pb
# scale_sig = signal_xsec * pb_conversion * lumi / nevents_signal
# scale_bkg = background_xsec * pb_conversion * lumi / nevents_background

# h_sig.Scale(scale_sig)
# h_bkg.Scale(scale_bkg)

# # -------------------
# # PLOT
# # -------------------
# c1 = ROOT.TCanvas("c1", "Z --> #mu#mu", 800, 600)
# h_sig.SetLineColor(ROOT.kRed)
# h_bkg.SetLineColor(ROOT.kBlue)
# h_sig.SetFillColorAlpha(ROOT.kRed, 0.35)
# h_bkg.SetFillColorAlpha(ROOT.kBlue, 0.35)
# h_sig.SetStats(0)
# h_bkg.SetStats(0)
# h_sig.Draw("HIST")
# h_bkg.Draw("HIST SAME")
# h_sig.GetXaxis().SetTitle("M_{#mu#mu} [GeV]")
# h_sig.GetYaxis().SetTitle("Events / bin")
# h_sig.SetTitle("Z → #mu#mu Invariant Mass")
# legend = ROOT.TLegend(0.6, 0.75, 0.88, 0.88)
# legend.AddEntry(h_sig, "Z --> #mu#mu (signal)", "f")
# legend.AddEntry(h_bkg, "tt(bar) (background)", "f")
# legend.Draw()
# c1.SaveAs("z_peak.png")
# c2 = ROOT.TCanvas()
# h_bkg.Draw("HIST")
# c2.SaveAs("background_debug.png")


# # -------------------
# # INTEGRAL IN SIGNAL WINDOW
# # -------------------
# bin_lo = h_sig.FindBin(mass_window[0])
# bin_hi = h_sig.FindBin(mass_window[1])

# S = h_sig.Integral(bin_lo, bin_hi)
# B = h_bkg.Integral(bin_lo, bin_hi)
# significance = S / np.sqrt(B) if B > 0 else 0

# print(f"\n=== Analysis Summary ===")
# print(f"Integrated luminosity: {lumi:.1f} pb⁻¹")
# print(f"Signal cross section: {signal_xsec:.3e} mb")
# print(f"Background cross section: {background_xsec:.3e} mb")
# print(f"Scale factors: signal={scale_sig:.2f}, background={scale_bkg:.2f}")
# print(f"Mass window: {mass_window[0]}–{mass_window[1]} GeV")
# print(f"Signal yield (S): {S:.1f}")
# print(f"Background yield (B): {B:.1f}")
# print(f"Statistical significance (S/√B): {significance:.2f}")

# # -------------------
# # REQUIRED LUMINOSITY FOR 5σ
# # -------------------
# target_sigma = 5.0
# required_lumi = lumi * (target_sigma / significance)**2 if significance > 0 else float("inf")
# print(f"Required luminosity for 5σ: {required_lumi:.1f} pb⁻¹")


# import ROOT
# from ROOT import TF1, TFile, TH1F
# import numpy as np

# LUMINOSITY = 1000  # fb^-1, voit muuttaa tätä
# SIGMA_SIGNAL = 2000e3  # fb
# SIGMA_BKG = 830e3      # fb
# N_GEN_SIGNAL = 200000
# N_GEN_BKG = 200000

# f_sig = TFile.Open("signal_test.root")
# f_bkg = TFile.Open("background_test.root")
# h_sig = f_sig.Get("h_mass")
# h_bkg = f_bkg.Get("h_mass")

# # Normalisointi
# scale_sig = (SIGMA_SIGNAL * LUMINOSITY) / N_GEN_SIGNAL
# scale_bkg = (SIGMA_BKG * LUMINOSITY) / N_GEN_BKG
# h_sig.Scale(scale_sig)
# h_bkg.Scale(scale_bkg)

# # Piirretään yhteiskuvana
# c = ROOT.TCanvas("c", "Invariant Mass")
# h_sig.SetLineColor(ROOT.kRed)
# h_bkg.SetLineColor(ROOT.kBlue)
# h_sig.SetStats(0)
# h_bkg.SetStats(0)
# h_sig.Draw("HIST")
# h_bkg.Draw("HIST SAME")

# h_sum = h_sig.Clone("h_sum")
# h_sum.Add(h_bkg)

# # Sovitus signaali + tausta
# fit = TF1("fit", "[0]*exp(-0.01*x) + [1]*exp(-0.5*((x-[2])/[3])**2)", 60, 120)
# fit.SetParameters(100, 1000, 91.2, 2.5)
# h_sum.Fit(fit, "R")

# # Integroi massaikkunassa
# mass_min = 80
# mass_max = 100
# nbins = h_sum.GetXaxis().FindBin
# bin_min = nbins(mass_min)
# bin_max = nbins(mass_max)

# N_total = fit.Integral(mass_min, mass_max) / h_sum.GetBinWidth(1)
# # Taustafunktio
# bkg_func = TF1("bkg", "[0]*exp(-0.01*x)", 60, 120)
# bkg_func.SetParameter(0, fit.GetParameter(0))
# N_bkg = bkg_func.Integral(mass_min, mass_max) / h_sum.GetBinWidth(1)
# N_sig = N_total - N_bkg
# significance = N_sig / np.sqrt(N_bkg)

# print(f"N_S = {N_sig:.1f}, N_B = {N_bkg:.1f}")
# print(f"Significance: {significance:.2f} sigma")

# # Tarvittava luminositeetti 5σ:lla
# required_lumi = (5 / significance) ** 2 * LUMINOSITY
# print(f"Required luminosity for 5σ: {required_lumi:.1f} fb^-1")

# c.SaveAs("invariant_mass.pdf")  # tai .png jos haluat kuvatiedoston


# import ROOT
# from ROOT import TF1, TFile, TH1F, TLegend, TLatex
# import numpy as np

# # === Constants ===
# LUMINOSITY = 1000  # fb^-1
# SIGMA_SIGNAL = 2000e3  # fb
# SIGMA_BKG = 830e3      # fb
# N_GEN_SIGNAL = 200000
# N_GEN_BKG = 200000

# # === Load Histograms ===
# f_sig = TFile.Open("signal_test.root")
# f_bkg = TFile.Open("background_test.root")
# h_sig = f_sig.Get("h_mass")
# h_bkg = f_bkg.Get("h_mass")

# # === Normalize ===
# scale_sig = (SIGMA_SIGNAL * LUMINOSITY) / N_GEN_SIGNAL
# scale_bkg = (SIGMA_BKG * LUMINOSITY) / N_GEN_BKG
# h_sig.Scale(scale_sig)
# h_bkg.Scale(scale_bkg)

# # === Create Total Histogram ===
# h_sum = h_sig.Clone("h_sum")
# h_sum.Add(h_bkg)

# # === Style Setup ===
# h_sig.SetLineColor(ROOT.kRed)
# h_bkg.SetLineColor(ROOT.kBlue)
# h_sum.SetLineColor(ROOT.kBlack)
# h_sig.SetLineWidth(2)
# h_bkg.SetLineWidth(2)
# h_sum.SetLineWidth(2)

# # === Canvas & Draw ===
# c = ROOT.TCanvas("c", "Invariant Mass", 800, 600)
# h_sum.SetTitle("Invariant Mass Spectrum;M_{#mu#mu} [GeV];Events / bin")
# h_sum.Draw("HIST")
# h_bkg.Draw("HIST SAME")
# h_sig.Draw("HIST SAME")

# legend = TLegend(0.6, 0.7, 0.88, 0.88)
# legend.AddEntry(h_sig, "Signal (Z #rightarrow #mu#mu)", "l")
# legend.AddEntry(h_bkg, "Background (t#bar{t})", "l")
# legend.AddEntry(h_sum, "Total", "l")
# legend.Draw()

# # === Fit Signal + Background ===
# fit_func = TF1("fit", "[0]*exp(-0.01*x) + [1]*exp(-0.5*((x-[2])/[3])**2)", 60, 120)
# fit_func.SetParameters(100, 1000, 91.2, 2.5)
# h_sum.Fit(fit_func, "R")

# # === Integrate in mass window ===
# mass_min = 80
# mass_max = 100
# bin_width = h_sum.GetBinWidth(1)
# N_total = fit_func.Integral(mass_min, mass_max) / bin_width

# # Background-only function using fitted parameter
# bkg_func = TF1("bkg_func", "[0]*exp(-0.01*x)", 60, 120)
# bkg_func.SetParameter(0, fit_func.GetParameter(0))
# N_bkg = bkg_func.Integral(mass_min, mass_max) / bin_width
# N_sig = N_total - N_bkg
# significance = N_sig / np.sqrt(N_bkg)

# # === Output ===
# print(f"N_S = {N_sig:.1f}, N_B = {N_bkg:.1f}")
# print(f"Significance: {significance:.2f} sigma")
# required_lumi = (5 / significance) ** 2 * LUMINOSITY
# print(f"Required luminosity for 5σ: {required_lumi:.1f} fb^-1")

# # === Annotation on plot ===
# label = TLatex()
# label.SetNDC()
# label.SetTextSize(0.04)
# label.DrawLatex(0.12, 0.85, f"N_S = {N_sig:.0f}, N_B = {N_bkg:.0f}, S/#sqrt{{B}} = {significance:.2f}")

# # === Save ===
# c.SaveAs("invariant_mass.pdf")


import ROOT
from ROOT import TF1, TFile, TH1F, TCanvas, TLegend
import numpy as np

# # Asetukset
# LUMINOSITY = 50  # fb^-1
# SIGMA_SIGNAL = 2000e3  # fb
# SIGMA_BKG = 830e3      # fb
# N_GEN_SIGNAL = 200000
# N_GEN_BKG = 200000

# # Ladataan histogrammit
# f_sig = TFile.Open("signal_test.root")
# f_bkg = TFile.Open("background_test.root")
# h_sig = f_sig.Get("h_mass")
# h_bkg = f_bkg.Get("h_mass")

import ROOT
from ROOT import TF1, TFile, TH1F, TCanvas, TLegend
import numpy as np

LUMINOSITY = 50  # fb^-1

def read_metadata(file):
    sigma = float(file.Get("cross_section").GetTitle())  # fb
    n_gen = int(file.Get("n_generated").GetTitle())
    return sigma, n_gen

# Ladataan histogrammit ja metatiedot
f_sig = TFile.Open("signal_test.root")
f_bkg = TFile.Open("background_test.root")
h_sig = f_sig.Get("h_mass")
h_bkg = f_bkg.Get("h_mass")
SIGMA_SIGNAL, N_GEN_SIGNAL = read_metadata(f_sig)
SIGMA_BKG, N_GEN_BKG = read_metadata(f_bkg)

# Normalisointi: skaalataan histogrammit ristiinvaikutuspinta-alan ja luminositeetin mukaan
scale_sig = (SIGMA_SIGNAL * LUMINOSITY) / N_GEN_SIGNAL
scale_bkg = (SIGMA_BKG * LUMINOSITY) / N_GEN_BKG
h_sig.Scale(scale_sig)
h_bkg.Scale(scale_bkg)

# Lasketaan yhteisspektri signaali + tausta
h_sum = h_sig.Clone("h_sum")
h_sum.Add(h_bkg)

# Piirretään histogrammit
c = TCanvas("c", "Invariant Mass")
h_sig.SetLineColor(ROOT.kRed)
h_bkg.SetLineColor(ROOT.kBlue)
h_sum.SetLineColor(ROOT.kBlack)
h_sig.SetStats(0)
h_bkg.SetStats(0)
h_sum.SetStats(0)

# Otsikko ja akselit
h_sig.SetTitle("Invariant Mass Spectrum")
h_sig.GetXaxis().SetTitle("M_{#mu#mu} [GeV]")
h_sig.GetYaxis().SetTitle("Events / bin")
h_sig.GetXaxis().SetTitleSize(0.045)
h_sig.GetYaxis().SetTitleSize(0.045)

# Piirretään
h_sig.Draw("HIST")
h_bkg.Draw("HIST SAME")
h_sum.Draw("HIST SAME")
h_sig.Draw("E1 SAME")  # Tapahtumapisteet + virherajat
h_bkg.Draw("E1 SAME")

# Legend
legend = TLegend(0.6, 0.7, 0.88, 0.88)
legend.AddEntry(h_sig, "Signal (Z → #mu#mu)", "l")
legend.AddEntry(h_bkg, "Background (t#bar{t})", "l")
legend.AddEntry(h_sum, "Total", "l")
legend.Draw()

# Sovitetaan yhdistetty jakauma: eksponentiaalinen tausta + Gaussin muotoinen signaali
fit = TF1("fit", "[0]*exp(-0.01*x) + [1]*exp(-0.5*((x-[2])/[3])**2)", 60, 120)
fit.SetParameters(100, 1000, 91.2, 2.5)  # alkuarvaukset
h_sum.Fit(fit, "R")

# Määritetään massaikkuna ja lasketaan integraalit
mass_min = 80
mass_max = 100
bin_min = h_sum.GetXaxis().FindBin(mass_min)
bin_max = h_sum.GetXaxis().FindBin(mass_max)

# Sovitetun funktion integraali koko alueella
N_total = fit.Integral(mass_min, mass_max) / h_sum.GetBinWidth(1)

# Taustafunktion erillinen integraali
bkg_func = TF1("bkg", "[0]*exp(-0.01*x)", 60, 120)
bkg_func.SetParameter(0, fit.GetParameter(0))
N_bkg = bkg_func.Integral(mass_min, mass_max) / h_sum.GetBinWidth(1)

# Signaalin määrä ja signifikanssi
N_sig = N_total - N_bkg
significance = N_sig / np.sqrt(N_bkg)

# Tuloste
print(f"N_S = {N_sig:.1f}, N_B = {N_bkg:.1f}")
print(f"Significance: {significance:.2f} sigma")

# Tarvittava luminositeetti 5σ-havainnolle
required_lumi = (5 / significance) ** 2 * LUMINOSITY
print(f"Required luminosity for 5σ: {required_lumi:.6f} fb^-1")

# Tallennetaan kuva
c.SaveAs("invariant_mass.pdf")