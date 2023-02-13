SillyCon
----
SillyCon is an interpretor for a silly constraint language with Boolean
and numerical constraints. See documentation for more information.

Boolean Propagation Solver
---
The Boolean Propagation Solver (BPS) is a C library for solving Boolean
Propagation Problems. It is used by Sillycon. 

See documentation for more information. 

Copyright & Licence
-------------------

Boolean Propagation Solver and SillyCon are created by Rick van der Meiden
 
Copyright Rick van der Meiden 2013

This (Boolean Propagation Solver and SillyCon) is free software: 
you can redistribute it and/or modify it under the terms of the 
GNU General Public License as published by the Free Software 
Foundation, either version 3 of the License, or (at your option) 
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

See LICENCE file.

Files
-----
README:     this file
LICENCE:    GPL version 3 licence information
doc/*       various documentation
html/*      browser sillycon demo and documentation
sillycon/*  SillyCon source code
core/*      BPS source code
test/*      test program source code
include/    include files for BPS
bin/*       executables
examples/*  example input files

Instructions
------------
These instructions are for a Linux or Unix-like system.

The software was created on a GNU/Linux system
using the gcc compiler.

The C code is simple and uses only standard library
functions. It should not be hard to get it working on other
operating systems. 

To build programs from source, type

    make

To install the SillyCon interpretor system wide, type

    sudo make install

The test program and the sillycon program are created in the bin
directory. Only sillycon is installed system wide by make install.

The examples directory contains input files that can be used 
for the test program (*.prop) and examples that can be used
for the sillycon program (*.silly)

To run a test, from the bps directory, type
    
    bin/test exmaples/test1.prop

To run a sillycon test

    bin/sillycon exmaples/sqrt.silly

For using SillyCon interpretor and Boolean Propagation Solver, 
see documentation files.
