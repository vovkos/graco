#include "pch.h"
#include "Parser.h"

//.............................................................................

bool
Parser::parseFile (
	Module* module,
	CmdLine* cmdLine,
	const rtl::String& filePath
	)
{
	bool result;

	io::MappedFile srcFile;

	result = srcFile.open (filePath, io::FileFlag_ReadOnly);
	if (!result)
	{
		err::setFormatStringError (
			"cannot open '%s': %s",
			filePath.cc (), // thanks a lot gcc
			err::getError ()->getDescription ().cc ()
			);
		return false;
	}

	size_t size = (size_t) srcFile.getSize ();
	char* p = (char*) srcFile.view (0, size);
	if (!p)
	{
		err::setFormatStringError (
			"cannot open '%s': %s",
			filePath.cc (),
			err::getError ()->getDescription ().cc ()
			);
		return false;
	}

	return parse (module, cmdLine, filePath, p, size);
}

bool
Parser::parse (
	Module* module,
	const CmdLine* cmdLine,
	const rtl::String& filePath,
	const char* source,
	size_t length
	)
{
	bool result;

	m_module = module;
	m_cmdLine = cmdLine;
	m_dir = io::getDir (filePath);

	m_defaultProductionSpecifiers.reset ();

	Lexer::create (filePath, source, length);

	result = program ();
	if (!result)
	{
		ensureSrcPosError ();
		return false;
	}

	return true;
}

bool
Parser::program ()
{
	bool result;

	for (;;)
	{
		const Token* token = getToken ();
		const Token* laToken;

		if (!token->m_token)
			break;

		switch (token->m_token)
		{
		case TokenKind_Lookahead:
			result = lookaheadStatement ();
			if (!result)
				return false;

			break;

		case TokenKind_Import:
			result = importStatement ();
			if (!result)
				return false;

			break;

		case TokenKind_Using:
			result = usingStatement ();
			if (!result)
				return false;

			break;

		case ';':
			nextToken ();
			break;

		case TokenKind_Identifier:
			laToken = getToken (1);
			if (laToken->m_token == '{' || laToken->m_token == '=')
			{
				result = defineStatement ();
				if (!result)
					return false;

				break;
			}

			// fall through

		default:
			result = declarationStatement ();
			if (!result)
				return false;
		}
	}

	return true;
}

bool
Parser::lookaheadStatement ()
{
	const Token* token = getToken ();
	ASSERT (token->m_token == TokenKind_Lookahead);

	nextToken ();

	token = expectToken ('=');
	if (!token)
		return false;

	nextToken ();

	token = expectToken (TokenKind_Integer);
	if (!token)
		return false;

	if (m_module->m_lookaheadLimit && m_module->m_lookaheadLimit != token->m_data.m_integer)
	{
		err::setFormatStringError ("redefinition of lookahead limit (previously seen as %d)", m_module->m_lookaheadLimit);
		return false;
	}

	m_module->m_lookaheadLimit = token->m_data.m_integer;

	nextToken ();

	token = expectToken (';');
	if (!token)
		return false;

	nextToken ();
	return true;
}

bool
Parser::importStatement ()
{
	const Token* token = getToken ();
	ASSERT (token->m_token == TokenKind_Import);

	nextToken ();

	token = expectToken (TokenKind_Literal);
	if (!token)
		return false;

	rtl::String filePath = io::findFilePath (
		token->m_data.m_string,
		m_dir,
		m_cmdLine ? &m_cmdLine->m_importDirList : NULL,
		true
		);

	if (filePath.isEmpty ())
	{
		err::setFormatStringError (
			"cannot find import file '%s'",
			token->m_data.m_string.cc ()
			);
		return false;
	}

	m_module->m_importList.insertTail (filePath);

	nextToken ();

	token = expectToken (';');
	if (!token)
		return false;

	nextToken ();
	return true;
}

