
# kääntäjä C++ koodin kääntämiseen
CXX = g++

# kääntäjälle tarvittavat asetukset ROOT:n kääntämiseen
CXXFLAGS = $(shell root-config --cflags)

# asetukset ROOT:n kirjastojen linkittämiseen
LDFLAGS = $(shell root-config --libs)

# määrittää tiedostojen nimet, jotka käännetään lähdekoodista
EXEC1 = generateTree
EXEC2 = plotHistogram

# määrittää tiedostojen nimet, joita käytetään binääritiedostojen luomiseen
# (ROOT-tiedostot aina binääritiedostoja)
SRC1 = generateTree.C
SRC2 = plotHistogram.C

# määrittää, että molemmat tiedostot käännetään
all: $(EXEC1) $(EXEC2)

# määrittää kuinka tiedostot käännetään
$(EXEC1): $(SRC1)
	$(CXX) $(CXXFLAGS) $(SRC1) -o $(EXEC1) $(LDFLAGS)

$(EXEC2): $(SRC2)
	$(CXX) $(CXXFLAGS) $(SRC2) -o $(EXEC2) $(LDFLAGS)

# suorittaa käännetyt ohjelmat
run: $(EXEC1) $(EXEC2)
	./$(EXEC1)
	./$(EXEC2)

# poistaa kaikki käännetyt ja väliaikaiset tiedostot
clean:
	rm -f $(EXEC1) $(EXEC2) histogram.png random_numbers.root

# erottaa tiedostojen nimet säännöistä (eli yllä olevista ohjeista miten tiedosto käännetään)
# estää virheitä, mikäli sääntöjen nimet ovat tiedostojen nimiä
.PHONY: all run clean
