
#include <iostream>
#include <fstream>
#include <vector>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <sstream>

void plot_widths_comparison() {
    std::vector<double> masses;
    std::vector<double> feynHiggsWidths;

    std::ifstream feynHiggsFile("outputFile");
    std::string line;
    while (std::getline(feynHiggsFile, line)) {
        std::stringstream ss(line);
        double mass, width;
        char comma;
        ss >> mass >> comma >> width;
        masses.push_back(mass);
        feynHiggsWidths.push_back(width);
    }

    std::vector<double> hDecayWidths;
    std::ifstream hDecayFile("br.sm2");
    std::string hDecayLine;
    while (std::getline(hDecayFile, hDecayLine)) {
        if (hDecayLine.empty() || !isdigit(hDecayLine[1])) continue;
        std::stringstream ss(hDecayLine);
        double mass, GG, GAM, ZGAM, WW, ZZ, width;
        ss >> mass >> GG >> GAM >> ZGAM >> WW >> ZZ >> width;
        hDecayWidths.push_back(width);
    }

    TGraph* graphFeynHiggs = new TGraph(masses.size(), &masses[0], &feynHiggsWidths[0]);
    TGraph* graphHDecay = new TGraph(masses.size(), &masses[0], &hDecayWidths[0]);

    TCanvas* canvas = new TCanvas("canvas", "Higgs Width Comparison", 800, 600);
    canvas->Divide(1, 2);
    
    canvas->cd(1);
    graphFeynHiggs->SetLineColor(kRed);
    graphHDecay->SetLineColor(kBlue);
    
    graphFeynHiggs->SetTitle("Higgs Widths;Mass (GeV);Width");
    graphFeynHiggs->GetXaxis()->SetLimits(110, 155);
    graphFeynHiggs->GetYaxis()->SetRangeUser(0.001, 0.0045);
    
    graphFeynHiggs->Draw("AL");
    graphHDecay->Draw("L SAME");
    
    TLegend* legend = new TLegend(0.1, 0.7, 0.3, 0.9);
    legend->AddEntry(graphFeynHiggs, "FeynHiggs Width", "l");
    legend->AddEntry(graphHDecay, "HDECAY Width", "l");
    legend->Draw();
    
    canvas->cd(2);
    std::vector<double> ratioWidths;
    for (size_t i = 0; i < feynHiggsWidths.size(); ++i) {
        ratioWidths.push_back(feynHiggsWidths[i] / hDecayWidths[i]);
    }

    TGraph* graphRatio = new TGraph(masses.size(), &masses[0], &ratioWidths[0]);
    graphRatio->SetTitle("FeynHiggs/HDECAY Ratio;Mass (GeV);Ratio");
    graphRatio->GetXaxis()->SetLimits(105,135);
    graphRatio->GetYaxis()->SetRangeUser(0.9, 1);
    graphRatio->SetMarkerStyle(20);
    graphRatio->SetMarkerColor(kBlack);
    graphRatio->Draw("AP");
    
    canvas->Update();
}