bool
Parser::declarationStatement ()
{
	bool result;

	ProductionSpecifiers specifiers = m_defaultProductionSpecifiers;
	result = productionSpecifiers (&specifiers);
	if (!result)
		return false;

	const Token* token = getToken ();
	if (token->m_token == TokenKind_Identifier)
	{
		result = production (&specifiers);
		if (!result)
			return false;
	}

	token = expectToken (';');
	if (!token)
		return false;

	nextToken ();
	return true;
}

bool
Parser::productionSpecifiers (ProductionSpecifiers* specifiers)
{
	Class* cls = NULL;
	uint_t symbolFlags = 0;

	bool isClassSpecified = false;
	bool noMoreSpecifiers = false;

	do
	{
		const Token* token = getToken ();
		const Token* laToken;

		switch (token->m_token)
		{
		case TokenKind_Class:
			if (isClassSpecified)
			{
				err::setStringError ("multiple class specifiers");
				return false;
			}

			cls = classSpecifier ();
			if (!cls)
				return false;

			isClassSpecified = true;
			break;

		case TokenKind_Default:
			if (isClassSpecified)
			{
				err::setStringError ("multiple class specifiers");
				return false;
			}

			nextToken ();

			cls = NULL;
			isClassSpecified = true;
			break;

		case TokenKind_NoAst:
			if (isClassSpecified)
			{
				err::setStringError ("multiple class specifiers");
				return false;
			}

			nextToken ();

			symbolFlags |= SymbolNodeFlag_NoAst;
			isClassSpecified = true;
			break;

		case TokenKind_Pragma:
			if (symbolFlags & SymbolNodeFlag_Pragma)
			{
				err::setStringError ("multiple 'pragma' specifiers");
				return false;
			}

			nextToken ();

			symbolFlags |= SymbolNodeFlag_Pragma;
			break;

		case TokenKind_Start:
			if (symbolFlags & SymbolNodeFlag_Start)
			{
				err::setStringError ("multiple 'start' specifiers");
				return false;
			}

			nextToken ();

			symbolFlags |= SymbolNodeFlag_Start;
			break;

		case TokenKind_Nullable:
			if (symbolFlags & SymbolNodeFlag_Nullable)
			{
				err::setStringError ("multiple 'nullable' specifiers");
				return false;
			}

			nextToken ();

			symbolFlags |= SymbolNodeFlag_Nullable;
			break;

		case TokenKind_Identifier:
			laToken = getToken (1);
			switch (laToken->m_token)
			{
			case TokenKind_Class:
			case TokenKind_Default:
			case TokenKind_NoAst:
			case TokenKind_Pragma:
			case TokenKind_Identifier:
				if (isClassSpecified)
				{
					err::setStringError ("multiple class specifiers");
					return false;
				}

				cls = m_module->m_classMgr.getClass (token->m_data.m_string);
				nextToken ();
				isClassSpecified = true;
				break;

			default:
				noMoreSpecifiers = true;
			}

			break;

		default:
			noMoreSpecifiers = true;
		}
	} while (!noMoreSpecifiers);

	specifiers->m_symbolFlags = symbolFlags;

	if (isClassSpecified)
		specifiers->m_class = cls;

	return true;
}

Class*
Parser::classSpecifier ()
{
	bool result;

	const Token* token = getToken ();
	ASSERT (token->m_token == TokenKind_Class);

	nextToken ();

	Class* cls;

	token = getToken ();

	if (token->m_token != TokenKind_Identifier)
	{
		cls = m_module->m_classMgr.createUnnamedClass ();
	}
	else
	{
		cls = m_module->m_classMgr.getClass (token->m_data.m_string);
		nextToken ();

		token = getToken ();
		if (token->m_token != '{' && token->m_token != ':')
			return cls;

		if (cls->m_flags & ClassFlag_Defined)
		{
			err::setFormatStringError (
				"redefinition of class '%s'",
				cls->m_name.cc ()
				);
			return NULL;
		}
	}

	if (token->m_token == ':')
	{
		nextToken ();

		token = expectToken (TokenKind_Identifier);
		if (!token)
			return NULL;

		cls->m_baseClass = m_module->m_classMgr.getClass (token->m_data.m_string);
		cls->m_baseClass->m_flags |= ClassFlag_Used;

		nextToken ();
	}

	result = userCode ('{', &cls->m_members, &cls->m_srcPos);
	if (!result)
		return NULL;

	cls->m_flags |= ClassFlag_Defined;
	return cls;
}

