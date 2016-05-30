/* graphgen.h
 * By Andrew Chronister, (c) 2016
 *
 * Main application-logic structures and graph-related definitions.
 */
#pragma once

// Purpose: Convenience typedefs and macro definitions
#include "types.h"

// Purpose: Vector types
#include "math.hpp"

// Purpose: memory management structures
#include "memory_arena.hpp"

// Purpose: font-rendering related structures
#include "stb_truetype.h"

// Forward declaration of app_state structure, which is referenced by structures
// in render.h
struct app_state;

// Purpose: bitmap structure
#include "render.h"

/* Structure that provides the application with usable blocks of memory and
 * larger pieces of data from the platform layer */
struct app_memory
{
    // Whether or not the structure has been properly initialized
    bool IsInitialized;

    // Size and pointer to the block of memory used for temporary calculations
    //  (for e.g. rendering and simulation needs)
    size_t TemporarySize;
    void* TemporaryBlock;

    // Size and pointer to the block of memory used for more persistent storage
    //  (for e.g. storing the graph and simulation state between steps)
    size_t PermanentSize;
    void* PermanentBlock;

    // Array of files to draw from. We embed directly in the structure for
    // convenience; Usage code will not need to change if this block is
    // made dynamic.
    char* NFAFiles[5];
    int NFAFileCount;

    // Memory block holding the contents of the .ttf file used to render text,
    // loaded by the platform layer.
    u8* TTFFile;
};

/* Structure describing the state of an input button. */
struct button_state
{
    // Is the button currently pressed this frame?
    bool IsPressed;
    // Was the button pressed last frame?
    bool WasPressed;
};

struct app_input
{
    // Timestep used in the simulation.
    // For interactive simulation, this should be the calculated time since the
    // last time UpdateAndRender was called.
    f32 dt;

    // Whether or not the application was hot-reloaded this tick
    bool Reloaded;
    // Whether or not the backing buffer size was changed this tick to accomodate
    // e.g. a window resize
    bool Resized;
    // Whether to perform only simulation activities this tick (and not draw to
    // the buffer).
    bool SimulateOnly;

    // Anonymous structure describing the state of the mouse input this frame.
    struct {
        // Position of the mouse cursor, in pixel coordinates from the center 
        // of the screen which are positive in the upwards direction
        ivec2 P;
        // Change in the mouse's scroll wheel this frame: 1.0f should mean
        // a scroll of 1 tick/nub away from the user
        f32 ScrollDelta;
        // The state of the three main mouse buttons:
        //  0 -> Left click
        //  1 -> Middle click
        //  2 -> Right click
        button_state Buttons[3];
    } Mouse;

    // A platform-designated button which is used to reinitialize the simulation
    // with random node positions.
    // The NFA files in the memory block will also be re-parsed, allowing for
    // hot-swapping of graphs if desired.
    button_state ResetButton;
};

/* Exported function definition for the main entry point to the dynamic library.
 *   Memory: A pointer to an initialized app_memory structure
 *   Buffer: A generic bitmap buffer to draw into
 *   Input: The state of the user input this frame */
#define UPDATE_AND_RENDER(name) void name(app_memory* Memory, bitmap* Buffer, app_input* Input)
typedef UPDATE_AND_RENDER(update_and_render);
extern "C" { update_and_render UpdateAndRender; }

// =====================
//   Graph structures
// =====================

/* Ad-hoc string structure which merely describes a pointer to the start of a
 * string and the length to consider.
 * Can describe a number of kinds of string:
 *  - Explicitly allocated strings, e.g. with malloc
 *  - Substrings of other blocks, e.g. sections of one of the NFA files
 *  - Static string literals
 *  - etc.
 * As such, no assumptions should be made about whether it can be freed.
 */
struct string
{
    char* Start;
    size_t Length;
};

/* Enumeration describing possible types of nodes in the nodegraph. */
enum node_type
{
    // A plain node
    NODE_REGULAR,
    // The entry node to the NFA. Used for aid in simulation/rendering.
    NODE_START,
    // An accept state if the NFA has boolean acceptance. 
    // Used for rendering purposes only.
    NODE_FINAL,

    // Virtual nodes used for rendering aid

    // Invisible node that determines the direction of the start arrow
    NODE_PRESTART,
    // Invisible node that defines the control point for a curve.
    NODE_CONTROL,
};

/* A unique identifier for a node.
 * Current usage is as an index into the Nodes array on the graph structure,
 * but this may change in the future and should not be relied upon. */
typedef s16 node_id;

