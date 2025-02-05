
#include <iostream> // I/O-kirjaston tuominen syötteen lukemista ja tulostamista varten
#include <cmath> // Matemaattisten funktioiden tuominen
#include "MET.h" // MET-luokan sisällyttäminen, joka mahdollistaa MET-olioiden käytön

int main() {
    double x, y; // Määritellään kaksi muuttujaa (x ja y), jotka tallentavat käyttäjän syötteet

    // Pyydetään käyttäjältä syötteet MET_x ja MET_y
    std::cout << "Anna MET_x: "; // Tulostetaan ohje MET_x:n syöttämiseksi
    std::cin >> x; // Otetaan vastaan käyttäjän syöte ja tallennetaan se muuttujaan x
    std::cout << "Anna MET_y: "; // Tulostetaan ohje MET_y:n syöttämiseksi
    std::cin >> y; // Otetaan vastaan käyttäjän syöte ja tallennetaan se muuttujaan y

    // Luodaan MET-olio käyttäjän syötteillä
    MET met(x, y);

    // Tulostetaan arvot
    std::cout << "\nTulokset:" << std::endl;
    std::cout << "MET: " << met.getMET() << std::endl;
    std::cout << "MET_x: " << met.getMETx() << std::endl;
    std::cout << "MET_y: " << met.getMETy() << std::endl;
    std::cout << "MET_phi: " << met.getMETphi() << std::endl;

    return 0; // Palautetaan arvo 0, joka merkitsee ohjelman onnistunutta suorittamista
}