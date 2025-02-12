
# määritellään, mitä kääntäjää käytetään, jos CXX ei ole jo määritelty, käytetään g++:aa (C++-kääntäjä)
CXX ?= g++

# kääntäjäasetukset: 
# -Wall: näytetään kaikki varoitukset
# -Wextra: näytetään lisää varoituksia
# -O2: käytetään kääntäjän optimointitasoa 2 (kohtalainen optimointi)
CXXFLAGS = -Wall -Wextra -O2

# linkityksessä käytettävät asetukset:
# -lm: linkitetään matematiikkakirjasto (libm)
LDFLAGS = -lm  # Linkitetään matematiikkakirjasto

# pääohjelma, joka käännetään oletusarvoisesti, kun suoritetaan "make" ilman argumentteja
all: reconstructed_track

# määritellään, miten "reconstructed_track" luodaan:
# - kootaan "reconstructed_track.o" tiedostosta käyttämällä C++-kääntäjää (CXX),
# - linkitetään se matematiikkakirjaston kanssa (LDFLAGS)
reconstructed_track: reconstructed_track.o
	$(CXX) $(LDFLAGS) -o reconstructed_track reconstructed_track.o

# määritellään, miten "reconstructed_track.o" luodaan:
# - käännetään "reconstructed_track.cpp" lähdekooditiedostosta käyttäen CXXFLAGS-asetuksia
reconstructed_track.o: reconstructed_track.cpp
	$(CXX) $(CXXFLAGS) -c reconstructed_track.cpp

# poistetaan käännetyt tiedostot
clean:
	rm -f reconstructed_track reconstructed_track.o