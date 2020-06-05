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

machine jnc;
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
lc_nl  = '\\' '\r'? nl;
esc    = '\\' [^\n];
lit_dq = '"' ([^"\n\\] | esc)* (["\\] | nl);
lit_sq = "'" ([^'\n\\] | esc)* (['\\] | nl);

#. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# main machine
#

main := |*

'var'    { createToken(TokenKind_Var); };
'const'  { createToken(TokenKind_Const); };
'null'   { createToken(TokenKind_Const); };
'assert' { createToken(TokenKind_Assert); };

'++'     { createToken(TokenKind_Inc); };
'--'     { createToken(TokenKind_Dec); };
'*='     { createToken(TokenKind_MulAssign); };
'/='     { createToken(TokenKind_DivAssign); };
'%='     { createToken(TokenKind_ModAssign); };
'+='     { createToken(TokenKind_AddAssign); };
'-='     { createToken(TokenKind_SubAssign); };
'<<='    { createToken(TokenKind_ShlAssign); };
'>>='    { createToken(TokenKind_ShrAssign); };
'&='     { createToken(TokenKind_AndAssign); };
'^='     { createToken(TokenKind_XorAssign); };
'|='     { createToken(TokenKind_OrAssign); };
'<<'     { createToken(TokenKind_Shl); };
'>>'     { createToken(TokenKind_Shr); };
'<='     { createToken(TokenKind_Le); };
'>='     { createToken(TokenKind_Ge); };
'=='     { createToken(TokenKind_Eq); };
'!='     { createToken(TokenKind_Ne); };
'&&'     { createToken(TokenKind_LogicalAnd); };
'||'     { createToken(TokenKind_LogicalOr); };

id               { createStringToken(TokenKind_Identifier); };
lit_sq           { createCharToken(TokenKind_Integer); };
dec+             { createIntegerToken(10); };
'0' oct+         { createIntegerToken(8); };
'0' [xX] hex+    { createIntegerToken(16, 2); };
'0' [oO] oct+    { createIntegerToken(8, 2); };
'0' [bB] bin+    { createIntegerToken(2, 2); };
dec+ ('.' dec*) | ([eE] [+\-]? dec+)
				 { createFpToken(); };

'//' [^\n]*      ;
'/*' (any | nl)* :>> '*/'?
				 ;

ws | nl ;
print    { createToken(ts[0]); };
any      { createErrorToken(ts[0]); };

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

//..............................................................................
