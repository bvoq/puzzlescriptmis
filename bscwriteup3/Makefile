
MAIN=main
TEX_FILES=$(wildcard ./*.tex)
BIB_FILES=$(wildcard ./*.bib)
BIB_FILES=$(wildcard ./*.bib)
BUILD_BIB_FILES=$(addprefix build/,$(notdir $(BIB_FILES))) 

TFLAGS=-output-directory=build

.PHONY: all
all: $(MAIN)

.PHONY:
$(MAIN): build $(MAIN).pdf
	open $(MAIN).pdf

build:
	mkdir -p build

$(MAIN).pdf: $(BUILD_BIB_FILES) $(TEX_FILES) $(BIB_FILES)
	pdflatex $(TFLAGS) -draftmode $(MAIN)
	cd build; export "BSTINPUTS=$$BSTINPUTS:.."; bibtex $(MAIN);
	pdflatex $(TFLAGS) -draftmode $(MAIN)
	pdflatex $(TFLAGS) -draftmode $(MAIN)
	pdflatex $(TFLAGS) $(MAIN)
	mv build/$(MAIN).pdf $(MAIN).pdf

# Bibtex doesn't understand build directories, so link each file
build/%.bib: %.bib
	cp $< $@

.PHONY: clean
clean:
	rm -rf build $(MAIN).pdf