bool
Parser::defineStatement ()
{
	const Token* token = expectToken (TokenKind_Identifier);
	if (!token)
		return false;

	Define* define = m_module->m_defineMgr.getDefine (token->m_data.m_string);
	define->m_srcPos.m_filePath = m_filePath;

	nextToken ();

	token = getToken ();
	switch (token->m_token)
	{
	case '{':
		return userCode ('{', &define->m_stringValue, &define->m_srcPos);

	case '=':
		nextToken ();

		token = getToken ();
		switch (token->m_token)
		{
		case TokenKind_Identifier:
		case TokenKind_Literal:
			define->m_stringValue = token->m_data.m_string;
			define->m_srcPos = token->m_pos;
			nextToken ();
			break;

		case TokenKind_Integer:
			define->m_kind = DefineKind_Integer;
			define->m_integerValue = token->m_data.m_integer;
			define->m_srcPos = token->m_pos;
			nextToken ();
			break;

		case '{':
			return userCode ('{', &define->m_stringValue, &define->m_srcPos);

		default:
			err::setFormatStringError (
				"invalid define value for '%s'",
				define->m_name.cc ()
				);
			return false;
		}

		break;

	default:
		err::setFormatStringError (
			"invalid define syntax for '%s'",
			define->m_name.cc ()
			);
		return false;
	}

	token = expectToken (';');
	if (!token)
		return false;

	nextToken ();
	return true;
}

bool
Parser::usingStatement ()
{
	bool result;

	const Token* token = getToken ();
	ASSERT (token->m_token == TokenKind_Using);

	nextToken ();

	result = productionSpecifiers (&m_defaultProductionSpecifiers);
	if (!result)
		return false;

	return true;
}

bool
Parser::customizeSymbol (SymbolNode* node)
{
	bool result;

	const Token* token = getToken ();
	if (token->m_token == '<')
	{
		result = userCode ('<', &node->m_arg, &node->m_argLineCol);
		if (!result)
			return false;
	}

	for (;;)
	{
		token = getToken ();

		rtl::String* string = NULL;
		lex::LineCol* lineCol = NULL;

		switch (token->m_token)
		{
		case TokenKind_Local:
			string = &node->m_local;
			lineCol = &node->m_localLineCol;
			break;

		case TokenKind_Enter:
			string = &node->m_enter;
			lineCol = &node->m_enterLineCol;
			break;

		case TokenKind_Leave:
			string = &node->m_leave;
			lineCol = &node->m_leaveLineCol;
			break;
		}

		if (!string)
			break;

		if (!string->isEmpty ())
		{
			err::setFormatStringError (
				"redefinition of '%s'::%s",
				node->m_name.cc (),
				token->getName ()
				);
			return false;
		}

		nextToken ();
		result = userCode ('{', string, lineCol);
		if (!result)
			return false;
	}

	if (!node->m_arg.isEmpty ())
	{
		result = processFormalArgList (node);
		if (!result)
			return false;
	}

	if (!node->m_local.isEmpty ())
	{
		result = processLocalList (node);
		if (!result)
			return false;
	}

	if (!node->m_enter.isEmpty ())
	{
		result = processSymbolEventHandler (node, &node->m_enter);
		if (!result)
			return false;
	}

	if (!node->m_leave.isEmpty ())
	{
		result = processSymbolEventHandler (node, &node->m_leave);
		if (!result)
			return false;
	}

	return true;
}

