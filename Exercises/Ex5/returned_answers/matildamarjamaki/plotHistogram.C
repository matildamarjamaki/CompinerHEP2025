
#include <TFile.h> // mahdollistaa ROOT-tiedostojen käsittelyä (.root)
#include <TTree.h> // antaa työkaluja ROOT:n TTree-tietorakenteen käsittelyyn
#include <TH1D.h> // ROOT:n histogrammiobjekti (käytetään yhden muuttujan histogrammien käsittelyyn)
#include <TCanvas.h> // "piirtoalusta"
#include <TF1.h> // ROOT:n funktionsovitus
#include <iostream> // tulostusten ja virheilmoitusten käsittelyyn

void plotHistogram() {
    // avataan tallennettua dataa sisältävä ROOT-tiedosto
    TFile *file = new TFile("random_numbers.root", "READ");
    // tarkistetaan onniAstuiko avaaminen
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file random_numbers.root" << std::endl;
        return;
    }

    // haetaan TTree-olio tiedostosta
    TTree *tree;
    file->GetObject("tree", tree);
    // tarkistetaan löytyikö TTree-olio
    if (!tree) {
        std::cerr << "Error: Cannot find TTree in file" << std::endl;
        return;
    }
    
    // luodaan histogrammi 50 binillä välille [-4,4]
    TH1D *hist = new TH1D("hist", "Histogram of Random Numbers;Value;Frequency", 50, -4, 4);
    
    // hakee tietoja TTree:sta ja tallentaa ne histogrammiin
    tree->Draw("random_number >> hist");
    
    // histogrammin ulkoasu
    hist->SetLineColor(kBlack); // musta viiva
    hist->SetLineWidth(3); // viivan leveys
    hist->SetFillColor(kYellow); // keltainen täyttöväri
    
    // luodaan "piirtoalusta" histogrammille
    TCanvas *canvas = new TCanvas("canvas", "Histogram", 800, 600);
    canvas->SetFillColor(kWhite); // valkoinen taustaväri
    
    // lopullinen printti histogrammille
    hist->Draw();
    
    // luodaan Gaussin jakauma, joka sovitetaan histogrammiin välille [-4,4]
    TF1 *fitFunc = new TF1("fitFunc", "gaus", -4, 4);

    // sovitetaan funktio histogrammiin
    hist->Fit(fitFunc, "R");
    
    // tallennetaan kuva histogrammista tiedostoksi
    canvas->SaveAs("histogram.png");
    
    // suljetaan tiedosto ja vapautetaan muisti
    file->Close();
    delete file;
}

// int main() {
//     plotHistogram();
//     return 0;
// }