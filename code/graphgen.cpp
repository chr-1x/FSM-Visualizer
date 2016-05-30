#include <cstring>
#include "types.h"
#include "math.hpp"
#include "bezier.hpp"
#include "graphgen.h"
#include "render.h"
#include "nfa_parse.h"

static const f32 RepulsionK = 45.0f;
static const f32 SideRepulsionK = 600.0f;
static const f32 AttractionK = 4.0f;
static const f32 DragK = 6.0f;
static const f32 NodeRadius = 0.8f;

internal f32
RandRange(f32 MinVal, f32 MaxVal)
{
    // rand isn't great but it'll suffice here
    f32 Result = ((f32)rand() / (f32)RAND_MAX) * (MaxVal - MinVal) + MinVal;
    return Result;
}

vec2 InitialNodePlacement(node_id NodeID)
{
    NodeID;
    return V2(7.0f*RandRange(-1,1), 7.0f*RandRange(-1,1));
}

node_id AddNode(graph* NodeGraph, graph_node Node)
{
    graph_node NewNode = Node;
    NewNode.ID = NodeGraph->NodeCount++;
    NewNode.P = InitialNodePlacement(NewNode.ID);
    NodeGraph->Nodes[NewNode.ID] = NewNode;
    return NewNode.ID;
}

node_id AddNode(graph* NodeGraph, node_type Type)
{
    graph_node NewNode = {};
    NewNode.ID = NodeGraph->NodeCount++;
    NewNode.Type = Type;
    NewNode.P = InitialNodePlacement(NewNode.ID);
    NodeGraph->Nodes[NewNode.ID] = NewNode;
    return NewNode.ID;
}

void AddEdge(graph* NodeGraph, graph_edge Edge)
{
    NodeGraph->Edges[NodeGraph->EdgeCount++] = Edge;
}

void AddEdge(graph* NodeGraph, node_id Node1, node_id Node2)
{
    graph_edge NewEdge = {};
    NewEdge.Source = Node1;
    NewEdge.Dest = Node2;
    NodeGraph->Edges[NodeGraph->EdgeCount++] = NewEdge;
}

internal bool
StringSegmentsEqual(size_t CompareLength, const char* String1, const char* String2)
{
	for (uint i = 0;
		i < CompareLength;
		++i)
	{
		if (String1[i] != String2[i]) { return false; }
	}

	return true;
}

graph_node* FindNodeByName(graph* Graph, 
                           char* NameStart, size_t NameLength, s16 IndexStart)
{
    graph_node* Result = NULL;
    for (s16 NodeIndex = IndexStart; NodeIndex < Graph->NodeCount; ++NodeIndex) {
        graph_node* Node = Graph->Nodes + NodeIndex;

        if (Node->Name.Length != NameLength) { continue; }
        if (StringSegmentsEqual((int)Node->Name.Length, Node->Name.Start, NameStart))
        {
            Result = Node;
            break;
        }
    }
    return Result;
}

graph_edge* FindEdgeByNodes(graph* Graph, 
                            node_id StartNode, node_id EndNode, s16 IndexStart)
{
    graph_edge* Result = NULL;
    for (s16 EdgeIndex = IndexStart; EdgeIndex < Graph->EdgeCount; ++EdgeIndex) {
        graph_edge* Edge = Graph->Edges + EdgeIndex;

        if (Edge->Source == StartNode && Edge->Dest == EndNode)
        {
            Result = Edge;
            break;
        }
    }
    return Result;
}

internal vec2
BounceVector(vec2 SurfaceVec, vec2 Incoming)
{
    vec2 SurfaceNormal = LeftNormal(SurfaceVec);
    vec2 Result = -(2*Inner(SurfaceNormal, Incoming) * SurfaceNormal - Incoming);
    return Result;
}

