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
parse(const sl::StringRef& source) {
	bool result;

	Lexer lexer;
	lexer.create(source);

	Parser parser;
	parser.create("my-source", Parser::StartSymbol);

	bool isEof;
	do {
		Token* token = lexer.takeToken();
		isEof = token->m_token == TokenKind_Eof;
		result = parser.consumeToken(token);
		if (!result)
			return false;
	} while (!isEof);

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
	lex::registerParseErrorProvider();

	bool result = parse(
		"const pi = 3.14159265358979323846;\n"
  		"var r = 100;\n"
		"2 * pi * r;\n"
		"var x, y, ; z = 15; x = y = 10;\n"
  		"assert(x == 10);\n"
		);

	if (!result) {
		printf("error: %s\n", err::getLastErrorDescription().sz());
		return -1;
	}

	return 0;
}

//..............................................................................
