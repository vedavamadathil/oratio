#!/usr/bin/python3
import os

class Build:
    cache = '.smake'
    odir = '.smake/build'
    tdir = '.smake/target'

    def __init__(self, target, sources, idirs):
        self.target = target
        self.sources = sources
        self.idirs = idirs

        self.compiler = 'g++'

        # Create the cache directory
        if not os.path.exists(self.cache):
            os.mkdir(self.cache)
        
        # Create the output directory
        if not os.path.exists(self.odir):
            os.mkdir(self.odir)
        
        # Create target build directory
        self.tpath = self.tdir + '/' + self.target
        if not os.path.exists(self.tdir):
            os.mkdir(self.tdir)
    
    def run(self):
        # List of compiled files
        compiled = []

        # Compile the sources
        for i in range(len(self.sources)):
            # TODO: show the target building message

            # Print
            print(f'[{i + 1}/{len(self.sources)}] Compiling {self.sources[i]}')

            # Get file name without directory
            fname = os.path.basename(self.sources[i])

            # Create the output file
            ofile = os.path.join(self.odir, fname.replace('.cpp', '.o'))

            # Compile the source
            cmd = '{} -c -o {} {} -I {}'.format(self.compiler, ofile, self.sources[i], self.idirs)
            os.system(cmd)

            # TODO: check failure

            # Add to compiled list
            compiled.append(ofile)
        
        # Compile all the objects
        cmd = '{} {} -o {}'.format(self.compiler, ' '.join(compiled), self.tpath) 
        print(f'Linking executable {self.target}')

        os.system(cmd)

        # Return the location of the target
        return self.tpath

class Script:
    pass

# TODO: make a new libray for this
os.system('mkdir -p bin/')
# os.system('g++ -std=c++11 -g nabu/nabu.cpp -I . -o bin/nabu && ./bin/nabu')

build = Build('nabu', ['nabu/nabu.cpp'], '.')
exe = build.run()
os.system(f'{exe} example.nabu')