internal void
SimulateGraph(graph* NodeGraph, vec2 MinSide, vec2 MaxSide, vec2 MouseP, f32 dt)
{
    for (s16 EdgeIndex = 0; EdgeIndex < NodeGraph->EdgeCount; ++EdgeIndex)
    {
        graph_edge* Edge = NodeGraph->Edges + EdgeIndex;
        if (Edge->HalfBidirectional) { continue; }
        graph_node* Node1 = NodeGraph->Nodes + Edge->Source;
        graph_node* Node2 = NodeGraph->Nodes + Edge->Dest;

        f32 AttractionKLocal = AttractionK;

        if (Edge->Loopback)
        {
            // Always maintain a short length for aesthetic reasons
            Node2 = NodeGraph->Nodes + Edge->Control;
            AttractionKLocal = 2.0f*AttractionK;
        }
        if (Node1->Type == NODE_PRESTART)
        {
            AttractionKLocal = 3.0f*AttractionK;
        }

        // Attraction:
        // Usually, people use Hookian springs here,
        // but I found that for my purposes, a simple generic attraction
        // in the direction of the link seems to work well with the other
        // parts of the simulation.

        vec2 Diff = Node2->P - Node1->P;
        vec2 nDiff = Normalize(Diff);

        Node1->ddP += AttractionKLocal * nDiff;
        Node2->ddP += -AttractionKLocal * nDiff;
    }

    // Hooray, n^2 Updates
    for (s16 Node1Index = 0; Node1Index < NodeGraph->NodeCount; ++Node1Index)
    {
        vec2 DeltaX, nDeltaX;

        graph_node* Node1 = NodeGraph->Nodes + Node1Index;
        for (s16 Node2Index = 0; Node2Index < NodeGraph->NodeCount; ++Node2Index)
        {
            if (Node2Index == Node1Index) { continue; }
            graph_node* Node2 = NodeGraph->Nodes + Node2Index;

            DeltaX = Node2->P - Node1->P;
            nDeltaX = Normalize(DeltaX);

            if (Node1->Type == NODE_CONTROL &&
                Node2->Type == NODE_CONTROL)
            {
                // Control points shouldn't really affect each other
                continue;
            }

            f32 RepulsionKLocal = RepulsionK;

            if (Node1->Type == NODE_CONTROL ||
                Node2->Type == NODE_CONTROL ||
                Node1->Type == NODE_PRESTART ||
                Node2->Type == NODE_PRESTART)
            {
                // Allow control points to be a lot closer to other nodes
                RepulsionKLocal = 0.3f*RepulsionK;
            }

            // Coloumbesque repulsion: Fr = k(q1*q2)/r^2
            // Except things don't have "charge", so the numerator term is entirely arbitrary
            // And there's not really mass, so it's only loosely treated as a force.

            f32 RadiusSq = LengthSq(DeltaX);
            f32 RepulsionMagnitude = SafeRatio0(RepulsionKLocal, RadiusSq);
            
            Node1->ddP += -nDeltaX * RepulsionMagnitude;
            Node2->ddP += nDeltaX * RepulsionMagnitude;
        }


        // Repulsive force from edges
        vec2 Sides[2] = { MinSide, MaxSide }; 

        for (int SideIndex = 0; SideIndex < 2; ++SideIndex)
        {
            // Min side
            DeltaX = Sides[SideIndex] - Node1->P;
            nDeltaX = Normalize(DeltaX);
            vec2 RadiusSq = V2(Square(DeltaX.x), Square(DeltaX.y));
            vec2 RepulsionMagnitude = SideRepulsionK * V2(SafeRatio0(1.0f , RadiusSq.x), SafeRatio0(1.0f, RadiusSq.y));

            if (Node1->Type == NODE_CONTROL ||
                Node1->Type == NODE_PRESTART) 
            {
                // Control points aren't as repulsed by the edges
                RepulsionMagnitude = 0.1f * RepulsionMagnitude;
            }

            Node1->ddP += -Hadamard(nDeltaX, RepulsionMagnitude);
        }

        // Repulsive force from mouse cursor
        DeltaX =(vec2)(MouseP) -  Node1->P ;
        nDeltaX = Normalize(DeltaX);
        f32 RadiusSq = LengthSq(DeltaX);
        f32 RepulsionMagnitude = SafeRatio0(RepulsionK, RadiusSq);
        
        Node1->ddP += -nDeltaX * RepulsionMagnitude;
    }

    // Do the update
    for (s16 NodeIndex = 0; NodeIndex < NodeGraph->NodeCount; ++NodeIndex)
    {
        graph_node* Node = NodeGraph->Nodes + NodeIndex;

        Node->ddP += -Node->dP * DragK;

        Node->dP += Node->ddP * dt;
        Node->P += Node->dP * dt;

        Node->ddP = V2(0.0f, 0.0f);
    }
            
    // Collision detection last
    // Hooray, n^2 Updates
    for (s16 Node1Index = 0; Node1Index < NodeGraph->NodeCount; ++Node1Index)
    {
        graph_node* Node1 = NodeGraph->Nodes + Node1Index;
        for (s16 Node2Index = 0; Node2Index < NodeGraph->NodeCount; ++Node2Index)
        {
            if (Node2Index == Node1Index) { continue; }
            graph_node* Node2 = NodeGraph->Nodes + Node2Index;

            vec2 Diff = Node2->P - Node1->P;
            vec2 NormalDiff = Normalize(Diff);
            if (LengthSq(Diff) < Square(2*NodeRadius))
            {
                vec2 Center = Node1->P + 0.5f*Diff;
                vec2 X1 = Node1->P = Center - NodeRadius*NormalDiff;
                vec2 X2 = Node2->P = Center + NodeRadius*NormalDiff;

                vec2 V1 = Node1->dP;
                vec2 V2 = Node2->dP;

                Node1->dP = V1 - SafeRatio0(Inner(V1 - V2, X1 - X2),LengthSq(X1 - X2))*(X1 - X2);
                Node2->dP = V2 - SafeRatio0(Inner(V2 - V1, X2 - X1),LengthSq(X2 - X1))*(X2 - X1);

                Diff = Node2->P - Node1->P;
                NormalDiff = Normalize(Diff);
            }
        }

        if (Node1->P.x - NodeRadius < MinSide.x) {
            Node1->P.x = MinSide.x + NodeRadius;
            Node1->dP.x = -Node1->dP.x;
        }
        else if (Node1->P.x + NodeRadius > MaxSide.x) {
            Node1->P.x = MaxSide.x - NodeRadius;
            Node1->dP.x = -Node1->dP.x;
        }

        if (Node1->P.y - NodeRadius < MinSide.y) {
            Node1->P.y = MinSide.y + NodeRadius;
            Node1->dP.y = -Node1->dP.y;
        }
        else if (Node1->P.y + NodeRadius > MaxSide.y) {
            Node1->P.y = MaxSide.y - NodeRadius;
            Node1->dP.y = -Node1->dP.y;
        }
    }
}

