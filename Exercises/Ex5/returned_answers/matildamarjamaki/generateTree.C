
#include <TTree.h> // antaa työkaluja ROOT:n TTree-tietorakenteen käsittelyyn
#include <TRandom3.h> // ROOT:n satunnaislukugeneraattori
#include <TFile.h> // mahdollistaa ROOT-tiedostojen käsittelyä (.root)

void generateTree() {
    const int N = 1000; // määrittää, kuinka monta satunnaislukua halutaan generoida (nyt 1000)

    // luodaan ROOT-tiedosto, johon tietorakenne tallennetaan
    // RECREATE --> tiedosto luodaan uudelleen, jos se on jo olemassa
    TFile *file = new TFile("random_numbers.root", "RECREATE");

    // luodaan ROOT-tietorakenne/TTree, johon tallennetaan satunnaisluvut
    TTree *tree = new TTree("tree", "Tree of Random Numbers");

    // muuttuja satunnaiselle luvulle
    double random_number;

    // lisätään branch tietorakenteeseen, jossa tallennetaan satunnaisnumero
    // branch = "random_number"
    // viite muuttujaan, johon data tallennetaan on &random_number
    tree->Branch("random_number", &random_number);

    // satunnaislukugeneraattorin luonti (TRandom3)
    // (käyttää Mersenne Twister -algoritmia satunnaislukujen tuottamiseen)
    TRandom3 randGen;

    // loop, joka käy läpi N satunnaislukua ja täyttää ne tietorakenteeseen
    for (int i = 0; i < N; ++i) {
        // tuottaa normaalijakautuneen satunnaisluvun (keskiarvo 0, hajonta 1)
        random_number = randGen.Gaus(0, 1);
        // täytetään tietorakenne uudella satunnaisluvulla
        tree->Fill();
    }

    // vie tietorakenteen sisällön tiedostoon
    tree->Write();

    // tiedoston sulkeminen, jotta tiedot tallentuvat ja tiedosto voidaan avata myöhemmin uudelleen
    file->Close();
}

// int main() {
//     generateTree();
//     return 0;
// }
