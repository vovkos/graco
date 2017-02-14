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

#include "pch.h"
#include "Lexer.h"
#include "Lexer.rl.cpp"

//..............................................................................

Token*
Lexer::createStringToken (
	int tokenKind,
	size_t left,
	size_t right
	)
{
	Token* token = createToken (tokenKind);
	ASSERT (token->m_pos.m_length >= left + right);

	size_t length = token->m_pos.m_length - (left + right);
	token->m_data.m_string = sl::StringRef (ts + left, length);
	return token;
}

Token*
Lexer::createCharToken (int tokenKind)
{
	Token* token = createToken (tokenKind);
	ASSERT (token->m_pos.m_length >= 2);

	char buffer [256];
	sl::String string (ref::BufKind_Stack, buffer, sizeof (buffer));
	enc::EscapeEncoding::decode (&string, sl::StringRef (ts + 1, token->m_pos.m_length - 2));

	token->m_data.m_integer = string [0];
	return token;
}

Token*
Lexer::createIntegerToken (
	int radix,
	size_t left
	)
{
	Token* token = createToken (TokenKind_Integer);
	token->m_data.m_int64_u = _strtoui64 (ts + left, NULL, radix);
	return token;
}

Token*
Lexer::createFpToken ()
{
	Token* token = createToken (TokenKind_Fp);
	token->m_data.m_double = strtod (ts, NULL);
	return token;
}

//..............................................................................
