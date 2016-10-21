//..............................................................................
//
//  This file is part of the Graco toolkit.
//
//  Graco is distributed under the MIT license.
//  For details see accompanying license.txt file,
//  the public copy of which is also available at:
//  http://tibbo.com/downloads/archive/graco/license.txt
//
//..............................................................................

#pragma once

//..............................................................................

enum TokenKind
{
	TokenKind_Eof = 0,
	TokenKind_Error = -1,
	TokenKind_Identifier = 256,
	TokenKind_Integer,
	TokenKind_Literal,
	TokenKind_Lookahead,
	TokenKind_Import,
	TokenKind_Using,
	TokenKind_Class,
	TokenKind_NoAst,
	TokenKind_Default,
	TokenKind_Arg,
	TokenKind_Local,
	TokenKind_Enter,
	TokenKind_Leave,
	TokenKind_Start,
	TokenKind_Pragma,
	TokenKind_Resolver,
	TokenKind_Priority,
	TokenKind_Any,
	TokenKind_Epsilon,
	TokenKind_Nullable,
	TokenKind_OpenBrace,
	TokenKind_CloseBrace,
	TokenKind_OpenChevron,
	TokenKind_CloseChevron,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

AXL_LEX_BEGIN_TOKEN_NAME_MAP (TokenName)
	AXL_LEX_TOKEN_NAME (TokenKind_Eof,          "eof")
	AXL_LEX_TOKEN_NAME (TokenKind_Error,        "error")
	AXL_LEX_TOKEN_NAME (TokenKind_Identifier,   "identifier")
	AXL_LEX_TOKEN_NAME (TokenKind_Integer,      "integer-constant")
	AXL_LEX_TOKEN_NAME (TokenKind_Literal,      "string-literal")
	AXL_LEX_TOKEN_NAME (TokenKind_Lookahead,    "lookahead")
	AXL_LEX_TOKEN_NAME (TokenKind_Import,       "import")
	AXL_LEX_TOKEN_NAME (TokenKind_Using,        "using")
	AXL_LEX_TOKEN_NAME (TokenKind_Class,        "class")
	AXL_LEX_TOKEN_NAME (TokenKind_NoAst,        "noast")
	AXL_LEX_TOKEN_NAME (TokenKind_Default,      "default")
	AXL_LEX_TOKEN_NAME (TokenKind_Arg,          "arg")
	AXL_LEX_TOKEN_NAME (TokenKind_Local,        "local")
	AXL_LEX_TOKEN_NAME (TokenKind_Enter,        "enter")
	AXL_LEX_TOKEN_NAME (TokenKind_Leave,        "leave")
	AXL_LEX_TOKEN_NAME (TokenKind_Start,        "start")
	AXL_LEX_TOKEN_NAME (TokenKind_Pragma,       "pragma")
	AXL_LEX_TOKEN_NAME (TokenKind_Resolver,     "resolver")
	AXL_LEX_TOKEN_NAME (TokenKind_Priority,     "priority")
	AXL_LEX_TOKEN_NAME (TokenKind_Any,          "any")
	AXL_LEX_TOKEN_NAME (TokenKind_Epsilon,      "epsilon")
	AXL_LEX_TOKEN_NAME (TokenKind_Nullable,     "nullable")
	AXL_LEX_TOKEN_NAME (TokenKind_OpenBrace,    "{.")
	AXL_LEX_TOKEN_NAME (TokenKind_CloseBrace,   ".}")
	AXL_LEX_TOKEN_NAME (TokenKind_OpenChevron,  "<.")
	AXL_LEX_TOKEN_NAME (TokenKind_CloseChevron, ".>")
AXL_LEX_END_TOKEN_NAME_MAP ()

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

typedef lex::RagelToken <TokenKind, TokenName> Token;

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum LexerMachine
{
	LexerMachine_Main,
	LexerMachine_UserCode,
	LexerMachine_UserCode2ndPass,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Lexer: public lex::RagelLexer <Lexer, Token>
{
	friend class lex::RagelLexer <Lexer, Token>;

public:
	static
	int
	getMachineState (LexerMachine machine);

protected:
	Token*
	createStringToken (
		int tokenKind,
		int left = 0,
		int right = 0
		)
	{
		Token* token = createToken (tokenKind);
		token->m_data.m_string = sl::StringRef (ts + left, token->m_pos.m_length - (left + right));
		return token;
	}

	Token*
	createCharToken (int tokenKind)
	{
		Token* token = createToken (tokenKind);
		token->m_data.m_integer = ts [1];
		return token;
	}

	Token*
	createIntegerToken (
		int radix = 10,
		int left = 0
		)
	{
		Token* token = createToken (TokenKind_Integer);
		token->m_data.m_integer = strtol (ts + left, NULL, radix);
		return token;
	}

	Token*
	createConstIntegerToken (int value)
	{
		Token* token = createToken (TokenKind_Integer);
		token->m_data.m_integer = value;
		return token;
	}

	// implemented in *.rl

	void
	init ();

	void
	exec ();
};

//..............................................................................
