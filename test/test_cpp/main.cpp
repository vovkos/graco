#include "pch.h"
#include "Lexer.h"
#include "Parser.llk.h"
#include "Parser.llk.cpp"

//.............................................................................

bool
parse (const char* p)
{
	bool result;

	Lexer lexer;
	lexer.create ("my-source", p);

	Parser parser;
	parser.create (Parser::StartSymbol);
	
	for (;;)
	{
		const Token* token = lexer.getToken ();
		if (token->m_token == TokenKind_Error)
		{
			err::setFormatStringError ("invalid character '\\x%02x'", (uchar_t) token->m_data.m_integer);
			return false;
		}

		result = parser.parseToken (token);
		if (!result)
			return false;

		if (token->m_token == TokenKind_Eof)
			break;

		lexer.nextToken ();
	}

	return true;
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

#if (_AXL_ENV == AXL_ENV_WIN)
int
wmain (
	int argc,
	wchar_t* argv []
	)
#else
int
main (
	int argc,
	char* argv []
	)
#endif
{
	bool result = parse ("{ var i = 0; }");
	if (!result)
	{
		printf ("error: %s\n", err::getLastErrorDescription ().cc ());
		return -1;
	}

	return 0;
}

//.............................................................................
