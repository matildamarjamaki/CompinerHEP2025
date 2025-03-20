
PYTHIA8_URL = https://pythia.org/download/pythia83/pythia8313.tgz
PYTHIA8_TGZ = $(notdir $(PYTHIA8_URL))
PYTHIA8_DIR = $(basename $(PYTHIA8_TGZ))

OPT = -g -Wall `root-config --cflags`
INC = -I$(PYTHIA8_DIR)/include
LIBS = -L$(PYTHIA8_DIR)/lib -lpythia8 -ldl `root-config --libs`


$(PYTHIA8_DIR):
	wget $(PYTHIA8_URL)
	tar xfvz $(PYTHIA8_TGZ)
	cd $(PYTHIA8_DIR) && ./configure && make

all:
	$(MAKE) $(PYTHIA8_DIR)
	$(CXX) $(OPT) $(INC) detect_muons.cxx $(LIBS) -o detect_muons
	$(CXX) $(OPT) $(INC) events.cxx $(LIBS) -o events

clean:
	rm events detect_muons muon_distributions.txt pythia8313.tgz muon_pT_distribution.png muon_eta_distribution.png