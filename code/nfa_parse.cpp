#pragma once
#include "nfa_parse.h"
#include <cassert>

namespace nfa_parse
{

struct tokenizer
{
    char* At;

    parse_error Error;
    char* ErrorMessage;

    int Line;
    char* LineStart;
};

enum token_type
{
    TT_Unknown = 0,

    TT_Identifier,
    TT_Numeric,
    TT_DeliminatedString,

    TT_Colon,
    TT_Comma,
    TT_Arrow,
    TT_OpenParen,
    TT_CloseParen,
    TT_OpenBracket,
    TT_CloseBracket,

    TT_EmptyLine,
    TT_EOF,

    TT_Count,
};

struct token
{
    token_type Type;
    string Text;
};

//TODO(chronister): More sophisticated error reporting here.
// I want to have this be variadic and take a format string
// instead of a set message, but windows lacks asprintf and
// a conformant vsnprintf implementation, so a shim needs to
// be made.
internal void
ReportError(tokenizer* Tokenizer, parse_error ErrorNum, char* Message)
{
    Tokenizer->Error = ErrorNum;
    Tokenizer->ErrorMessage = Message;
}

inline bool
IsEOF(tokenizer* Tokenizer)
{
    return Tokenizer->At[0] == '\0';
}

inline bool
IsWhitespace(char C)
{
    bool Result = (C == ' ' || 
                   C == '|' ||
                   C == '\n' ||
                   C == '\r');
    return Result;
}

inline bool
IsNumeric(char C)
{
    bool Result = (C >= '0' && C <= '9');
    return Result;
}

inline bool
IsLowerAlpha(char C)
{
    bool Result = (C >= 'a' && C <= 'z');
    return Result;
}

inline bool
IsUpperAlpha(char C)
{
    bool Result = (C >= 'A' && C <= 'Z');
    return Result;
}

inline bool
IsAlpha(char C)
{
    return IsLowerAlpha(C) || IsUpperAlpha(C);
}

inline bool
IsIdentChar(char C)
{
    return IsAlpha(C) || IsNumeric(C);
}

internal void
EatWhitespace(tokenizer* Tokenizer)
{
    while (!IsEOF(Tokenizer) && IsWhitespace(Tokenizer->At[0]))
    {
        if (Tokenizer->At[0] == '\n') {
            ++Tokenizer->Line;
            Tokenizer->LineStart = Tokenizer->At + 1;
        }
        ++Tokenizer->At;
    }
}

internal token
GetToken(tokenizer* Tokenizer)
{
    EatWhitespace(Tokenizer);
    
    token Token = {};
    switch(Tokenizer->At[0])
    {
        case '\0': { Token.Type = TT_EOF; } break;
        case ':': { ++Tokenizer->At; Token.Type = TT_Colon; } break;
        case ',': { ++Tokenizer->At; Token.Type = TT_Comma; } break;
        case '[': { ++Tokenizer->At; Token.Type = TT_OpenBracket; } break;
        case ']': { ++Tokenizer->At; Token.Type = TT_CloseBracket; } break;
        case '(': { ++Tokenizer->At; Token.Type = TT_OpenParen; } break;
        case ')': { ++Tokenizer->At; Token.Type = TT_CloseParen; } break;
        case '-': 
        { 
            ++Tokenizer->At; 
            if (Tokenizer->At[0] == '>') { ++Tokenizer->At; Token.Type = TT_Arrow; }
            else { 
                Token.Type = TT_Identifier; 
                Token.Text.Start = Tokenizer->At - 1;
                Token.Text.Length = 1;
            }
        } break;

        case 1:
        {
            ++Tokenizer->At;
            Token.Text.Start = Tokenizer->At;
            while (!IsEOF(Tokenizer) &&
                   Tokenizer->At[0] != 1)
            {
                ++Tokenizer->At;
            }
            Token.Text.Length = Tokenizer->At - Token.Text.Start;
            Token.Type = TT_DeliminatedString;
            ++Tokenizer->At; // Move it off the second tab
        } break;

        default:
        {
            if (IsIdentChar(Tokenizer->At[0]))
            {
                Token.Type = TT_Identifier;
                Token.Text.Start = Tokenizer->At;
                while (!IsEOF(Tokenizer) && 
                    (IsIdentChar(Tokenizer->At[0]) ||
                     (IsWhitespace(Tokenizer->At[0]) && IsIdentChar(Tokenizer->At[1]))))
                {
                    ++Tokenizer->At;
                }
                Token.Text.Length = Tokenizer->At - Token.Text.Start;
                break;
            }
        } break;
    }

    return Token;
}

internal bool
EatToken(tokenizer* Tokenizer)
{
    token NextToken = GetToken(Tokenizer);
    return NextToken.Type != TT_EOF;
}

internal bool
TokenTextEquals(token Token, char* String)
{
    for (int CharIndex = 0;
        CharIndex < Token.Text.Length;
        ++CharIndex)
    {
        if (String[CharIndex] != Token.Text.Start[CharIndex]) { return false; }
    }
    if (*(String + Token.Text.Length) != '\0') { return false; }
    return true;
}

internal token
RequireToken(tokenizer* Tokenizer, token_type Type)
{
    token Result = GetToken(Tokenizer);
    if (Result.Type != Type) 
    {
        ReportError(Tokenizer, ERR_Missing_Token, "Unexpected token"); //"Expected %d, got %d", Type, Result.Type)
    }
    return Result;
}

internal token
RequireIdentifier(tokenizer* Tokenizer, char* Identifier)
{
    token Result = GetToken(Tokenizer);
    if (Result.Type != TT_Identifier)
    {
        ReportError(Tokenizer, ERR_Missing_Identifier, "Unexpected token"); //"Expected %s, got %s", Identifier, Result)
    }
    if(!(TokenTextEquals(Result, Identifier)))
    {
        ReportError(Tokenizer, ERR_Missing_Identifier, "Unexpected identifier"); //Expected %s, got %s", Identifier, Result)
    }
    return Result;
}

internal token
EatUntilIdentifier(tokenizer* Tokenizer, char* Identifier)
{
    token Result = {};
    do {
        Result = GetToken(Tokenizer);
        if (Result.Type == TT_EOF) 
        {
            ReportError(Tokenizer, ERR_Unexpected_EOF, "Unexpected EOF"); //"While looking for %s", Identifier);
        }
    } while (!(Result.Type == TT_Identifier && TokenTextEquals(Result, Identifier)));
    return Result;
}

internal token
EatUntilToken(tokenizer* Tokenizer, token_type Type)
{
    token Result = {};
    do {
        Result = GetToken(Tokenizer);
        if (Result.Type == TT_EOF) 
        {
            ReportError(Tokenizer, ERR_Unexpected_EOF, "Unexpected EOF"); //"While looking for %d", Type);
        }
    } while (Result.Type != Type);
    return Result;
}

internal token
PeekToken(tokenizer* Tokenizer)
{
    tokenizer Orig = *Tokenizer;
    token Result = GetToken(Tokenizer);
    *Tokenizer = Orig;
    return Result;
}

internal token
PeekTwoTokens(tokenizer* Tokenizer)
{
    tokenizer Orig = *Tokenizer;
    EatToken(Tokenizer);
    token Result = GetToken(Tokenizer);
    *Tokenizer = Orig;
    return Result;
}

extern graph_error
GenerateGraph(char* InputText, graph* Graph)
{
    graph_error Result = {};
    token NextToken;

    s16 PrestartID = AddNode(Graph, NODE_PRESTART); // Reserve 0th node for prestart

    tokenizer TokenizerLocal;
    tokenizer* Tokenizer = &TokenizerLocal;
    Tokenizer->At = InputText;

    EatUntilIdentifier(Tokenizer, "Nfa");
    RequireToken(Tokenizer, TT_OpenParen);
    RequireIdentifier(Tokenizer, "id");
    RequireToken(Tokenizer, TT_Colon);
    token JavaID = RequireToken(Tokenizer, TT_Identifier);
    Graph->JavaID = JavaID.Text;

    EatUntilIdentifier(Tokenizer, "All States");
    EatUntilToken(Tokenizer, TT_Colon);

    do {
        token NameToken = RequireToken(Tokenizer, TT_Identifier);
        RequireToken(Tokenizer, TT_OpenParen);
        RequireIdentifier(Tokenizer, "obj id");
        RequireToken(Tokenizer, TT_Colon);
        token JavaIDToken = RequireToken(Tokenizer, TT_Identifier);
        RequireToken(Tokenizer, TT_CloseParen);

        graph_node Node = {};
        Node.Name = NameToken.Text;
        Node.JavaID = JavaIDToken.Text;
        AddNode(Graph, Node);

        NextToken = PeekToken(Tokenizer);
    } while (!TokenTextEquals(NextToken, "Start State"));

    RequireIdentifier(Tokenizer, "Start State");
    RequireToken(Tokenizer, TT_Colon);
    token StartName = RequireToken(Tokenizer, TT_Identifier);

    graph_node* StartNode = FindNodeByName(Graph, 
                                           StartName.Text.Start, StartName.Text.Length, 
                                           PrestartID);
    assert(StartNode != NULL);
    StartNode->Type = NODE_START;
    AddEdge(Graph, PrestartID, StartNode->ID);

    RequireIdentifier(Tokenizer, "Accept States");
    RequireToken(Tokenizer, TT_Colon);
    RequireToken(Tokenizer, TT_OpenBracket);

    do {
        token AcceptName = RequireToken(Tokenizer, TT_Identifier);

        graph_node* AcceptNode = FindNodeByName(Graph, 
                                                AcceptName.Text.Start, AcceptName.Text.Length, 
                                                PrestartID);
        if (AcceptNode -= NULL) {
            Result.Error = ERR_Unknown_Node;
        }

        AcceptNode->Type = NODE_FINAL;

        NextToken = GetToken(Tokenizer); // Could be comma, could be close brace
    } while (NextToken.Type != TT_CloseBracket);

    RequireIdentifier(Tokenizer, "Transitions");
    RequireToken(Tokenizer, TT_Colon);

    NextToken.Type = TT_Unknown;
    do {
        token SourceName = RequireToken(Tokenizer, TT_Identifier);
        RequireToken(Tokenizer, TT_Colon);

        graph_node* SourceNode = FindNodeByName(Graph, 
                                                SourceName.Text.Start, SourceName.Text.Length, 
                                                PrestartID);
        if (SourceNode == NULL)
        {
            Result.Error = ERR_Unknown_Node;
            /*
            Result.ErrorMessage = asprintf("Graph referenced unknown node %.*s at line %d", 
                                           SourceName.Text.Length, SourceName.Text.Start,
                                           Tokenizer->Line);
                                           */
        }

        do {
            token TransitionChar = RequireToken(Tokenizer, TT_DeliminatedString);
            RequireToken(Tokenizer, TT_Arrow);
            token DestName = RequireToken(Tokenizer, TT_Identifier);

            graph_node* DestNode = FindNodeByName(Graph, 
                                                  DestName.Text.Start, DestName.Text.Length, 
                                                  PrestartID);
            if (DestNode == NULL)
            {
                Result.Error = ERR_Unknown_Node;
                /*
                Result.ErrorMessage = asprintf("Graph referenced unknown node %.*s at line %d", 
                                               DestName.Text.Length, DestName.Text.Start,
                                               Tokenizer->Line);
                                               */
            }
            
            graph_edge NewEdge = {};
            NewEdge.Source = SourceNode->ID;
            NewEdge.Dest = DestNode->ID;
            NewEdge.Transition = TransitionChar.Text;

            graph_edge* ExistingEdge = NULL;
            if ((ExistingEdge = FindEdgeByNodes(Graph, SourceNode->ID, DestNode->ID, PrestartID)) != NULL) 
            {
                ExistingEdge->OtherTransitions++;
                ExistingEdge->Transition.Start = ".";
                ExistingEdge->Transition.Length = 1;
            }
            else
            {
                if ((ExistingEdge = FindEdgeByNodes(Graph, DestNode->ID, SourceNode->ID, PrestartID)) != NULL) 
                {
                    ExistingEdge->HalfBidirectional = true;
                }

                if (DestNode->ID == SourceNode->ID)
                {
                    node_id ControlNode = AddNode(Graph, NODE_CONTROL);
                    NewEdge.Control = ControlNode;
                    NewEdge.Loopback = true;
                }
                AddEdge(Graph, NewEdge);
            }
        } while (PeekTwoTokens(Tokenizer).Type != TT_Colon && PeekToken(Tokenizer).Type != TT_EOF);

    } while (PeekToken(Tokenizer).Type != TT_EOF);

    if (Tokenizer->Error != ERR_No_Parse_Error) {
        Result.Error = Tokenizer->Error;
        Result.ErrorMessage = Tokenizer->ErrorMessage;
    }
    
    return Result;
}

}