bool
Parser::processFormalArgList (SymbolNode* node)
{
	const Token* token;

	rtl::String resultString;

	Lexer lexer;
	lexer.create (getMachineState (LexerMachine_UserCode2ndPass), "formal-arg-list", node->m_arg);

	const char* p = node->m_arg;

	for (;;)
	{
		token = lexer.getToken ();

		if (!token->m_token)
			break;

		if (token->m_token == TokenKind_Error)
		{
			err::setFormatStringError ("invalid character '\\x%02x'", (uchar_t) token->m_data.m_integer);
			return false;
		}

		if (token->m_token != TokenKind_Identifier)
		{
			lexer.nextToken ();
			continue;
		}

		rtl::String name = token->m_data.m_string;

		resultString.append (p, token->m_pos.m_p - p);
		resultString.append (name);
		p = token->m_pos.m_p + token->m_pos.m_length;

		lexer.nextToken ();

		node->m_argNameList.insertTail (name);
		node->m_argNameSet.visit (name);

		token = lexer.getToken ();
		if (!token->m_token)
			break;

		token = lexer.expectToken (',');
		if (!token)
			return false;

		lexer.nextToken ();
	}

	ASSERT (!token->m_token);
	resultString.append (p, token->m_pos.m_p - p);

	node->m_arg = resultString;
	return true;
}

bool
Parser::processLocalList (SymbolNode* node)
{
	const Token* token;

	rtl::String resultString;

	Lexer lexer;
	lexer.create (getMachineState (LexerMachine_UserCode2ndPass), "local-list", node->m_local);

	const char* p = node->m_local;

	for (;;)
	{
		token = lexer.getToken ();
		if (token->m_token <= 0)
			break;

		if (token->m_token != TokenKind_Identifier)
		{
			lexer.nextToken ();
			continue;
		}

		rtl::String name = token->m_data.m_string;

		resultString.append (p, token->m_pos.m_p - p);
		resultString.append (name);
		p = token->m_pos.m_p + token->m_pos.m_length;

		lexer.nextToken ();

		node->m_localNameList.insertTail (name);
		node->m_localNameSet.visit (name);
	}

	ASSERT (!token->m_token);
	resultString.append (p, token->m_pos.m_p - p);

	node->m_local = resultString;
	return true;
}

bool
Parser::processSymbolEventHandler (
	SymbolNode* node,
	rtl::String* string
	)
{
	const Token* token;

	rtl::String resultString;

	Lexer lexer;
	lexer.create (getMachineState (LexerMachine_UserCode2ndPass), "event-handler", *string);

	const char* p = *string;

	for (;;)
	{
		token = lexer.getToken ();
		if (token->m_token <= 0)
			break;

		rtl::HashTableIterator <const char*> it;

		switch (token->m_token)
		{
		case TokenKind_Identifier:
			it = node->m_localNameSet.find (token->m_data.m_string);
			if (it)
			{
				resultString.append (p, token->m_pos.m_p - p);
				resultString.appendFormat (
					"$local.%s",
					token->m_data.m_string.cc ()
					);
				break;
			}

			it = node->m_argNameSet.find (token->m_data.m_string);
			if (it)
			{
				resultString.append (p, token->m_pos.m_p - p);
				resultString.appendFormat (
					"$arg.%s",
					token->m_data.m_string.cc ()
					);
				break;
			}

			err::setFormatStringError ("undeclared identifier '%s'", token->m_data.m_string.cc ());
			return false;

		case TokenKind_Integer:
			if (token->m_data.m_integer != 0)
			{
				err::setFormatStringError ("'enter' or 'leave' cannot have indexed references");
				return false;
			}

			resultString.append (p, token->m_pos.m_p - p);
			resultString.append ('$');
			break;

		default:
			lexer.nextToken ();
			continue;
		}

		p = token->m_pos.m_p + token->m_pos.m_length;
		lexer.nextToken ();
	}

	ASSERT (!token->m_token);
	resultString.append (p, token->m_pos.m_p - p);

	*string = resultString;
	return true;
}

