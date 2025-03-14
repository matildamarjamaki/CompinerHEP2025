
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>

void plot_higgs_widths() {
    gStyle->SetOptStat(0);  // Ei ROOTin tilastoikkunaa

    std::ifstream file_fh("feynhiggs_widths.txt");  // FeynHiggsin leveydet
    std::ifstream file_hd("br.sm2");  // HDECAYn leveydet

    if (!file_fh.is_open() || !file_hd.is_open()) {
        std::cerr << "Error: Cannot open one or more files!" << std::endl;
        return;
    }

    std::vector<double> masses, feynWidths, hdWidths;
    std::string line;

    // Ohitetaan FeynHiggs-tiedoston otsikkorivit
    std::getline(file_fh, line);
    std::getline(file_fh, line);

    std::cout << "Luen dataa...\n";

    // Luetaan data molemmista tiedostoista
    while (std::getline(file_fh, line) && std::getline(file_hd, line)) {
        std::istringstream iss_fh(line);
        std::istringstream iss_hd(line);
        
        double mh_fh, width_fh;
        double mh_hd, width_hd;

        if (!(iss_fh >> mh_fh >> width_fh)) {
            std::cerr << "Ohitetaan virheellinen rivi FeynHiggs-tiedostosta: " << line << std::endl;
            continue;
        }

        if (!(iss_hd >> mh_hd >> width_hd)) {
            std::cerr << "Ohitetaan virheellinen rivi HDECAY-tiedostosta: " << line << std::endl;
            continue;
        }

        if (std::abs(mh_fh - mh_hd) > 1e-3) {
            std::cerr << "Massat eivät täsmää: FeynHiggs = " << mh_fh
                      << ", HDECAY = " << mh_hd << std::endl;
            continue;
        }

        masses.push_back(mh_fh);
        feynWidths.push_back(width_fh);
        hdWidths.push_back(width_hd);
    }

    file_fh.close();
    file_hd.close();

    if (masses.empty()) {
        std::cerr << "Ei kelvollisia datapisteitä. Lopetetaan.\n";
        return;
    }

    // Piirretään leveydet
    TCanvas *c1 = new TCanvas("c1", "Higgsin leveys vs massa", 800, 600);
    TGraph *graph_fh = new TGraph(masses.size(), &masses[0], &feynWidths[0]);
    TGraph *graph_hd = new TGraph(masses.size(), &masses[0], &hdWidths[0]);

    graph_fh->SetTitle("Higgsin hiukkasen leveys vs massa;Massa (GeV);Leveys (GeV)");
    graph_fh->SetMarkerStyle(20);
    graph_fh->SetMarkerSize(2);
    graph_fh->SetMarkerColor(kRed);
    graph_fh->SetLineColor(kRed);
    graph_fh->SetLineWidth(2);

    graph_hd->SetMarkerStyle(21);
    graph_hd->SetMarkerSize(2);
    graph_hd->SetMarkerColor(kBlue);
    graph_hd->SetLineColor(kBlue);
    graph_hd->SetLineWidth(2);

    graph_fh->Draw("APL");
    graph_hd->Draw("P SAME");

    TLegend *legend = new TLegend(0.7, 0.7, 0.9, 0.9);
    legend->AddEntry(graph_fh, "FeynHiggs", "lp");
    legend->AddEntry(graph_hd, "HDECAY", "p");
    legend->Draw();

    gPad->Update();
    c1->SaveAs("higgs_widths_comparison.png");

    // Piirretään suhteiden kuvaaja
    TCanvas *c2 = new TCanvas("c2", "Suhde FeynHiggs/HDECAY", 800, 600);
    std::vector<double> ratios;
    for (size_t i = 0; i < masses.size(); ++i) {
        if (hdWidths[i] != 0) {
            ratios.push_back(feynWidths[i] / hdWidths[i]);
        } else {
            std::cerr << "Varoitus: HDECAYn leveys on nolla M=" << masses[i] << std::endl;
            ratios.push_back(0);
        }
    }

    TGraph *graph_ratio = new TGraph(masses.size(), &masses[0], &ratios[0]);
    graph_ratio->SetTitle("Suhde FeynHiggs/HDECAY;Massa (GeV);Suhde");
    graph_ratio->SetMarkerStyle(20);
    graph_ratio->SetMarkerSize(2);
    graph_ratio->Draw("AP");

    gPad->Update();
    c2->SaveAs("width_ratio.png");

    std::cout << "Piirto valmis. Kuvat tallennettu.\n";
}