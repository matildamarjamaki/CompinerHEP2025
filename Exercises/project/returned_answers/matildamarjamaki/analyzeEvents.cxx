
void analyzeEvents() {
    // Avaa ROOT-tiedosto ja lataa puu
    TFile *file = new TFile("events.root", "READ");
    if (!file || file->IsZombie()) {
        cout << "Error: cannot open events.root!" << endl;
        return;
    }
    
    TTree *tree = (TTree*)file->Get("EventTree");
    if (!tree) {
        cout << "Error: Tree EventTree not found!" << endl;
        return;
    }

    // Muuttujat datan tallennusta varten
    vector<float> *muon_pt = 0;
    vector<float> *muon_eta = 0;
    bool passTrigger = false;

    // Linkitetään muuttujat puun haaroihin
    tree->SetBranchAddress("muon_pt", &muon_pt);
    tree->SetBranchAddress("muon_eta", &muon_eta);
    tree->SetBranchAddress("passTrigger", &passTrigger);

    // Histogrammien määrittely
    TH1F *h_muon_pt = new TH1F("h_muon_pt", "Muon pT; pT (GeV); Events", 50, 0, 100);
    TH1F *h_muon_eta = new TH1F("h_muon_eta", "Muon eta; #eta; Events", 50, -2.5, 2.5);
    TH1F *h_passTrigger = new TH1F("h_passTrigger", "Trigger Efficiency; Passed; Events", 2, 0, 2);

    int nEntries = tree->GetEntries();
    int passedEvents = 0;

    // Käydään läpi kaikki tapahtumat
    for (int i = 0; i < nEntries; i++) {
        tree->GetEntry(i);

        if (!muon_pt->empty()) {
            h_muon_pt->Fill(muon_pt->at(0));  // Tallennetaan ensimmäisen myonin pT
        }
        if (!muon_eta->empty()) {
            h_muon_eta->Fill(muon_eta->at(0));  // Tallennetaan ensimmäisen myonin eta
        }

        if (passTrigger) {
            h_passTrigger->Fill(1);
            passedEvents++;
        } else {
            h_passTrigger->Fill(0);
        }
    }

    // Piirretään histogrammit
    TCanvas *c1 = new TCanvas("c1", "Muon Analysis", 1200, 400);
    c1->Divide(3, 1);

    c1->cd(1);
    h_muon_pt->Draw();

    c1->cd(2);
    h_muon_eta->Draw();

    c1->cd(3);
    h_passTrigger->Draw();

    cout << "Trigger efficiency: " << (double)passedEvents / nEntries * 100 << " %" << endl;
}