bool
Parser::processActualArgList (
	ArgumentNode* node,
	const rtl::String& string
	)
{
	const Token* token;

	Lexer lexer;
	lexer.create (getMachineState (LexerMachine_UserCode2ndPass), "actual-arg-list", string);

	int level = 0;

	const char* p = string;

	for (;;)
	{
		token = lexer.getToken ();
		if (token->m_token <= 0)
			break;

		switch (token->m_token)
		{
		case '(':
		case '{':
		case '[':
			level++;
			break;

		case ')':
		case '}':
		case ']':
			level--;
			break;

		case ',':
			if (level == 0)
			{
				rtl::String valueString (p, token->m_pos.m_p - p);
				node->m_argValueList.insertTail (valueString);

				p = token->m_pos.m_p + token->m_pos.m_length;
			}
		};

		lexer.nextToken ();
	}

	rtl::String valueString (p, token->m_pos.m_p - p);
	node->m_argValueList.insertTail (valueString);

	ASSERT (!token->m_token);
	return true;
}

void
Parser::setGrammarNodeSrcPos (
	GrammarNode* node,
	const lex::LineCol& lineCol
	)
{
	node->m_srcPos.m_filePath = m_filePath;
	node->m_srcPos.m_line = lineCol.m_line;
	node->m_srcPos.m_col = lineCol.m_col;
}

bool
Parser::production (const ProductionSpecifiers* specifiers)
{
	bool result;

	const Token* token = getToken ();
	ASSERT (token->m_token == TokenKind_Identifier);

	SymbolNode* symbol = m_module->m_nodeMgr.getSymbolNode (token->m_data.m_string);
	if (!symbol->m_productionArray.isEmpty ())
	{
		err::setFormatStringError (
			"redefinition of symbol '%s'",
			symbol->m_name.cc ()
			);
		return false;
	}

	setGrammarNodeSrcPos (symbol);

	nextToken ();

	symbol->m_class = specifiers->m_class;
	symbol->m_flags |= specifiers->m_symbolFlags;

	if (symbol->m_class)
		symbol->m_class->m_flags |= ClassFlag_Used;

	if (symbol->m_flags & SymbolNodeFlag_Pragma)
		m_module->m_nodeMgr.m_startPragmaSymbol.m_productionArray.append (symbol);

	if ((symbol->m_flags & SymbolNodeFlag_Start) && !m_module->m_nodeMgr.m_primaryStartSymbol)
		m_module->m_nodeMgr.m_primaryStartSymbol = symbol;

	result = customizeSymbol (symbol);
	if (!result)
		return false;

	token = expectToken (':');
	if (!token)
		return false;

	nextToken ();

	GrammarNode* rightSide = alternative ();
	if (!rightSide)
		return false;

	symbol->addProduction (rightSide);
	return true;
}

GrammarNode*
Parser::alternative ()
{
	GrammarNode* node = sequence ();
	if (!node)
		return NULL;

	SymbolNode* temp = NULL;

	for (;;)
	{
		const Token* token = getToken ();
		if (token->m_token != '|')
			break;

		nextToken ();

		GrammarNode* node2 = sequence ();
		if (!node2)
			return NULL;

		if (!temp)
			if (node->m_kind == NodeKind_Symbol && (node->m_flags & SymbolNodeFlag_Named))
			{
				temp = (SymbolNode*) node;
			}
			else
			{
				temp = m_module->m_nodeMgr.createTempSymbolNode ();
				temp->addProduction (node);

				setGrammarNodeSrcPos (temp, node->m_srcPos);
				node = temp;
			}

		temp->addProduction (node2);
	}

	return node;
}

static
inline
bool
isFirstOfPrimary (int token)
{
	switch (token)
	{
	case TokenKind_Identifier:
	case TokenKind_Integer:
	case TokenKind_Any:
	case TokenKind_Resolver:
	case '{':
	case '(':
	case '.':
		return true;

	default:
		return false;
	}
}

