// warning C4065: switch statement contains 'default' but no 'case' labels

#pragma warning (disable: 4065)

//.............................................................................

%%{

machine jnc;
write data;

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
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

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
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

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# main machine
#

main := |*

'var'            { createToken (TokenKind_Var); };
'func'           { createToken (TokenKind_Func); };

id               { createStringToken (TokenKind_Identifier); };
lit_sq           { createCharToken (TokenKind_Integer); };
lit_dq           { createStringToken (TokenKind_Literal, 1, 1); };
dec+             { createIntegerToken (10); };
'0' [xX] hex+    { createIntegerToken (16, 2); };
dec+ ('.' dec+) | ([eE] [+\-]? dec+)
				 { createFpToken (); };

ws | nl          ;
print            { createToken (ts [0]); };
any              { createErrorToken (ts [0]); };

*|;

}%%

//.............................................................................

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

//.............................................................................
