#pragma once
#include "math.hpp"
#include "memory_arena.hpp"
#include "stb_truetype.h"

struct app_state;

#include "render.h"

struct app_memory
{
    bool IsInitialized;

    size_t TemporarySize;
    void* TemporaryBlock;

    size_t PermanentSize;
    void* PermanentBlock;

    char* NFAFiles[5];
    int NFAFileCount;

    u8* TTFFile;
};

struct button_state
{
    bool IsPressed;
    bool WasPressed;
};

struct app_input
{
    f32 dt;

    bool Reloaded;
    bool Resized;

    struct {
        ivec2 P;
        f32 ScrollDelta;
        button_state Buttons[3];
    } Mouse;

    button_state ResetButton;
};

#define UPDATE_AND_RENDER(name) void name(app_memory* Memory, bitmap* Buffer, app_input* Input)
typedef UPDATE_AND_RENDER(update_and_render);
extern "C" { update_and_render UpdateAndRender; }

struct string
{
    char* Start;
    size_t Length;
};

enum node_type
{
    NODE_REGULAR,
    NODE_START,
    NODE_FINAL,

    // Virtual nodes used for rendering aid
    NODE_PRESTART,
    NODE_CONTROL,
};

typedef s16 node_id;

struct graph_node
{
    vec2 P;
    vec2 dP;
    vec2 ddP;
    node_type Type;
    node_id ID;
    string Name;
    string JavaID;
};

struct graph_edge
{
    node_id Source;
    node_id Dest;
    node_id Control;
    string Transition;

    int OtherTransitions;
    bool HalfBidirectional;
    bool Loopback;
};

struct graph
{
    s16 NodeCount;
    graph_node Nodes[512];
    s16 EdgeCount;
    graph_edge Edges[1024]; // NOTE: well, actually there can be 512^2 edges, but...

    string Name;
    string JavaID;
};

struct app_state
{
    bool IsInitialized;

    memory_arena GraphArena;
    memory_arena TempArena;

    int Seed;
    graph* Graph;

    f32 PixelsPerUnit;

    stbtt_fontinfo FontInfo;
};

node_id AddNode(graph* NodeGraph, graph_node Node);

node_id AddNode(graph* NodeGraph, node_type Type = NODE_REGULAR);

void AddEdge(graph* NodeGraph, graph_edge Edge);

void AddEdge(graph* NodeGraph, node_id Node1, node_id Node2);

graph_node* FindNodeByName(graph* Graph, char* NameStart, size_t NameLength, s16 IndexStart = 0);

graph_edge* FindEdgeByNodes(graph* Graph, node_id StartNode, node_id EndNode, s16 IndexStart = 0);


