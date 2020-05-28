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
#include "Parser.llk.h"

//..............................................................................

bool
parse(const sl::StringRef& p)
{
	bool result;

	Lexer lexer;
	lexer.create("my-source", p);

	Parser parser;
	parser.create(Parser::StartSymbol);

	for (;;)
	{
		const Token* token = lexer.getToken();
		if (token->m_token == TokenKind_Error)
		{
			err::setFormatStringError("invalid character '\\x%02x'", (uchar_t) token->m_data.m_integer);
			return false;
		}

		result = parser.parseToken(token);
		if (!result)
			return false;

		if (token->m_token == TokenKind_Eof)
			break;

		lexer.nextToken();
	}

	return true;
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

#if (_AXL_OS_WIN)
int
wmain(
	int argc,
	wchar_t* argv[]
	)
#else
int
main(
	int argc,
	char* argv[]
	)
#endif
{
	g::getModule()->setTag("graco_test_cpp");

	bool result = parse(
		"const pi = 3.14159265358979323846;\n"
  		"var r = 100;\n"
		"2 * pi * r;\n"
		"var x, y, ; z = 15; x = y = 10;\n"
  		"assert(x == 10);\n"
		);

	if (!result)
	{
		printf("error: %s\n", err::getLastErrorDescription().sz());
		return -1;
	}

	return 0;
}

//..............................................................................
