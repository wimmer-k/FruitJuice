.EXPORT_ALL_VARIABLES:

.PHONY: clean all

BIN_DIR = $(HOME)/bin
LIB_DIR = $(HOME)/lib
COMMON_DIR = $(HOME)/common/


ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)
ROOTINC      := -I$(shell root-config --incdir)

CPP             = g++
CFLAGS		= -pedantic -Wall -Wno-long-long -g -O3 $(ROOTCFLAGS) -fPIC

INCLUDES        = -I./inc -I$(COMMON_DIR)
BASELIBS 	= -lm $(ROOTLIBS) $(ROOTGLIBS) -L$(LIB_DIR) -lSpectrum
ALLIBS  	=  $(BASELIBS) -lCommandLineInterface -lGrape
LIBS 		= $(ALLIBS)
LFLAGS		= -g -fPIC

#O_FILES = build/Grape.o
LIB_O_FILES = build/Grape.o build/GrapeDictionary.o 

Fruit:	Fruit.cc $(LIB_DIR)/libGrape.so 
	@echo "Compiling $@"
	@$(CPP) $(CFLAGS) $(INCLUDES) $< $(LIBS) -o $(BIN_DIR)/$@ 

$(LIB_DIR)/libGrape.so: $(LIB_O_FILES) 
	@echo "Making $@"
	@$(CPP) -g -fPIC -shared -o $@ $^ -lc

build/%.o: src/%.cc inc/%.hh
	@echo "Compiling $@"
	@mkdir -p $(dir $@)
	@$(CPP) $(CFLAGS) $(INCLUDES) -c $< -o $@ 

build/GrapeDictionary.o: build/GrapeDictionary.cc
	@echo "Compiling $@"
	@mkdir -p $(dir $@)
	@$(CPP) $(INCLUDES) -fPIC -c $< -o $@ $(CFLAGS)

build/GrapeDictionary.cc: inc/Grape.hh inc/GrapeLinkDef.h
	@echo "Building $@"
	@mkdir -p build
	@rootcint -f $@ -c $(INCLUDES) $(ROOTCFLAGS) $(notdir $^)

clean:
	@echo "Cleaning up"
	@rm -rf build