GrammarNode*
Parser::sequence ()
{
	GrammarNode* node = quantifier ();
	if (!node)
		return NULL;

	SequenceNode* temp = NULL;

	for (;;)
	{
		const Token* token = getToken ();

		if (!isFirstOfPrimary (token->m_token))
			break;

		GrammarNode* node2 = quantifier ();
		if (!node2)
			return NULL;

		if (!temp)
			if (node->m_kind == NodeKind_Sequence)
			{
				temp = (SequenceNode*) node;
			}
			else
			{
				temp = m_module->m_nodeMgr.createSequenceNode ();
				temp->append (node);

				setGrammarNodeSrcPos (temp, node->m_srcPos);
				node = temp;
			}

		temp->append (node2);
	}

	return node;
}

GrammarNode*
Parser::quantifier ()
{
	GrammarNode* node = primary ();
	if (!node)
		return NULL;

	const Token* token = getToken ();
	if (token->m_token != '?' &&
		token->m_token != '*' &&
		token->m_token != '+')
		return node;

	GrammarNode* temp = m_module->m_nodeMgr.createQuantifierNode (node, token->m_token);
	if (!temp)
		return NULL;

	setGrammarNodeSrcPos (temp, node->m_srcPos);

	nextToken ();
	return temp;
}

GrammarNode*
Parser::primary ()
{
	bool result;

	GrammarNode* node;
	ActionNode* actionNode;

	const Token* token = getToken ();
	switch (token->m_token)
	{
	case '.':
	case TokenKind_Any:
	case TokenKind_Integer:
		node = beacon ();
		if (!node)
			return NULL;

		break;

	case TokenKind_Epsilon:
		node = &m_module->m_nodeMgr.m_epsilonNode;
		nextToken ();
		break;

	case ';':
	case '|':
		node = &m_module->m_nodeMgr.m_epsilonNode;
		// and don't swallow token
		break;

	case TokenKind_Identifier:
		node = beacon ();
		if (!node)
			return NULL;

		token = getToken ();
		if (token->m_token == '<')
		{
			BeaconNode* beacon = (BeaconNode*) node;
			ArgumentNode* argument = m_module->m_nodeMgr.createArgumentNode ();
			SequenceNode* sequence = m_module->m_nodeMgr.createSequenceNode ();

			setGrammarNodeSrcPos (sequence, node->m_srcPos);

			rtl::String string;
			result = userCode ('<', &string, &argument->m_srcPos);
			if (!result)
				return NULL;

			result = processActualArgList (argument, string);
			if (!result)
				return NULL;

			beacon->m_argument = argument;
			argument->m_targetSymbol = beacon->m_target;

			sequence->append (beacon);
			sequence->append (argument);
			node = sequence;
		}

		break;

	case '{':
		actionNode = m_module->m_nodeMgr.createActionNode ();

		result = userCode ('{', &actionNode->m_userCode, &actionNode->m_srcPos);
		if (!result)
			return NULL;

		node = actionNode;
		break;

	case TokenKind_Resolver:
		node = resolver ();
		if (!node)
			return NULL;

		break;

	case '(':
		nextToken ();

		node = alternative ();
		if (!node)
			return NULL;

		token = expectToken (')');
		if (!token)
			return NULL;

		nextToken ();
		break;

	default:
		err::setUnexpectedTokenError (token->getName (), "primary");
		return NULL;
	}

	return node;
}

BeaconNode*
Parser::beacon ()
{
	SymbolNode* node;

	const Token* token = getToken ();
	switch (token->m_token)
	{
	case TokenKind_Any:
	case '.':
		node = &m_module->m_nodeMgr.m_anyTokenNode;
		break;

	case TokenKind_Identifier:
		node = m_module->m_nodeMgr.getSymbolNode (token->m_data.m_string);
		break;

	case TokenKind_Integer:
		if (!token->m_data.m_integer)
		{
			err::setFormatStringError ("cannot use a reserved eof token \\00");
			return NULL;
		}

		node = m_module->m_nodeMgr.getTokenNode (token->m_data.m_integer);
		break;

	default:
		ASSERT (false);
	}

	BeaconNode* beacon = m_module->m_nodeMgr.createBeaconNode (node);
	setGrammarNodeSrcPos (beacon);

	nextToken ();

	token = getToken ();
	if (token->m_token != '$')
		return beacon;

	nextToken ();
	token = expectToken (TokenKind_Identifier);
	if (!token)
		return NULL;

	beacon->m_label = token->m_data.m_string;
	nextToken ();

	return beacon;
}

