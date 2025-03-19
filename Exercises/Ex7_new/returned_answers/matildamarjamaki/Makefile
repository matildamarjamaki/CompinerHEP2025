
# Compiler and flags
CXX = g++
CXXFLAGS = -O3 -g -fomit-frame-pointer -ffast-math -Wall -m64

# FeynHiggs paths
FEYNHIGGS_PATH = /home/matildma/Documents/Programs/FeynHiggs-2.19.0
INCLUDE_DIR = $(FEYNHIGGS_PATH)/x86_64-Linux/include
LIB_DIR = $(FEYNHIGGS_PATH)/build

# Libraries
LIBS = -L$(LIB_DIR) -lFH -lgfortran

# Source and executable
SRC = feynhiggs_widths.cc
EXEC = feynhiggs_widths

# Build
all:
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(SRC) $(LIBS) -o $(EXEC)

# Clean
clean:
	rm -f $(EXEC) *.o *.png outputFile mass_width.txt
