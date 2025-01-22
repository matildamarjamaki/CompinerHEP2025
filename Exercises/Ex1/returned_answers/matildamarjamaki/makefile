MAIN = main

# Default target                                                                                                                                                                        
all: $(MAIN).pdf

# Rule to build the PDF                                                                                                                                                                 
$(MAIN).pdf: $(MAIN).tex
	pdflatex $(MAIN).tex
	bibtex $(MAIN)
	mpost feyngraph1.mp
	mpost feyngraph2.mp
	pdflatex $(MAIN).tex
	pdflatex $(MAIN).tex

# Clean intermediate files                                                                                                                                                              
clean:
	rm -f $(MAIN).aux $(MAIN).bbl $(MAIN).blg $(MAIN).log $(MAIN).out $(MAIN).toc $(MAIN).synctex.gz

# Full clean (including PDF)                                                                                                                                                            
cleanall: clean
	rm -f $(MAIN).pdf

.PHONY: all clean cleanall
