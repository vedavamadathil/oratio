#!/usr/bin/python3
import os

class Build:
	pass

# TODO: make a new libray for this
os.system('mkdir -p bin/')
os.system('g++ -std=c++11 -g nabu/nabu.cpp -I . -o bin/nabu && ./bin/nabu')
