# FSM-Visualizer 

Tool for generating FSM nodegraphs organized with force-based displacement.

HUGE TODO:
 - Makefile
 - Visual Studio project

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

An eventual (but presently out of scope) goal is to add a convenient interface
in the running program to tweak these so that a good balance can be reached.
For now, if you are running on Windows, the platform layer can (and currently
has to) be compiled separately from the rest of the program to allow for
hot-reloading of the application. This enables easy modification of these
constants and other aesthetic parts of the program.

To-do list, in approximate order of importance:

 - Better parsing to allow unmodified input from the UW CSE 311 tester output
    - Better error handling and messages on malformed input
 - In-application interface for modifying simulation parameters and exporting
   images
 - Use of bezier curves with node-to-node links for more aesthetically pleasing
   output (presently only used for node-to-self links)
 - Improved rendering performance on lower-end systems
 - X-Windows platform support for interactive use