/* A single node in the graph, along with all of its simulation parameters. */
struct graph_node
{
    // The position of the node
    vec2 P;
    // The velocity vector of the node
    vec2 dP;
    // The acceleration vector of the node
    vec2 ddP;
    // The type of node as described above
    node_type Type;
    // The node's identification number. See definition of node_id for more
    // details.
    node_id ID;
    // String representing the node's friendly name
    string Name;
    // String representing the java hash-code of the node
    string JavaID;
};

/* An edge in the nodegraph, connecting at most two nodes. */
struct graph_edge
{
    // The originating node
    node_id Source;
    // The terminating node
    node_id Dest;
    // [Optional] A node which can be used to help render a curve.
    node_id Control;
    // String describing the transition trigger of the edge.
    string Transition;

    // Count of additional transition triggers on the edge.
    //  TODO(chronister): Temporary solution; This really ought to be
    //  the count of an array of Transition strings.
    int OtherTransitions;

    // Whether or not this node is one-half of an edge which goes both
    // ways. Used in the simulation to avoid applying attractive force
    // in both directions, which can result in strangely short connections.
    bool HalfBidirectional;
};

/* Structure describing an entire nodegraph.
 * For the purposes of simulation, a graph is treated as a self-contained
 * network of nodes.
 * Presently, only one graph can be simulated at a time, so if you wish
 * to have two graphs interact, you should add the nodes of the second graph
 * to the first. */
struct graph
{
    // NOTE: The numbers here are chosen arbitrarily; most node graphs coming
    // out of CSE311 NFAs should not have nearly this many nodes, and the
    // standard simulation and rendering would choke if this number were
    // actually reached.
    // The numbers are meant to be a fairly rediculous upper limit, to give
    // a bound on the memory usage of this part of the application.

    // The number of valid nodes in the Nodes array
    s16 NodeCount;
    // Up to 512 nodes to use in the simulation. Flexible, usage code should
    // not need to be updated if this is changed to a dynamic quantity.
    graph_node Nodes[512];
    // The number of valid edges in the Edges array
    s16 EdgeCount;
    // Up to 1024 edges to use in the simulation. Flexible, usage code should
    // not need to be updated if this is changed to a dynamic quantity.
    graph_edge Edges[1024];

    // [Optional] (Currently unused) Name of the node graph, for display
    // purposes.
    string Name;
    // The java hash-code of the NFA
    string JavaID;
};

/* Structure used to store the current state of the application. */
struct app_state
{
    // Whether or not this structure has been initialized properly
    bool IsInitialized;

    // Memory arena used to store more permanent graph structures
    // such as the graph itself and (speculatively) dynamically
    // allocated nodes.
    memory_arena GraphArena;
    // Memory arena for transient calculations, should be used
    // with temporary_memory to avoid accumulating old data
    // from frame to frame.
    memory_arena TempArena;

    // A pointer to the current node graph being simulated.
    graph* Graph;

    // Scalable value determining the translation from "world" units to screen
    // units
    f32 PixelsPerUnit;

    // stb_truetype font structure describing the font used for text rendering.
    stbtt_fontinfo FontInfo;
};

/* Procedure that adds a node with the same properties as Node to the graph,
 * returning the id of the added node. 
 * Properties guaranteed retained:
 *  - Type
 *  - Name
 *  - JavaID */
node_id AddNode(graph* NodeGraph, graph_node Node);

/* Procedure that adds a new node of the given type to the graph, returning
 * the id of the added node. */
node_id AddNode(graph* NodeGraph, node_type Type = NODE_REGULAR);

/* Procedure that adds an edge with the same properties as Edge to the graph.
 * Properties guaranteed retained:
 *  - Source
 *  - Dest
 *  - Control
 *  - Transition
 *  - OtherTransitions
 *  - HalfBidirectional */
void AddEdge(graph* NodeGraph, graph_edge Edge);

/* Procedure that adds a new edge between the two given nodes to the graph. */
void AddEdge(graph* NodeGraph, node_id Node1, node_id Node2);

/* Procedure that finds a node in the graph by name. IndexStart is the position
 * in the graph's Nodes array to begin searching. */
graph_node* FindNodeByName(graph* Graph, char* NameStart, size_t NameLength, s16 IndexStart = 0);

/* Procedure that finds an edge in the graph by its source and dest node IDs.
 * IndexStart is the position in the graph's Edges array to begin searching. */
graph_edge* FindEdgeByNodes(graph* Graph, node_id StartNode, node_id EndNode, s16 IndexStart = 0);


