#!/usr/bin/python3
import os

# TODO: make a new libray for this
os.system('mkdir -p bin/')
os.system('g++ -std=c++11 nabu/nabu.cpp -I . -o bin/nabu && ./bin/nabu')