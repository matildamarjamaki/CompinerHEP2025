
#include <ROOT/RDataFrame.hxx>
#include <TCanvas.h>
#include <TH1D.h>
#include <TFile.h>

void analysis_rdf() {
    std::string filename = "/home/matildma/Documents/CompinerHEP2025/Exercises/Ex12/returned_answers/matildamarjamaki/DYJetsToLL.root";

    ROOT::RDataFrame df("Events", filename);

    // Filter: keep only events passing the HLT_IsoMu24 trigger
    auto df_filtered = df.Filter("HLT_IsoMu24", "Trigger HLT_IsoMu24");

    // Histogram the pileup distribution using PV_npvs as proxy
    auto h_pileup = df_filtered.Histo1D(
        {"h_pileup", "Pileup Distribution;Pileup (PV_npvs);Events", 100, 0, 100},
        "PV_npvs"
    );

    // Save histogram to file
    TFile outfile("output_rdf.root", "RECREATE");
    h_pileup->Write();
    outfile.Close();

    // Plot
    TCanvas* c = new TCanvas("c", "Pileup", 800, 600);
    h_pileup->Draw("HIST");

    c->Update();
    c->SaveAs("pileup_distribution_rdf.png");

}