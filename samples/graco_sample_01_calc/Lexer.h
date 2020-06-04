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

	// special tokens

	TokenKind_Inc,
	TokenKind_Dec,
	TokenKind_MulAssign,
	TokenKind_DivAssign,
	TokenKind_ModAssign,
	TokenKind_AddAssign,
	TokenKind_SubAssign,
	TokenKind_ShlAssign,
	TokenKind_ShrAssign,
	TokenKind_AndAssign,
	TokenKind_XorAssign,
	TokenKind_OrAssign,
	TokenKind_Shl,
	TokenKind_Shr,
	TokenKind_Le,
	TokenKind_Ge,
	TokenKind_Eq,
	TokenKind_Ne,
	TokenKind_LogicalAnd,
	TokenKind_LogicalOr,

	// keywords

	TokenKind_Var,
	TokenKind_Const,
	TokenKind_Null,
	TokenKind_Assert,

	TokenKind_New,
	TokenKind_Int,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

AXL_LEX_BEGIN_TOKEN_NAME_MAP(TokenName)

	// common tokens

	AXL_LEX_TOKEN_NAME(TokenKind_Eof,        "eof")
	AXL_LEX_TOKEN_NAME(TokenKind_Error,      "error")
	AXL_LEX_TOKEN_NAME(TokenKind_Identifier, "identifier")
	AXL_LEX_TOKEN_NAME(TokenKind_Integer,    "integer-constant")
	AXL_LEX_TOKEN_NAME(TokenKind_Fp,         "floating-point-constant")

	// special tokens

	AXL_LEX_TOKEN_NAME(TokenKind_Inc,        "++")
	AXL_LEX_TOKEN_NAME(TokenKind_Dec,        "--")
	AXL_LEX_TOKEN_NAME(TokenKind_MulAssign,  "*=")
	AXL_LEX_TOKEN_NAME(TokenKind_DivAssign,  "/=")
	AXL_LEX_TOKEN_NAME(TokenKind_ModAssign,  "%=")
	AXL_LEX_TOKEN_NAME(TokenKind_AddAssign,  "+=")
	AXL_LEX_TOKEN_NAME(TokenKind_SubAssign,  "-=")
	AXL_LEX_TOKEN_NAME(TokenKind_ShlAssign,  "<<=")
	AXL_LEX_TOKEN_NAME(TokenKind_ShrAssign,  ">>=")
	AXL_LEX_TOKEN_NAME(TokenKind_AndAssign,  "&=")
	AXL_LEX_TOKEN_NAME(TokenKind_XorAssign,  "^=")
	AXL_LEX_TOKEN_NAME(TokenKind_OrAssign,   "|=")
	AXL_LEX_TOKEN_NAME(TokenKind_Shl,        "<<")
	AXL_LEX_TOKEN_NAME(TokenKind_Shr,        ">>")
	AXL_LEX_TOKEN_NAME(TokenKind_Le,         "<=")
	AXL_LEX_TOKEN_NAME(TokenKind_Ge,         ">=")
	AXL_LEX_TOKEN_NAME(TokenKind_Eq,         "==")
	AXL_LEX_TOKEN_NAME(TokenKind_Ne,         "!=")
	AXL_LEX_TOKEN_NAME(TokenKind_LogicalAnd, "&&")
	AXL_LEX_TOKEN_NAME(TokenKind_LogicalOr,  "||")

	// keyword tokens

	AXL_LEX_TOKEN_NAME(TokenKind_Var,    "var")
	AXL_LEX_TOKEN_NAME(TokenKind_Const,  "const")
	AXL_LEX_TOKEN_NAME(TokenKind_Null,   "null")
	AXL_LEX_TOKEN_NAME(TokenKind_Assert, "assert")

	AXL_LEX_TOKEN_NAME(TokenKind_New, "new")
	AXL_LEX_TOKEN_NAME(TokenKind_Int, "int")

AXL_LEX_END_TOKEN_NAME_MAP();

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

typedef lex::RagelToken<TokenKind, TokenName, lex::StdTokenData> Token;

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Lexer: public lex::RagelLexer<Lexer, Token>
{
	friend class lex::RagelLexer<Lexer, Token>;

protected:
	Token*
	createStringToken(
		int tokenKind,
		size_t left = 0,
		size_t right = 0
		);

	Token*
	createCharToken(int tokenKind);

	Token*
	createIntegerToken(
		int radix = 10,
		size_t left = 0
		);

	Token*
	createFpToken();

	// implemented in *.rl

	void
	init();

	void
	exec();
};

//..............................................................................
