# FSM-Visualizer 

![A sample graph generated out of three NFAs](http://cloud.chronal.net/public/nodegraph.png)

C++-based tool for generating FSM nodegraphs organized with force-based displacement.

## Usage

Prebuilt executables will be made available once the feature set has stabilized
a bit.

### Generating NFA data files

Presently, the only way to generate valid data files is by using the provided
NFAWriter class with the UW CSE 311 Grep project.

Copy the `NFAWriter.java` file to your Grep project's src directory. From
anywhere in your code that deals with NFA's you can now add lines like the
following:

    NFAWriter.writeToFile(someNFA, "nfa_test.nfa");

This will write out a string representation of that NFA to the directory where
the code runs. From there, you can copy that `.nfa` file to the `data` folder of
the FSM visualizer.

### Interactive use

On Windows, you can run the simulation interactively. Presently the only
controls are as follows:

 - Scroll to zoom in and out. Note that this will also change the simulation
   boundaries, which can affect the behavior of the graph as one of the forces
   used acts relative to the distance from the edge of the simulation.  It can be
   helpful to zoom out to allow the nodegraph to disperse more effectively, but
   causes text to become difficult to read.
 - Click and hold, and drag the mouse around to exert force on nodes. This is
   helpful for manually pruning a nodegraph to get ideal results.
 - Press space or an arrow key to reset the node positions to a random
   distribution. This is helpful if something is too difficult to fix with the
   mouse positioning or you just get a bad roll.

The windows platform layer will search for a "data" folder in the same directory
as the .exe, and attempt to load the first at-most 5 .nfa files it finds in that
directory. NFA files are currently a slightly modified version of the UW CSE 311
tester output, which has had sentinel values added around transition characters
to ease parsing. This is negotiable and efforts are presently underway to add
support for parsing NFAs from this output without modification.

### Noninteractive use

On POSIX compliant systems, (Linux and Mac, primarily), you can compile with the
`graphgen_static_posix.cpp` platform layer to use the program as a
noninteractive diagram generator. This layer takes as a single command line
argument the name of a .nfa file to parse (same caveats as above), and will
produce as output an image file in `fsm/<name_of_nfa_file>.png`.

The noninteractive program produces its output by running the simulation for a
preset number of steps, determined by the preprocessor constant
`SIMULATION_ITERATIONS` at the top of `graphgen_static_posix.cpp`. The default
value is 1000, which has proven to be sufficient for reasonable large graphs and
provides a nice tradeoff between simulation time and diagram quality. 

Further parameters are available for tweaking at the top of `graphgen.cpp`.
These are the constants used in the simulation. In order:

 -  `RepulsionK` -- Determines the strength of repulsive force between any two
    nodes, which acts similar to electrostatic forces between particles.
 -  `SideRepulsionK` -- The strength of the repulsive force between the sides of
    the simulation and the nodes.
 -  `AttractionK` -- The strength of the spring-like attractive force between
    connected nodes.
 -  `DragK` -- The coefficient of drag applied each step of the simulation.
    Larger values cause slower but more stable convergence.
 -  `NodeRadius` -- The size of the nodes in the simulation. Given in units,
    which at the default zoom level are equvalent to 40 pixels.

## Building/Debugging

### Windows

Use the `build.bat` script to build on windows. It optionally accepts an
argument whose value should be either "win32" or "win64" to build for 32-bit or
64-bit respectively.

In order for this batch file to operate correctly, you will need to have run
vcvarsall.bat on the shell beforehand. This is usually found in a directory
along the lines of `C:\Programs32\Microsoft Visual Studio 14.0\VC\`. 

To debug, you can either run visual studio on the produced executable directly
or rely on `debug.bat` to do so for you. `debug.bat` assumes the `%VC%`
environment variable contains the path used above. If you wish to break on
specific source files, you can simply drag them into Visual Studio from
explorer and place your breakpoints.

I have no intention of making a Visual Studio vcxproj available for building at
this time. 

### Linux

A makefile is provided which should work out of the box on most machines. Use
`make` in the main project directory to build.

To debug, I hope you have a good C++ debugger on hand. I prefer `cgdb` for most
purposes, an ncurses wrapper for gdb that allows you to view the source code
continuously while debugging.

## Reading/Contributing

The source code makes use of a number of C++ features such as light use of
templates, declare-anywhere, typedef-less structs, etc. However, the coding
style is very C-like, preferring POD structs to classes, procedures to methods,
custom memory management to new/delete, etc. Commentary is dense in header files
but currently fairly sparse in the bodies of modules, so some decisions may not
be obvious. Also keep in mind that it was originally written in about 6 hours as
a debug tool, and then expanded over 5 days into a more polished, self-enclosed
product.

If you intend to contribute to this repository, I encourage you to attempt to
match the style of the code. It's somewhat nonstandard, and prefers simplicity
and clarity over reuse of library code / encapsulation. Despite this, I feel it
is fairly straightforward to read and modify.

## Credits

**Adam Blank** -- Teaching the CSE 311 course and providing most of the meat of
the NFAWriter class, as well as creating the Grep project in the first place.

**Sean Barrett** -- for `stb_image_write.h` and `stb_truetype.h`, indispensible
single-header C libraries without which this project would not have been possible.

## To-do list

An eventual (but presently out of scope) goal is to add a convenient interface
in the running program to tweak the simulation parameters so that a good balance
can be reached.  For now, if you are running on Windows, the platform layer can
(and currently has to) be compiled separately from the rest of the program to
allow for hot-reloading of the application. This enables easy modification of
these constants and other aesthetic parts of the program.

The rest of the list, in approximate order of importance:

 - Better parsing to allow unmodified input from the UW CSE 311 tester output
    - Better error handling and messages on malformed input
 - In-application interface for modifying simulation parameters and exporting
   images
 - Use of bezier curves with node-to-node links for more aesthetically pleasing
   output (presently only used for node-to-self links)
 - Improved rendering performance on lower-end systems
 - X-Windows platform support for interactive use