internal void
DrawGraph(app_state* State, bitmap* Target, rgba_color BGColor, graph* Graph)
{
    BGColor;
    f32 LineWidth = 2.0f / State->PixelsPerUnit;
    for (s16 EdgeIndex = 0; EdgeIndex < Graph->EdgeCount; ++EdgeIndex)
    {
        graph_edge* Edge = Graph->Edges + EdgeIndex;
        graph_node* StartNode = Graph->Nodes + Edge->Source;
        graph_node* EndNode = Graph->Nodes + Edge->Dest;

        vec2 Diff = EndNode->P - StartNode->P;
        vec2 P1 = StartNode->P + NodeRadius * Normalize(Diff); 
        vec2 P2 = EndNode->P - NodeRadius * Normalize(Diff); 

        if (StartNode->Type == NODE_PRESTART) {
            P1 = EndNode->P - NodeRadius * 3 * Normalize(Diff);
        }

        vec2 LabelP;
        if (Edge->Loopback)
        {
            graph_node* ControlNode = Graph->Nodes + Edge->Control;
            vec2 ControlP = ControlNode->P;
            vec2 NodeDir = Normalize(ControlP - StartNode->P);
            f32 ControlDist = 4.0f;
            bezier_cubic<1> EdgeCurve = CurveCubic<1>(StartNode->P, StartNode->P + (0.5f*ControlDist)*NodeDir + (0.5f*ControlDist)*Perp(NodeDir), 
                                                                    StartNode->P + (0.5f*ControlDist)*NodeDir - (0.5f*ControlDist)*Perp(NodeDir), StartNode->P);
            DrawBezierCubicSegment(State, Target, 1, (verts<4>*)EdgeCurve.Segments, EdgeCurve.TotalLength,
                                   LineWidth, V4(0,0,0,1), 15, false);

            LabelP = StartNode->P + 0.45f*ControlDist * NodeDir;
        }
        else {
            vec2 LineVerts[2] = { P1, P2 };
            DrawLinearArrow(State, Target, LineVerts, 1.5f*LineWidth, 4.0f*LineWidth, V4(0,0,0,1));

            LabelP = StartNode->P + 0.5f*Diff + 0.3f*LeftNormal(Diff);
        }

        if (Edge->Transition.Start != NULL)
        {
            if (Edge->Transition.Length == 1 && 
                Edge->Transition.Start[0] == '-') 
            {
                int CodePoint = 0x03b5;
                //TODO(chronister): THIS IS WRONG
                DrawWorldString(State, Target, 1, (char*)&CodePoint,
                                LabelP, 0.6f, V4(0,0,0,1));
            }
            else 
            {
                DrawWorldString(State, Target, Edge->Transition.Length, Edge->Transition.Start,
                                LabelP, 0.6f, V4(0,0,0,1));
            }
        }
    }

    for (s16 NodeIndex = 0; NodeIndex < Graph->NodeCount; ++NodeIndex)
    {
        graph_node* Node = Graph->Nodes + NodeIndex;
        switch (Node->Type)
        {
            case NODE_START:
            case NODE_REGULAR:
            {
                DrawOval(State, Target, Node->P, V2(NodeRadius,NodeRadius), V4(0,0,0,1));
                DrawOval(State, Target, Node->P, V2(NodeRadius-LineWidth,NodeRadius-LineWidth), V4(1,1,1,1));
            } break;

            case NODE_FINAL:
            {
                DrawOval(State, Target, Node->P, V2(NodeRadius,NodeRadius), V4(0,0,0,1));
                DrawOval(State, Target, Node->P, V2(NodeRadius-LineWidth,NodeRadius-LineWidth), V4(1,1,1,1));
                DrawOval(State, Target, Node->P, V2(NodeRadius-2*LineWidth,NodeRadius-2*LineWidth), V4(0,0,0,1));
                DrawOval(State, Target, Node->P, V2(NodeRadius-3*LineWidth,NodeRadius-3*LineWidth), V4(1,1,1,1));
            } break;

            case NODE_PRESTART:
            case NODE_CONTROL:
            {
#if 0
                DrawOval(State, Target, Node->P, V2(0.2f,0.2f), V4(0,0,0,0.4f));
#endif
            } break;
        }

        if (Node->Type != NODE_PRESTART && Node->Name.Start != NULL)
        {
            f32 Scale = 0.42f;
            if (Node->Type == NODE_FINAL) {
                Scale = 0.33f;
            }
            DrawWorldString(State, Target, Node->Name.Length, Node->Name.Start,
                               Node->P, Scale, V4(0,0,0,1));
        }
    }

    DrawString(State, Target, 8, "GraphGen",
               V2(0.0f, Target->Height/2 - 27.0f), 27.0f, V4(0,0,0,1));
}

