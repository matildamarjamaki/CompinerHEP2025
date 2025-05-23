# Makefile is a part of the PYTHIA event generator.
# Copyright (C) 2025 Torbjorn Sjostrand.
# PYTHIA is licenced under the GNU GPL v2 or later, see COPYING for details.
# Please respect the MCnet Guidelines, see GUIDELINES for details.
# Author: Philip Ilten, October 2014 - November 2017.
#
# This is is the Makefile used to build PYTHIA on POSIX systems.
# Example usage is:
#     make -j2
# For help using the make command please consult the local system documentation,
# i.e. "man make" or "make --help".

################################################################################
# VARIABLES: Definition of the relevant variables from the configuration script
# and the distribution structure.
################################################################################

# Set the shell.
SHELL=/usr/bin/env bash

# Include the configuration and set the local directory structure.
ifeq (,$(findstring clean, $(MAKECMDGOALS)))
  ifneq (Makefile.inc,$(MAKECMDGOALS))
    LOCAL_CFG:=$(shell $(MAKE) Makefile.inc))
    include Makefile.inc
  endif
endif
LOCAL_BIN=bin/
LOCAL_DOCS=AUTHORS COPYING GUIDELINES README ../../examples/Makefile.inc
LOCAL_EXAMPLE=examples
LOCAL_INCLUDE=include
LOCAL_LIB=lib
LOCAL_SHARE=share/Pythia8
LOCAL_SRC=src
LOCAL_TMP=tmp
LOCAL_MKDIRS:=$(shell mkdir -p $(LOCAL_TMP) $(LOCAL_LIB))
CXX_COMMON:=-I$(LOCAL_INCLUDE) $(CXX_COMMON)
OBJ_COMMON:=-MD $(CXX_COMMON) $(OBJ_COMMON)
LIB_COMMON=-Wl,-rpath,../lib:$(PREFIX_LIB) -ldl $(GZIP_LIB)

