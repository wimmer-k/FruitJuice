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
ALLIBS  	=  $(BASELIBS) -lCommandLineInterface 
LIBS 		= $(ALLIBS)
LFLAGS		= -g -fPIC


#Fruit:	build/%.o
Fruit:	Fruit.cc
	@echo "Compiling $@"
	@$(CPP) $(CFLAGS) $(INCLUDES) $< $(LIBS) -o $(BIN_DIR)/$@ 

build/%.o: %.cc
	@echo "Compiling $@"
	@mkdir -p $(dir $@)
	@$(CPP) -fPIC -c $< -o $@ $(CFLAGS)

#build/Dictionary.o: build/Dictionary.cc
#	@echo "Compiling $@"
#	@mkdir -p $(dir $@)
#	@$(CPP) -fPIC -c $< -o $@ $(CFLAGS)
#
#build/Dictionary.cc: $(wildcard include/*.hh) include/LinkDef.h
#	@echo "Building $@"
#	@mkdir -p build
#	@rootcint -f $@ -c $(INCLUDES) $(ROOTCFLAGS) $(notdir $^)

clean:
	@echo "Cleaning up"
	@rm -rf build
