#pragma once
#include "graphgen.h"

namespace nfa_parse
{

enum parse_error
{
    ERR_No_Parse_Error = 0,
    ERR_Missing_Token,
    ERR_Missing_Identifier,
    ERR_Unexpected_EOF,

    Parse_Error_Count,
};

enum graph_generation_error
{
    ERR_No_Graphgen_error = Parse_Error_Count,
    ERR_Unknown_Node,
};

struct graph_error
{
    int Error;
    char* ErrorMessage;
};

extern graph_error 
GenerateGraph(char* InputText, graph* Graph);

}
