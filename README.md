# FruitJuice
An unpacker and event-builder for GRAPE

##Purpose:
This is a program to assist the analysis of data taken with GRAPE

##Installation:
- obtain the CommandLineInterface and install it to a directory of your liking
- if you are using a Mac, copy the included `Makefile.Mac` to `Makefile`
- in the Makefile change the following line to the directory where you installed the CommandLineInterface:
```
COMMON_DIR = $(HOME)/common
```
- type `make`

##Usage:
The installation will compile two codes: Fruit and Juice
###Fruit:
Fruit is the unpacker and event-builder part. It reads the raw data files, creates hits (class `GrapeHit`) and combines them together to events (class `GrapeEvent`) base on the difference in time-stamp.
It requires a settings file, specifying the data files for the individual detector modules and the event building window.
To run Fruit type:
```
Fruit -s settings.dat -o output.root
```
where `settings.dat` is your settings file, example provided, and `output.root` is the path to your output file.

Optional flags are:
```
Fruit -s settings.dat -o output.root -v verboselevel -lb lastbuffer -wt writetree
```
where `verboselevel` is the level of output printed to stdout, useful for debugging, and understanding, especially in combaination with `lastbuffer` which indicates the last buffer to be read from the file. The `writetree` flag is set by default, if you don't want to write or fill the root tree, set it to `0`. There will be no output, but the code runs faster.

##Juice:
Juice is for additional analysis of the data and for making histograms.
To run it type:
```
Juice -i input.root -o output.root
```
where `input.root` is the sorted tree created by Fruit, and `output.root` contains the histograms.

Optional flags are:
```
Juice -i input.root -o output.root -v verboselevel -le lastevent
```
where `verboselevel` is the level of output printed to stdout, and `lastevent` the number of the last event read from `input.root`
