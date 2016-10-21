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
	// common tokens

	TokenKind_Eof = 0,
	TokenKind_Error = -1,
	TokenKind_Identifier = 256,
	TokenKind_Integer,
	TokenKind_Fp,
	TokenKind_Literal,

	// keywords

	TokenKind_Var,
	TokenKind_Func,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

AXL_LEX_BEGIN_TOKEN_NAME_MAP (TokenName)

	// common tokens

	AXL_LEX_TOKEN_NAME (TokenKind_Eof,        "eof")
	AXL_LEX_TOKEN_NAME (TokenKind_Error,      "error")
	AXL_LEX_TOKEN_NAME (TokenKind_Identifier, "identifier")
	AXL_LEX_TOKEN_NAME (TokenKind_Integer,    "integer-constant")
	AXL_LEX_TOKEN_NAME (TokenKind_Fp,         "floating-point-constant")

	// keyword tokens

	AXL_LEX_TOKEN_NAME (TokenKind_Var,  "var")
	AXL_LEX_TOKEN_NAME (TokenKind_Func, "func")
AXL_LEX_END_TOKEN_NAME_MAP ();

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

typedef lex::RagelToken <TokenKind, TokenName, lex::StdTokenData> Token;

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Lexer: public lex::RagelLexer <Lexer, Token>
{
	friend class lex::RagelLexer <Lexer, Token>;

protected:
	Token*
	createStringToken (
		int tokenKind,
		size_t left = 0,
		size_t right = 0
		);

	Token*
	createCharToken (int tokenKind);

	Token*
	createIntegerToken (
		int radix = 10,
		size_t left = 0
		);

	Token*
	createFpToken ();

	// implemented in *.rl

	void
	init ();

	void
	exec ();
};

//..............................................................................
