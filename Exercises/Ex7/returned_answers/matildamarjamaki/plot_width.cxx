
#include <iostream>
#include <fstream>
#include <vector>
#include <TGraph.h>
#include <TCanvas.h>

void plot_width() {
    std::ifstream file("br.sm2");  // Muuta tarvittaessa tiedoston nimi
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file!" << std::endl;
        return;
    }

    std::vector<double> masses, widths;
    std::string line;
    
    // Ohitetaan ensimmäiset kaksi riviä (otsikot ja viiva)
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        double mh, gg, gamgam, zgam, ww, zz, width;
        if (!(iss >> mh >> gg >> gamgam >> zgam >> ww >> zz >> width)) {
            std::cerr << "Skipping invalid line: " << line << std::endl;
            continue;
        }
        masses.push_back(mh);
        widths.push_back(width);
    }

    file.close();

    // Piirretään kuvaaja
    TCanvas *c1 = new TCanvas("c1", "Higgsin leveys vs massa", 800, 600);
    TGraph *graph = new TGraph(masses.size(), &masses[0], &widths[0]);

    graph->SetTitle("Higgsin hiukkasen leveys vs massa;Massa (GeV);Leveys (GeV)");
    graph->SetMarkerStyle(20);
    graph->SetMarkerSize(1);
    graph->Draw("AP");

    c1->SaveAs("higgs_widths.png");  // Tallennetaan kuva
}