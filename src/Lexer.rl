// warning C4065: switch statement contains 'default' but no 'case' labels

#pragma warning (disable: 4065)

//.............................................................................

%%{

machine llkc; 
write data;

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# prepush / postpop (for fcall/fret)
#

prepush 
{
	stack = PrePush ();
}

postpop
{
	PostPop ();
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
nl     = '\n' @{ NewLine (p + 1); };
esc    = '\\' [^\n];
lit_dq = '"' ([^"\n\\] | esc)* (["\\] | nl);
lit_sq = "'" ([^'\n\\] | esc)* (['\\] | nl);

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# user code machines
#

# 1st pass extracts user code from .llk file

user_code := |*

lit_sq         ;
lit_dq         ;
[{}()<>]       { CreateToken (ts [0]); };
'{.'           { CreateToken (EToken_OpenBrace); };
'.}'           { CreateToken (EToken_CloseBrace); };
'<.'           { CreateToken (EToken_OpenChevron); };
'.>'           { CreateToken (EToken_CloseChevron); };
nl             ;
any            ;

*|;

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

# 2nd pass extracts locators & delimiters from the output of the 1st pass

user_code_2nd_pass := |*

lit_sq         ;
lit_dq         ;
'$' dec+       { CreateIntegerToken (10, 1); };
'$' 'arg'      { CreateToken (EToken_Arg); };
'$' 'local'    { CreateToken (EToken_Local); };
'$' id         { CreateStringToken (EToken_Identifier, 1); };
'$'            { CreateConstIntegerToken (0); };
[{}()<>,;]     { CreateToken (ts [0]); };
'{.'           { CreateToken (EToken_OpenBrace); };
'.}'           { CreateToken (EToken_CloseBrace); };
'<.'           { CreateToken (EToken_OpenChevron); };
'.>'           { CreateToken (EToken_CloseChevron); };
nl             ;
any            ;

*|;

# . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
#
# main machine 
#

main := |*

'lookahead'    { CreateToken (EToken_Lookahead); };
'import'       { CreateToken (EToken_Import); };
'using'        { CreateToken (EToken_Using); };
'class'        { CreateToken (EToken_Class); };
'noast'        { CreateToken (EToken_NoAst); };
'default'      { CreateToken (EToken_Default); };
'local'        { CreateToken (EToken_Local); };
'enter'        { CreateToken (EToken_Enter); };
'leave'        { CreateToken (EToken_Leave); };
'start'        { CreateToken (EToken_Start); };
'pragma'       { CreateToken (EToken_Pragma); };
'resolver'     { CreateToken (EToken_Resolver); };
'priority'     { CreateToken (EToken_Priority); };
'any'          { CreateToken (EToken_Any); };
'epsilon'      { CreateToken (EToken_Epsilon); };
'nullable'     { CreateToken (EToken_Nullable); };

lit_sq         { CreateCharToken (EToken_Integer); };
lit_dq         { CreateStringToken (EToken_Literal, 1, 1); };
id             { CreateStringToken (EToken_Identifier); };
dec+           { CreateIntegerToken (10); };
'0' [Xx] hex+  { CreateIntegerToken (16, 2); };

'//' [^\n]*    ;
'/*' (any | nl)* :>> '*/' ;

ws | nl        ;
print          { CreateToken (ts [0]); };
any            { CreateErrorToken (ts [0]); };

*|;

}%%

//.............................................................................

void 
CLexer::Init ()
{
	%% write init;
}

void
CLexer::Exec ()
{
	%% write exec;
}

int
CLexer::GetMachineState (ELexerMachine Machine)
{
	switch (Machine)
	{
	case ELexerMachine_Main:
		return llkc_en_main;

	case ELexerMachine_UserCode:
		return llkc_en_user_code;

	case ELexerMachine_UserCode2ndPass:
		return llkc_en_user_code_2nd_pass;

	default:
		ASSERT (false);
		return llkc_en_main;
	}
}

//.............................................................................