extern "C"
UPDATE_AND_RENDER(UpdateAndRender)
{
    if (!Memory->IsInitialized) { return; }

    app_state* State = (app_state*)Memory->PermanentBlock;
    if (!State->IsInitialized) 
    {
        InitializeArena(&State->GraphArena, 
                        Memory->PermanentSize - sizeof(app_state), 
                        (void*)((u8*)Memory->PermanentBlock + sizeof(app_state)));

        InitializeArena(&State->TempArena, 
                        Memory->TemporarySize - sizeof(app_state), 
                        (void*)((u8*)Memory->TemporaryBlock + sizeof(app_state)));

        stbtt_InitFont(&State->FontInfo,
                       Memory->TTFFile,
                       stbtt_GetFontOffsetForIndex(Memory->TTFFile, 0));

        State->PixelsPerUnit = 40;

        State->Graph = PushStruct(&State->GraphArena, graph);

        memset(State->Graph, 0, sizeof(graph));

        for (int NFAFileIndex = 0; NFAFileIndex < Memory->NFAFileCount; ++NFAFileIndex)
        {
            nfa_parse::GenerateGraph(Memory->NFAFiles[NFAFileIndex], State->Graph);
        }

        State->IsInitialized = true;
    }

    if (Input->ResetButton.IsPressed && !Input->ResetButton.WasPressed)
    {
        memset(State->Graph, 0, sizeof(graph));

        for (int NFAFileIndex = 0; NFAFileIndex < Memory->NFAFileCount; ++NFAFileIndex)
        {
            nfa_parse::GenerateGraph(Memory->NFAFiles[NFAFileIndex], State->Graph);
        }
    }

    State->PixelsPerUnit = State->PixelsPerUnit * Pow(1.1f, Input->Mouse.ScrollDelta);

    vec2 VirtualMouseP = Input->Mouse.P/State->PixelsPerUnit;
    if (!Input->Mouse.Buttons[0].IsPressed) { VirtualMouseP = V2(-50000, -50000); }

    SimulateGraph(State->Graph, 
                  -0.5f*Buffer->Dim/State->PixelsPerUnit, 
                  0.5f*Buffer->Dim/State->PixelsPerUnit, 
                  VirtualMouseP,
                  Input->dt);

    ClearBitmap(Buffer, V4(1,1,1,1));
    DrawGraph(State, Buffer, V4(1,1,1,0), State->Graph);

    DrawOval(State, Buffer, V2(Input->Mouse.P.X, Input->Mouse.P.Y) / State->PixelsPerUnit,
             V2(0.2f, 0.2f), V4(0,0,0,0.5f));
}
