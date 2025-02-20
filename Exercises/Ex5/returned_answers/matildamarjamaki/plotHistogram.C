
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TF1.h>
#include <iostream>

void plotHistogram() {
    // Open the ROOT file
    TFile *file = new TFile("random_numbers.root", "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file random_numbers.root" << std::endl;
        return;
    }
    
    // Retrieve the tree
    TTree *tree;
    file->GetObject("tree", tree);
    if (!tree) {
        std::cerr << "Error: Cannot find TTree in file" << std::endl;
        return;
    }
    
    // Define a histogram
    TH1D *hist = new TH1D("hist", "Histogram of Random Numbers;Value;Frequency", 50, -4, 4);
    
    // Fill the histogram from the tree
    tree->Draw("random_number >> hist");
    
    // Customize histogram appearance
    hist->SetLineColor(kBlack);
    hist->SetLineWidth(3);
    hist->SetFillColor(kYellow);
    
    // Create a canvas
    TCanvas *canvas = new TCanvas("canvas", "Histogram", 800, 600);
    canvas->SetFillColor(kWhite);
    
    // Draw the histogram
    hist->Draw();
    
    // Fit the histogram with a Gaussian
    TF1 *fitFunc = new TF1("fitFunc", "gaus", -4, 4);
    hist->Fit(fitFunc, "R");
    
    // Save the plot as an image
    canvas->SaveAs("histogram.png");
    
    // Cleanup
    file->Close();
    delete file;
}

int main() {
    plotHistogram();
    return 0;
}