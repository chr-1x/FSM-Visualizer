/* nfa_parse.h
 * by Andrew Chronister, (c) 2016
 *
 * Structures and exported function definitions for the NFA parser.
 * Currently requires the NFA to be written using the java class provided elsewhere 
 * in this project.
 */
#pragma once

// Purpose: Graph-related structures and function declarations
#include "graphgen.h"

// Put the parser in its own namespace since some of the names are pretty short
// and potentially conflicting.
namespace nfa_parse
{

/* Enumeration describing possible errors that can occur during the parsing
 * stage. */
enum parse_error
{
    // Everything went fine
    ERR_No_Parse_Error = 0,
    // Hit an unexpected token
    ERR_Missing_Token,
    // Hit an unexpected identifier
    ERR_Missing_Identifier,
    // Hit the end of the file before we were expecting to
    ERR_Unexpected_EOF,

    // Number of items in the enum
    Parse_Error_Count,
};

/* Enumeration describing possible errors that can occur during the graph
 * building stage */
enum graph_generation_error
{
    // Everything went fine
    ERR_No_Graphgen_error = Parse_Error_Count,
    // NFA specified a name of a node that we didn't have recorded in the graph
    ERR_Unknown_Node,
};

/* Data structure which contains an error number and the corresponding error
 * message with more detail about the problem. */
struct graph_error 
{ 
    int Error; char* ErrorMessage; 
};

/* Generates a node graph given the null-terminated NFA file described by
 * InputText. */
extern graph_error 
GenerateGraph(char* InputText, graph* Graph);

}
