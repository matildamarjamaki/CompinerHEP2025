
// #include <TFile.h>
// #include <TTree.h>
// #include <TRandom3.h>
// #include <iostream>

// void generateTree() {
//     // Create a ROOT file to store the tree
//     TFile *file = new TFile("random_numbers.root", "RECREATE");
    
//     // Create a TTree
//     TTree *tree = new TTree("tree", "Tree with normally distributed random numbers");
    
//     // Variable to hold generated numbers
//     double value;
    
//     // Create a branch in the tree
//     tree->Branch("value", &value, "value/D");
    
//     // Random number generator
//     TRandom3 randGen(0); // Seed = 0 (can be changed)
    
//     // Generate N = 1000 normally distributed random numbers (mean=0, sigma=1)
//     const int N = 1000;
//     for (int i = 0; i < N; ++i) {
//         value = randGen.Gaus(0, 1);
//         tree->Fill();
//     }
    
//     // Write tree to file
//     tree->Write();
//     file->Close();
    
//     std::cout << "Generated " << N << " normally distributed numbers and saved to random_numbers.root" << std::endl;
// }

#include <TTree.h>
#include <TRandom3.h>
#include <TFile.h>

void generateTree() {
    // Number of random numbers to generate
    const int N = 1000;

    // Create a ROOT file to save the tree
    TFile *file = new TFile("random_numbers.root", "RECREATE");

    // Create a TTree with a single branch to hold the random numbers
    TTree *tree = new TTree("tree", "Tree of Random Numbers");

    // Variable to hold each random number
    double random_number;

    // Create a branch to store the random numbers in the tree
    tree->Branch("random_number", &random_number);

    // Create a random number generator
    TRandom3 randGen;

    // Fill the tree with N normally distributed random numbers
    for (int i = 0; i < N; ++i) {
        random_number = randGen.Gaus(0, 1);  // Generate a normal random number with mean 0 and standard deviation 1
        tree->Fill();
    }

    // Write the tree to the ROOT file
    tree->Write();

    // Close the file
    file->Close();
}

int main() {
    generateTree();
    return 0;
}