# PYTHIA.
OBJECTS=$(patsubst $(LOCAL_SRC)/%.cc,$(LOCAL_TMP)/%.o,\
	$(sort $(wildcard $(LOCAL_SRC)/*.cc)))
TARGETS=$(LOCAL_LIB)/libpythia8.a $(LOCAL_LIB)/libpythia8$(LIB_SUFFIX)

# LHAPDF.
ifeq ($(LHAPDF5_USE),true)
  TARGETS+=$(LOCAL_LIB)/libpythia8lhapdf5.so
endif
ifeq ($(LHAPDF6_USE),true)
  TARGETS+=$(LOCAL_LIB)/libpythia8lhapdf6.so
endif

# POWHEG (needs directory that contains just POWHEG libraries).
ifeq ($(POWHEG_USE),true)
  TARGETS+=$(LOCAL_LIB)/libpythia8powhegHooks.so
  POWHEG_DIR=$(subst -L,,$(filter -L%,$(POWHEG_LIB)))/
  ifneq ($(POWHEG_DIR),.)
    TARGETS+=$(patsubst $(POWHEG_DIR)lib%.so,\
	     $(LOCAL_LIB)/libpythia8powheg%.so,$(wildcard $(POWHEG_DIR)*))
  endif
endif

# MG5 matrix element plugins.
ifeq ($(MG5MES_USE),true)
  TARGETS+=mg5mes
endif

# Python.
ifeq ($(PYTHON_USE),true)
  TARGETS+=python
endif

################################################################################
# RULES: Definition of the rules used to build PYTHIA.
################################################################################

# Rules without physical targets (secondary expansion for documentation).
.SECONDEXPANSION:
.PHONY: all install clean distclean mg5mes python

# All targets.
all: $(TARGETS) $(addprefix $(LOCAL_SHARE)/, $(LOCAL_DOCS))

# The documentation.
$(addprefix $(LOCAL_SHARE)/, $(LOCAL_DOCS)): $$(notdir $$@)
	cp $^ $@

# The Makefile configuration.
Makefile.inc:
	./configure

# Auto-generated (with -MD flag) dependencies.
-include $(LOCAL_TMP)/*.d $(LOCAL_TMP)/mg5mes/*.d

# PYTHIA.
$(LOCAL_TMP)/Pythia.o: $(LOCAL_SRC)/Pythia.cc Makefile.inc
	$(CXX) $< -o $@ -c $(OBJ_COMMON) -DXMLDIR=\"$(PREFIX_SHARE)/xmldoc\"
$(LOCAL_TMP)/FJcore.o: $(LOCAL_SRC)/FJcore.cc
	$(CXX) $< -o $@ -c $(OBJ_COMMON) -DFJCORE_HAVE_LIMITED_THREAD_SAFETY
$(LOCAL_TMP)/Streams.o: $(LOCAL_SRC)/Streams.cc Makefile.inc
	$(CXX) $< -o $@ -c $(OBJ_COMMON)
$(LOCAL_TMP)/%.o: $(LOCAL_SRC)/%.cc
	$(CXX) $< -o $@ -c $(OBJ_COMMON)
$(LOCAL_LIB)/libpythia8.a: $(OBJECTS)
	ar cr $@ $^
$(LOCAL_LIB)/libpythia8$(LIB_SUFFIX): $(OBJECTS)
	$(CXX) $^ -o $@ $(CXX_COMMON) $(CXX_SHARED) $(CXX_SONAME)$(notdir $@)\
	  $(LIB_COMMON) $(CXX_DTAGS)

# LHAPDF (turn off all warnings for readability).
$(LOCAL_TMP)/LHAPDF%Plugin.o: $(LOCAL_INCLUDE)/Pythia8Plugins/LHAPDF%.h
	$(CXX) -x c++ $< -o $@ -c -MD -w $(CXX_COMMON) $(LHAPDF$*_INCLUDE)
$(LOCAL_LIB)/libpythia8lhapdf%.so: $(LOCAL_TMP)/LHAPDF%Plugin.o\
	$(LOCAL_LIB)/libpythia8$(LIB_SUFFIX)
	$(CXX) $< -o $@ $(CXX_COMMON) $(CXX_SHARED) $(CXX_SONAME)$(notdir $@)\
	 $(LHAPDF$*_LIB) -lLHAPDF -Llib -lpythia8

# POWHEG.
$(LOCAL_TMP)/LHAPowheg.o: $(LOCAL_INCLUDE)/Pythia8Plugins/LHAPowheg.h
	$(CXX) -x c++ $< -o $@ -c -MD -w $(CXX_COMMON)
$(LOCAL_TMP)/PowhegHooks.o: $(LOCAL_INCLUDE)/Pythia8Plugins/PowhegHooks.h
	$(CXX) -x c++ $< -o $@ -c -MD -w $(CXX_COMMON)
$(LOCAL_LIB)/libpythia8powheg%.so: $(POWHEG_DIR)lib%.so\
	$(LOCAL_TMP)/LHAPowheg.o $(LOCAL_LIB)/libpythia8$(LIB_SUFFIX)
	$(CXX) $(LOCAL_TMP)/LHAPowheg.o -o $@ $(CXX_COMMON) $(CXX_SHARED)\
	 $(CXX_SONAME)$(notdir $@) -Llib -lpythia8\
	 -Wl,-rpath,../lib:$(POWHEG_DIR) -L$(POWHEG_DIR) -l$*
$(LOCAL_LIB)/libpythia8powhegHooks.so: $(LOCAL_TMP)/PowhegHooks.o\
	$(LOCAL_LIB)/libpythia8$(LIB_SUFFIX)
	$(CXX) $< -o $@ $(CXX_COMMON) $(CXX_SHARED) $(CXX_SONAME)$(notdir $@)\
	 -Llib -lpythia8

# MG5 matrix element plugins.
mg5mes:
	cd $(MG5MES_BIN) && $(MAKE)

# Python.
python: $(LOCAL_LIB)/libpythia8$(LIB_SUFFIX)
	cd plugins/python && $(MAKE)

# Install.
install: all
	mkdir -p $(PREFIX_BIN) $(PREFIX_INCLUDE) $(PREFIX_LIB) $(PREFIX_SHARE)
	rsync -a $(LOCAL_BIN)/* $(PREFIX_BIN)
	rsync -a $(LOCAL_INCLUDE)/* $(PREFIX_INCLUDE)
	rsync -a $(LOCAL_LIB)/* $(PREFIX_LIB)
	rsync -a $(LOCAL_SHARE)/* $(PREFIX_SHARE)
	rsync -a $(LOCAL_EXAMPLE) $(PREFIX_SHARE)

# Clean.
clean:
	cd plugins/python && $(MAKE) clean
	cd plugins/mg5mes && $(MAKE) clean
	rm -rf $(LOCAL_TMP) $(LOCAL_LIB)
	rm -f $(LOCAL_EXAMPLE)/*Dct.*
	rm -f $(LOCAL_EXAMPLE)/*[0-9][0-9]
	rm -f $(LOCAL_EXAMPLE)/weakbosons.lhe
	rm -f $(LOCAL_EXAMPLE)/hist.root

# Clean all temporary and generated files.
distclean: clean
	cd examples && make clean
	find . -type f -name Makefile.inc -print0 | xargs -0 rm -f
	find . -type f -name "*~" -print0 | xargs -0 rm -f
	find . -type f -name "#*" -print0 | xargs -0 rm -f
	rm -rf $(LOCAL_BIN)
	rm -f $(LOCAL_SHARE)/AUTHORS
	rm -f $(LOCAL_SHARE)/COPYING
	rm -f $(LOCAL_SHARE)/GUIDELINES
	rm -f $(LOCAL_SHARE)/README
