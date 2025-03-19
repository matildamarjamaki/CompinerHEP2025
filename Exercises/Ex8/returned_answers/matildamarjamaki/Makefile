# CXX = g++
# CXXFLAGS = -O2 -std=c++17 -Wall `root-config --cflags` -I$(PYTHIA8)/include
# LDFLAGS = `root-config --glibs` -L$(PYTHIA8)/lib -lpythia8

# # Pythia8 installation path (modify if needed)
# PYTHIA8 ?= /path/to/pythia8

# # Executable names
# EXEC1 = runHiggsWidth
# EXEC2 = higgsMassPlot

# # Source files
# SRC1 = runHiggsWidth.cxx
# SRC2 = higgsMassPlot.cxx

# # Build first program
# $(EXEC1): $(SRC1)
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# # Build second program
# $(EXEC2): $(SRC2)
# 	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# # Build both
# all: $(EXEC1) $(EXEC2)

# # Clean up
# clean:
# 	rm -f $(EXEC1) $(EXEC2) *.o *.root *.png


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

%:
	$(CXX) $(OPT) $(INC) $@.cxx $(LIBS) -o $@

clean:
	rm  HiggsMass.png HiggsMass.root higgsMassPlot pythia8313.tgz runHiggsWidth

higgsMassPlot:
	$(CXX) $(OPT) $(INC) higgsMassPlot.cxx $(LIBS) -o higgsMassP
%:
	$(CXX) $(OPT) $(INC) $@.cxx $(LIBS) -o $@


.PHONY: higgsMassPlot
higgsMassPlot:
	$(CXX) $(OPT) $(INC) higgsMassPlot.cxx $(LIBS) -o higgsMassPlot
	/bin/tcsh higgsMassPlot.job

.PHONY: runHiggsWidth
runHiggsWidth:
	$(CXX) $(OPT) $(INC) runHiggsWidth.cxx $(LIBS) -o runHiggsWidth
	/bin/tcsh runHiggsWidth.job