SymbolNode*
Parser::resolver ()
{
	const Token* token = getToken ();
	ASSERT (token->m_token == TokenKind_Resolver);

	lex::RagelTokenPos pos = token->m_pos;

	nextToken ();

	token = expectToken ('(');
	if (!token)
		return NULL;

	nextToken ();

	GrammarNode* resolver = alternative ();
	if (!resolver)
		return NULL;

	token = expectToken (')');
	if (!token)
		return NULL;

	nextToken ();

	size_t priority = 0;

	token = getToken ();
	if (token->m_token == TokenKind_Priority)
	{
		nextToken ();

		token = expectToken ('(');
		if (!token)
			return NULL;

		nextToken ();

		token = expectToken (TokenKind_Integer);
		if (!token)
			return NULL;

		priority = token->m_data.m_integer;
		nextToken ();

		token = expectToken (')');
		if (!token)
			return NULL;

		nextToken ();
	}

	GrammarNode* production = sequence ();
	if (!production)
		return NULL;

	SymbolNode* temp = m_module->m_nodeMgr.createTempSymbolNode ();
	temp->m_resolver = resolver;
	temp->m_resolverPriority = priority;
	temp->addProduction (production);

	setGrammarNodeSrcPos (temp, pos);

	return temp;
}

bool
Parser::userCode (
	int openBracket,
	rtl::String* string,
	lex::SrcPos* srcPos
	)
{
	bool result = userCode (openBracket, string, (lex::LineCol*) srcPos);
	if (!result)
		return false;

	srcPos->m_filePath = m_filePath;
	return true;
}

bool
Parser::userCode (
	int openBracket,
	rtl::String* string,
	lex::LineCol* lineCol
	)
{
	const Token* token = expectToken (openBracket);
	if (!token)
		return false;

	gotoState (getMachineState (LexerMachine_UserCode), token, GotoStateKind_ReparseToken);

	token = getToken ();

	openBracket = token->m_token; // could change! e.g. { vs {. and < vs <.

	int closeBracket;

	switch (openBracket)
	{
	case '{':
		closeBracket = '}';
		break;

	case '(':
		closeBracket = ')';
		break;

	case '<':
		closeBracket = '>';
		break;

	case TokenKind_OpenBrace:
		closeBracket = TokenKind_CloseBrace;
		break;

	case TokenKind_OpenChevron:
		closeBracket = TokenKind_CloseChevron;
		break;

	default:
		ASSERT (false);

		err::setFormatStringError ("invalid user code opener '%s'", Token::getName (openBracket));
		return false;
	}

	const char* begin = token->m_pos.m_p + token->m_pos.m_length;
	*lineCol = token->m_pos;

	nextToken ();

	int level = 1;

	for (;;)
	{
		token = getToken ();

		if (token->m_token == TokenKind_Eof)
		{
			err::setUnexpectedTokenError ("eof", "user-code");
			return false;
		}
		else if (token->m_token == TokenKind_Error)
		{
			err::setFormatStringError ("invalid character '\\x%02x'", (uchar_t) token->m_data.m_integer);
			return false;
		}
		else if (token->m_token == openBracket)
		{
			level++;
		}
		else if (token->m_token == closeBracket)
		{
			level--;

			if (level <= 0)
				break;
		}

		nextToken ();
	}

	size_t end = token->m_pos.m_offset;

	token = getToken ();
	ASSERT (token->m_token == closeBracket);

	*string = rtl::String (begin, token->m_pos.m_p - begin);

	gotoState (getMachineState (LexerMachine_Main), token, GotoStateKind_EatToken);

	return true;
}

//.............................................................................
