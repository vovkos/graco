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

// warning C4065: switch statement contains 'default' but no 'case' labels

#pragma warning (disable: 4065)

//..............................................................................

%%{

machine llkc;
write data;

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# prepush / postpop (for fcall/fret)
#

prepush
{
	stack = prePush ();
}

postpop
{
	postPop ();
}

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# standard definitions
#

dec    = [0-9];
hex    = [0-9a-fA-F];
oct    = [0-7];
bin    = [01];
id     = [_a-zA-Z] [_a-zA-Z0-9]*;
ws     = [ \t\r]+;
nl     = '\n' @{ newLine (p + 1); };
esc    = '\\' [^\n];
lit_dq = '"' ([^"\n\\] | esc)* (["\\] | nl);
lit_sq = "'" ([^'\n\\] | esc)* (['\\] | nl);

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# user code machines
#

# 1st pass extracts user code from .llk file

user_code := |*

lit_sq         ;
lit_dq         ;
[{}()<>]       { createToken(ts[0]); };
'{.'           { createToken(TokenKind_OpenBrace); };
'.}'           { createToken(TokenKind_CloseBrace); };
'<.'           { createToken(TokenKind_OpenChevron); };
'.>'           { createToken(TokenKind_CloseChevron); };
nl             ;
any            ;

*|;

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

# 2nd pass extracts locators & delimiters from the output of the 1st pass

user_code_2nd_pass := |*

lit_sq         ;
lit_dq         ;
'$' dec+       { createIntegerToken(10, 1); };
'$' id         { createStringToken(TokenKind_Identifier, 1); };
'$'            { createConstIntegerToken(0); };
[{}()<>,;]     { createToken(ts[0]); };
'{.'           { createToken(TokenKind_OpenBrace); };
'.}'           { createToken(TokenKind_CloseBrace); };
'<.'           { createToken(TokenKind_OpenChevron); };
'.>'           { createToken(TokenKind_CloseChevron); };
nl             ;
any            ;

*|;

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# main machine
#

main := |*

'import'       { createToken(TokenKind_Import); };
'struct'       { createToken(TokenKind_Struct); };
'class'        { createToken(TokenKind_Struct); };
'default'      { createToken(TokenKind_Default); };
'local'        { createToken(TokenKind_Local); };
'enter'        { createToken(TokenKind_Enter); };
'leave'        { createToken(TokenKind_Leave); };
'start'        { createToken(TokenKind_Start); };
'pragma'       { createToken(TokenKind_Pragma); };
'catch'        { createToken(TokenKind_Catch); };
'lookahead'    { createToken(TokenKind_Lookahead); };
'resolve'      { createToken(TokenKind_Resolve); };
'priority'     { createToken(TokenKind_Priority); };
'any'          { createToken(TokenKind_Any); };
'epsilon'      { createToken(TokenKind_Epsilon); };
'nullable'     { createToken(TokenKind_Nullable); };
'true'         { createToken(TokenKind_True); };
'false'        { createToken(TokenKind_False); };

lit_sq         { createCharToken(TokenKind_Integer); };
lit_dq         { createStringToken(TokenKind_Literal, 1, 1); };
id             { createStringToken(TokenKind_Identifier); };
dec+           { createIntegerToken(10); };
'0' [xx] hex+  { createIntegerToken(16, 2); };

'//' [^\n]*    ;
'/*' (any | nl)* :>> '*/' ;

ws | nl        ;
print          { createToken(ts[0]); };
any            { createErrorToken(ts[0]); };

*|;

}%%

//..............................................................................

void
Lexer::init ()
{
	%% write init;
}

void
Lexer::exec ()
{
	%% write exec;
}

int
Lexer::getMachineState (LexerMachine machine)
{
	switch (machine)
	{
	case LexerMachine_Main:
		return llkc_en_main;

	case LexerMachine_UserCode:
		return llkc_en_user_code;

	case LexerMachine_UserCode2ndPass:
		return llkc_en_user_code_2nd_pass;

	default:
		ASSERT (false);
		return llkc_en_main;
	}
}

//..............................................................................
