
#ifndef MISSINGET_H // Tarkistetaan, onko "MISSINGET_H" jo määritelty. Jos ei ole, määritellään se
#define MISSINGET_H // Määritellään "MISSINGET_H", jotta tämä tiedosto ei tule sisällytettyä useammin kuin kerran

#include <cmath> // Sisällytetään matematiikkakirjasto, joka mahdollistaa matemaattisten funktioiden käytön

// Luokan MET määrittely
class MET {
private: // Määrittää, että muuttujat ja metodit ovat vain luokan sisäisesti käytettävissä
    double met_x; // x-komponentti MET:stä
    double met_y; // y-komponentti MET:stä

public: // Muuttujat ja metodit ovat käytettävissä luokan ulkopuolelta
    // Konstruktorin määrittely, joka ottaa kaksi parametria x ja y ja alustaa met_x ja met_y
    MET(double x, double y) : met_x(x), met_y(y) {}

    // Funktio, joka laskee MET-arvon (kokonaisenergia)
    double getMET() const {
        // MET-arvo lasketaan Pythagoraan lauseella
        return std::sqrt(met_x * met_x + met_y * met_y);
    }

    // x-komponentin palauttava funktio
    double getMETx() const { return met_x; }

    // y-komponentin palauttava funktio
    double getMETy() const { return met_y; }

    // MET-kulman (phi) laskeminen
    double getMETphi() const {
        return std::atan2(met_y, met_x);
    }
};

// Suojataan tiedoston käyttö uudelleenmäärittelyä vastaan
#endif