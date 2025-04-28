
# PYTHIA8_URL = https://pythia.org/download/pythia83/pythia8313.tgz
# PYTHIA8_TGZ = $(notdir $(PYTHIA8_URL))
# PYTHIA8_DIR = $(basename $(PYTHIA8_TGZ))

# OPT = -g -Wall `root-config --cflags`
# INC = -I$(PYTHIA8_DIR)/include
# LIBS = -L$(PYTHIA8_DIR)/lib -lpythia8 -ldl `root-config --libs`


# $(PYTHIA8_DIR):
# 	wget $(PYTHIA8_URL)
# 	tar xfvz $(PYTHIA8_TGZ)
# 	cd $(PYTHIA8_DIR) && ./configure && make

# all:
# 	$(MAKE) $(PYTHIA8_DIR)

# %:
# 	cp $(PYTHIA8_DIR)/examples/$@.cc .
# 	$(CXX) $(OPT) $(INC) $@.cc $(LIBS) -o $@

# files = $(wildcard *~ main* pythia* *.exe)
# clean:
# 	rm -rf $(files)
# 	rm -f *.png

# cleanAll:
# 	$(MAKE) clean

# # test:
# # 	$(CXX) $(OPT) $(INC) $@.cc $(LIBS) -o test.exe

# signal:
# 	$(CXX) $(OPT) $(INC) signal.cc $(LIBS) -o signal.exe

# background:
# 	$(CXX) $(OPT) $(INC) background.cc $(LIBS) -o background.exe

PYTHIA8_URL = https://pythia.org/download/pythia83/pythia8313.tgz
PYTHIA8_TGZ = $(notdir $(PYTHIA8_URL))
PYTHIA8_DIR = $(basename $(PYTHIA8_TGZ))

OPT  = -g -Wall `root-config --cflags`
INC  = -I$(PYTHIA8_DIR)/include
LIBS = -L$(PYTHIA8_DIR)/lib -lpythia8 -ldl `root-config --libs`

EXES = signal.exe background.exe

# Default: build Pythia and executables
all: $(PYTHIA8_DIR) $(EXES)

$(PYTHIA8_DIR):
	wget $(PYTHIA8_URL)
	tar xfvz $(PYTHIA8_TGZ)
	cd $(PYTHIA8_DIR) && ./configure && make

signal.exe: signal.cc
	$(CXX) $(OPT) $(INC) signal.cc $(LIBS) -o signal.exe

background.exe: background.cc
	$(CXX) $(OPT) $(INC) background.cc $(LIBS) -o background.exe

# Run both simulations and analysis
run: all
	@echo "\033[1;34mRunning signal simulation...\033[0m"
	LD_LIBRARY_PATH=$(PYTHIA8_DIR)/lib ./signal.exe > signal.out
	@echo "\033[1;32mSignal complete. Output in signal.out\033[0m"

	@echo "\033[1;34mRunning background simulation...\033[0m"
	LD_LIBRARY_PATH=$(PYTHIA8_DIR)/lib ./background.exe > background.out
	@echo "\033[1;32mBackground complete. Output in background.out\033[0m"

	@echo "\033[1;34mRunning Python analysis...\033[0m"
	python3 analyze.py
	@echo "\033[1;32mAnalysis complete.\033[0m"

# Clean only generated files
clean:
	rm -f *.exe *.png signal.out background.out

# Clean everything including Pythia
cleanAll:
	$(MAKE) clean
	rm -rf $(PYTHIA8_DIR) $(PYTHIA8_TGZ)
