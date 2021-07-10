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
#include "Parser.h"

//..............................................................................

Parser::ProductionSpecifiers::ProductionSpecifiers()
{
	m_resolver = NULL;
	m_lookaheadLimit = 0;
	m_flags = 0;
}

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

bool
Parser::parseFile(const sl::StringRef& filePath)
{
	bool result;

	io::MappedFile srcFile;

	result = srcFile.open(filePath, io::FileFlag_ReadOnly);
	if (!result)
	{
		err::setFormatStringError(
			"cannot open '%s': %s",
			filePath.sz(),
			err::getLastErrorDescription().sz()
			);
		return false;
	}

	size_t size = (size_t)srcFile.getSize();
	char* p = (char*)srcFile.view(0, size);
	if (!p)
	{
		err::setFormatStringError(
			"cannot open '%s': %s",
			filePath.sz(),
			err::getLastErrorDescription().sz()
			);
		return false;
	}

	return parse(filePath, sl::StringRef(p, size));
}

bool
Parser::parse(
	const sl::StringRef& filePath,
	const sl::StringRef& source
	)
{
	const sl::String& cachedSource = m_module->cacheSource(source);
	Lexer::create(filePath, cachedSource);

	m_dir = io::getDir(filePath);
	m_module->m_nodeMgr.m_lookaheadLimit = m_cmdLine->m_lookaheadLimit;

	bool result = program();
	if (!result)
	{
		ensureSrcPosError();
		return false;
	}

	return true;
}

bool
Parser::program()
{
	for (;;)
	{
		const Token* token = getToken();
		if (!token->m_token)
			break;

		const Token* laToken;
		size_t lookaheadLimit = 0;
		bool result = true;

		switch (token->m_token)
		{
		case ';':
			nextToken();
			break;

		case TokenKind_Import:
			result = importStatement();
			break;

		case TokenKind_Lookahead:
			lookaheadLimit = lookahead();
			if (lookaheadLimit == -1)
				return false;

			laToken = getToken();
			if (laToken->m_token != ';')
			{
				result = declarationStatement(lookaheadLimit);
				break;
			}

			nextToken();

			m_module->m_nodeMgr.m_lookaheadLimit = lookaheadLimit ?
				lookaheadLimit :
				m_cmdLine->m_lookaheadLimit;

			lookaheadLimit = 0;
			break;

		case TokenKind_Identifier:
			laToken = getToken(1);
			if (laToken->m_token == '{' || laToken->m_token == '=')
			{
				result = defineStatement();
				break;
			}

			result = declarationStatement();
			break;

		default:
			result = declarationStatement();
		}

		if (!result)
			return false;
	}

	return true;
}

bool
Parser::importStatement()
{
	const Token* token = getToken();
	ASSERT(token->m_token == TokenKind_Import);

	nextToken();

	token = expectToken(TokenKind_Literal);
	if (!token)
		return false;

	sl::String filePath = io::findFilePath(
		token->m_data.m_string,
		m_dir,
		m_cmdLine ? &m_cmdLine->m_importDirList : NULL,
		true
		);

	if (filePath.isEmpty())
	{
		err::setFormatStringError(
			"cannot find import file '%s'",
			token->m_data.m_string.sz()
			);
		return false;
	}

	m_module->m_importList.insertTail(filePath);
	nextToken();
	return true;
}

bool
Parser::declarationStatement(size_t lookaheadLimit)
{
	bool result;

	ProductionSpecifiers specifiers;
	specifiers.m_lookaheadLimit = lookaheadLimit;

	result =
		productionSpecifiers(&specifiers) &&
		production(&specifiers);

	if (!result)
		return false;

	const Token* token = expectToken(';');
	if (!token)
		return false;

	nextToken();
	return true;
}

bool
Parser::productionSpecifiers(ProductionSpecifiers* specifiers)
{
	bool result;
	bool isValueSpecified = false;
	bool noMoreSpecifiers = false;

	do
	{
		const Token* token = getToken();
		switch (token->m_token)
		{
		case TokenKind_Struct:
			if (isValueSpecified)
			{
				err::setError("multiple value specifiers");
				return false;
			}

			nextToken();
			result = userCode('{', &specifiers->m_valueBlock, &specifiers->m_valueLineCol);
			if (!result)
				return false;

			isValueSpecified = true;
			break;

		case TokenKind_Lookahead:
			result = lookaheadSpecifier(specifiers);
			if (!result)
				return false;

			break;

		case TokenKind_Resolver:
			result = resolverSpecifier(specifiers);
			if (!result)
				return false;

			break;

		case TokenKind_Pragma:
			if (specifiers->m_flags & SymbolNodeFlag_Pragma)
			{
				err::setError("multiple 'pragma' specifiers");
				return false;
			}

			nextToken();
			specifiers->m_flags |= SymbolNodeFlag_Pragma;
			break;

		case TokenKind_Start:
			if (specifiers->m_flags & SymbolNodeFlag_Start)
			{
				err::setError("multiple 'start' specifiers");
				return false;
			}

			nextToken();
			specifiers->m_flags |= SymbolNodeFlag_Start;
			break;

		case TokenKind_Nullable:
			if (specifiers->m_flags & SymbolNodeFlag_Nullable)
			{
				err::setError("multiple 'nullable' specifiers");
				return false;
			}

			nextToken();
			specifiers->m_flags |= SymbolNodeFlag_Nullable;
			break;

		default:
			noMoreSpecifiers = true;
		}
	} while (!noMoreSpecifiers);

	return true;
}

size_t
Parser::lookahead()
{
	const Token* token = getToken();
	ASSERT(token->m_token == TokenKind_Lookahead);

	nextToken();

	token = expectToken('(');
	if (!token)
		return -1;

	nextToken();

	size_t lookaheadLimit;

	token = getToken();
	switch (token->m_tokenKind)
	{
	case TokenKind_Default:
		lookaheadLimit = 0;
		break;

	case TokenKind_Integer:
		lookaheadLimit = token->m_data.m_integer;
		break;

	default:
		err::setFormatStringError("invalid lookahead specified");
		return -1;
	}

	nextToken();

	if (getToken(0)->m_token == ',' && getToken(1)->m_token == TokenKind_Default)
	{
		nextToken(2);
		m_cmdLine->m_lookaheadLimit = lookaheadLimit;
	}

	token = expectToken(')');
	if (!token)
		return -1;

	nextToken();
	return lookaheadLimit;
}

bool
Parser::lookaheadSpecifier(ProductionSpecifiers* specifiers)
{
	if (specifiers->m_lookaheadLimit)
	{
		err::setError("multiple 'lookahead' specifiers");
		return false;
	}

	specifiers->m_lookaheadLimit = lookahead();
	return specifiers->m_lookaheadLimit != -1;
}

bool
Parser::resolverSpecifier(ProductionSpecifiers* specifiers)
{
	if (specifiers->m_resolver)
	{
		err::setError("multiple 'resolver' specifiers");
		return false;
	}

	const Token* token = getToken();
	ASSERT(token->m_token == TokenKind_Resolver);

	nextToken();

	token = expectToken('(');
	if (!token)
		return false;

	nextToken();

	specifiers->m_resolver = alternative();
	if (!specifiers->m_resolver)
		return false;

	specifiers->m_resolverPriority = 0;

	token = getToken();
	if (token->m_token == ',')
	{
		nextToken();

		token = expectToken(TokenKind_Integer);
		specifiers->m_resolverPriority = token->m_data.m_integer;
		nextToken();
	}

	token = expectToken(')');
	if (!token)
		return false;

	nextToken();
	return true;
}

bool
Parser::defineStatement()
{
	const Token* token = expectToken(TokenKind_Identifier);
	if (!token)
		return false;

	Define* define = m_module->m_defineMgr.getDefine(token->m_data.m_string);
	define->m_srcPos.m_filePath = m_filePath;

	nextToken();

	token = getToken();
	switch (token->m_token)
	{
	case '{':
		return userCode('{', &define->m_stringValue, &define->m_srcPos);

	case '=':
		nextToken();

		token = getToken();
		switch (token->m_token)
		{
		case TokenKind_Identifier:
		case TokenKind_Literal:
			define->m_stringValue = token->m_data.m_string;
			break;

		case TokenKind_Integer:
			define->m_defineKind = DefineKind_Integer;
			define->m_integerValue = token->m_data.m_integer;
			break;

		case TokenKind_True:
		case TokenKind_False:
			define->m_defineKind = DefineKind_Bool;
			define->m_integerValue = token->m_tokenKind == TokenKind_True;
			break;

		case '{':
			return userCode('{', &define->m_stringValue, &define->m_srcPos);

		default:
			err::setFormatStringError(
				"invalid define value for '%s'",
				define->m_name.sz()
				);
			return false;
		}

		define->m_srcPos = token->m_pos;
		nextToken();
		break;

	default:
		err::setFormatStringError(
			"invalid define syntax for '%s'",
			define->m_name.sz()
			);
		return false;
	}

	token = expectToken(';');
	if (!token)
		return false;

	nextToken();
	return true;
}

bool
Parser::customizeSymbol(SymbolNode* node)
{
	bool result;

	const Token* token = getToken();
	if (token->m_token == '<')
	{
		result = userCode('<', &node->m_paramBlock, &node->m_paramLineCol);
		if (!result)
			return false;
	}

	for (;;)
	{
		token = getToken();

		sl::StringRef* string = NULL;
		lex::LineCol* lineCol = NULL;

		switch (token->m_token)
		{
		case TokenKind_Local:
			string = &node->m_localBlock;
			lineCol = &node->m_localLineCol;
			break;

		case TokenKind_Enter:
			string = &node->m_enterBlock;
			lineCol = &node->m_enterLineCol;
			break;

		case TokenKind_Leave:
			string = &node->m_leaveBlock;
			lineCol = &node->m_leaveLineCol;
			break;
		}

		if (!string)
			break;

		if (!string->isEmpty())
		{
			err::setFormatStringError(
				"redefinition of '%s'::%s",
				node->m_name.sz(),
				token->getName()
				);
			return false;
		}

		nextToken();
		result = userCode('{', string, lineCol);
		if (!result)
			return false;
	}

	if (!node->m_paramBlock.isEmpty())
	{
		result = processParamBlock(node);
		if (!result)
			return false;
	}

	if (!node->m_localBlock.isEmpty())
	{
		result = processLocalBlock(node);
		if (!result)
			return false;
	}

	if (!node->m_enterBlock.isEmpty())
	{
		result = processEnterLeaveBlock(node, &node->m_enterBlock);
		if (!result)
			return false;
	}

	if (!node->m_leaveBlock.isEmpty())
	{
		result = processEnterLeaveBlock(node, &node->m_leaveBlock);
		if (!result)
			return false;
	}

	return true;
}

bool
Parser::processParamBlock(SymbolNode* node)
{
	Lexer lexer;
	lexer.create(getMachineState(LexerMachine_UserCode2ndPass), "param-list", node->m_paramBlock);
	const char* p = node->m_paramBlock.cp();
	const Token* token;
	sl::String resultString;

	for (;;)
	{
		token = lexer.getToken();

		if (!token->m_token)
			break;

		if (token->m_token == TokenKind_Error)
		{
			err::setFormatStringError("invalid character '\\x%02x'", (uchar_t) token->m_data.m_integer);
			return false;
		}

		if (token->m_token != TokenKind_Identifier)
		{
			lexer.nextToken();
			continue;
		}

		sl::String name = token->m_data.m_string;

		resultString.append(p, token->m_pos.m_p - p);
		resultString.append(name);
		p = token->m_pos.m_p + token->m_pos.m_length;

		lexer.nextToken();

		node->m_paramNameList.insertTail(name);
		node->m_paramNameSet.visit(name);

		token = lexer.getToken();
		if (!token->m_token)
			break;

		token = lexer.expectToken(',');
		if (!token)
			return false;

		lexer.nextToken();
	}

	ASSERT(!token->m_token);
	resultString.append(p, token->m_pos.m_p - p);
	node->m_paramBlock = resultString;
	return true;
}

bool
Parser::processLocalBlock(SymbolNode* node)
{
	Lexer lexer;
	lexer.create(getMachineState(LexerMachine_UserCode2ndPass), "local-list", node->m_localBlock);
	const char* p = node->m_localBlock.cp();
	const Token* token;
	sl::String resultString;

	for (;;)
	{
		token = lexer.getToken();
		if (token->m_token <= 0)
			break;

		if (token->m_token != TokenKind_Identifier)
		{
			lexer.nextToken();
			continue;
		}

		sl::String name = token->m_data.m_string;

		resultString.append(p, token->m_pos.m_p - p);
		resultString.append(name);
		p = token->m_pos.m_p + token->m_pos.m_length;

		lexer.nextToken();

		node->m_localNameList.insertTail(name);
		node->m_localNameSet.visit(name);
	}

	ASSERT(!token->m_token);
	resultString.append(p, token->m_pos.m_p - p);
	node->m_localBlock = resultString;
	return true;
}

bool
Parser::processEnterLeaveBlock(
	SymbolNode* node,
	sl::StringRef* string
	)
{
	Lexer lexer;
	lexer.create(getMachineState(LexerMachine_UserCode2ndPass), "event-handler", *string);
	const char* p = string->cp();
	const Token* token;
	sl::String resultString;

	for (;;)
	{
		token = lexer.getToken();
		if (token->m_token <= 0)
			break;

		sl::StringHashTableIterator<bool> it;

		switch (token->m_token)
		{
		case TokenKind_Identifier:
			it = node->m_localNameSet.find(token->m_data.m_string);
			if (it)
			{
				resultString.append(p, token->m_pos.m_p - p);
				resultString.appendFormat(
					"$local.%s",
					token->m_data.m_string.sz()
					);
				break;
			}

			it = node->m_paramNameSet.find(token->m_data.m_string);
			if (it)
			{
				resultString.append(p, token->m_pos.m_p - p);
				resultString.appendFormat(
					"$param.%s",
					token->m_data.m_string.sz()
					);
				break;
			}

			err::setFormatStringError("undeclared identifier '%s'", token->m_data.m_string.sz());
			return false;

		case TokenKind_Integer:
			if (token->m_data.m_integer != 0)
			{
				err::setFormatStringError("'enter' or 'leave' cannot have indexed references");
				return false;
			}

			resultString.append(p, token->m_pos.m_p - p);
			resultString.append('$');
			break;

		default:
			lexer.nextToken();
			continue;
		}

		p = token->m_pos.m_p + token->m_pos.m_length;
		lexer.nextToken();
	}

	ASSERT(!token->m_token);
	resultString.append(p, token->m_pos.m_p - p);
	*string = resultString;
	return true;
}

bool
Parser::processActualArgList(
	ArgumentNode* node,
	const sl::StringRef& string
	)
{
	const Token* token;

	Lexer lexer;
	lexer.create(getMachineState(LexerMachine_UserCode2ndPass), "actual-arg-list", string);

	int level = 0;

	const char* p = string.cp();

	for (;;)
	{
		token = lexer.getToken();
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
				sl::String valueString(p, token->m_pos.m_p - p);
				node->m_argValueList.insertTail(valueString);

				p = token->m_pos.m_p + token->m_pos.m_length;
			}
		};

		lexer.nextToken();
	}

	sl::String valueString(p, token->m_pos.m_p - p);
	node->m_argValueList.insertTail(valueString);

	ASSERT(!token->m_token);
	return true;
}

void
Parser::setGrammarNodeSrcPos(
	GrammarNode* node,
	const lex::LineCol& lineCol
	)
{
	node->m_srcPos.m_filePath = m_filePath;
	node->m_srcPos.m_line = lineCol.m_line;
	node->m_srcPos.m_col = lineCol.m_col;
}

bool
Parser::production(const ProductionSpecifiers* specifiers)
{
	bool result;

	const Token* token = expectToken(TokenKind_Identifier);
	if (!token)
		return false;

	SymbolNode* symbol = m_module->m_nodeMgr.getSymbolNode(token->m_data.m_string);
	if (!symbol->m_productionArray.isEmpty())
	{
		err::setFormatStringError(
			"redefinition of symbol '%s'",
			symbol->m_name.sz()
			);
		return false;
	}

	setGrammarNodeSrcPos(symbol);

	nextToken();

	symbol->m_valueBlock = specifiers->m_valueBlock;
	symbol->m_valueLineCol = specifiers->m_valueLineCol;
	symbol->m_flags |= specifiers->m_flags;

	if (specifiers->m_resolver)
	{
		// resolver is a temp symbol with a single production

		SymbolNode* temp = m_module->m_nodeMgr.createTempSymbolNode();
		temp->addProduction(specifiers->m_resolver);
		setGrammarNodeSrcPos(temp, specifiers->m_resolver->m_srcPos);

		symbol->m_resolver = temp;
		symbol->m_resolverPriority = specifiers->m_resolverPriority;
	}

	symbol->m_lookaheadLimit = specifiers->m_lookaheadLimit ?
		specifiers->m_lookaheadLimit :
		m_module->m_nodeMgr.m_lookaheadLimit;

	if (symbol->m_flags & SymbolNodeFlag_Pragma)
		m_module->m_nodeMgr.m_pragmaStartSymbol.m_productionArray.append(symbol);

	if ((symbol->m_flags & SymbolNodeFlag_Start) && !m_module->m_nodeMgr.m_primaryStartSymbol)
		m_module->m_nodeMgr.m_primaryStartSymbol = symbol;

	result = customizeSymbol(symbol);
	if (!result)
		return false;

	token = expectToken(':');
	if (!token)
		return false;

	nextToken();

	GrammarNode* rightSide = alternative();
	if (!rightSide)
		return false;

	symbol->addProduction(rightSide);
	return true;
}

GrammarNode*
Parser::alternative()
{
	GrammarNode* node = sequence();
	if (!node)
		return NULL;

	SymbolNode* temp = NULL;

	for (;;)
	{
		const Token* token = getToken();
		if (token->m_token != '|')
			break;

		nextToken();

		GrammarNode* node2 = sequence();
		if (!node2)
			return NULL;

		if (!temp)
			if (node->m_nodeKind == NodeKind_Symbol && (node->m_flags & SymbolNodeFlag_User))
			{
				temp = (SymbolNode*)node;
			}
			else
			{
				temp = m_module->m_nodeMgr.createTempSymbolNode();
				temp->addProduction(node);

				setGrammarNodeSrcPos(temp, node->m_srcPos);
				node = temp;
			}

		temp->addProduction(node2);
	}

	return node;
}

static
inline
bool
isFirstOfPrimary(int token)
{
	switch (token)
	{
	case TokenKind_Identifier:
	case TokenKind_Integer:
	case TokenKind_EofToken:
	case TokenKind_AnyToken:
	case TokenKind_Catch:
	case '{':
	case '(':
	case '.':
		return true;

	default:
		return false;
	}
}

GrammarNode*
Parser::sequence()
{
	GrammarNode* node = quantifier();
	if (!node)
		return NULL;

	SequenceNode* temp = NULL;

	for (;;)
	{
		const Token* token = getToken();

		if (!isFirstOfPrimary(token->m_token))
			break;

		GrammarNode* node2 = quantifier();
		if (!node2)
			return NULL;

		if (!temp)
			if (node->m_nodeKind == NodeKind_Sequence)
			{
				temp = (SequenceNode*)node;
			}
			else
			{
				temp = m_module->m_nodeMgr.createSequenceNode();
				temp->append(node);

				setGrammarNodeSrcPos(temp, node->m_srcPos);
				node = temp;
			}

		temp->append(node2);
	}

	return node;
}

GrammarNode*
Parser::quantifier()
{
	GrammarNode* node = primary();
	if (!node)
		return NULL;

	const Token* token = getToken();
	if (token->m_token != '?' &&
		token->m_token != '*' &&
		token->m_token != '+')
		return node;

	GrammarNode* temp = m_module->m_nodeMgr.createQuantifierNode(node, token->m_token);
	if (!temp)
		return NULL;

	setGrammarNodeSrcPos(temp, node->m_srcPos);

	nextToken();
	return temp;
}

GrammarNode*
Parser::primary()
{
	bool result;

	GrammarNode* node;
	ActionNode* actionNode;

	const Token* token = getToken();
	switch (token->m_token)
	{
	case '.':
	case TokenKind_EofToken:
	case TokenKind_AnyToken:
	case TokenKind_Integer:
		node = beacon();
		break;

	case TokenKind_Epsilon:
		node = &m_module->m_nodeMgr.m_epsilonNode;
		nextToken();
		break;

	case ';':
	case '|':
		node = &m_module->m_nodeMgr.m_epsilonNode;
		// and don't swallow token
		break;

	case TokenKind_Identifier:
		node = beacon();
		if (!node)
			return NULL;

		token = getToken();
		if (token->m_token == '<')
		{
			BeaconNode* beacon = (BeaconNode*)node;
			ArgumentNode* argument = m_module->m_nodeMgr.createArgumentNode();
			SequenceNode* sequence = m_module->m_nodeMgr.createSequenceNode();

			setGrammarNodeSrcPos(sequence, node->m_srcPos);

			sl::StringRef string;
			result = userCode('<', &string, &argument->m_srcPos);
			if (!result)
				return NULL;

			result = processActualArgList(argument, string);
			if (!result)
				return NULL;

			beacon->m_argument = argument;
			argument->m_targetSymbol = beacon->m_target;

			sequence->append(beacon);
			sequence->append(argument);
			node = sequence;
		}

		break;

	case '{':
		actionNode = m_module->m_nodeMgr.createActionNode();

		result = userCode('{', &actionNode->m_userCode, &actionNode->m_srcPos);
		if (!result)
			return NULL;

		node = actionNode;
		break;

	case TokenKind_Catch:
		node = catcher();
		break;

	case '(':
		nextToken();
		node = alternative();
		token = expectToken(')');
		if (!token)
			return NULL;

		nextToken();
		break;

	default:
		lex::setUnexpectedTokenError(token->getName(), "primary");
		return NULL;
	}

	return node;
}

BeaconNode*
Parser::beacon()
{
	SymbolNode* node;

	const Token* token = getToken();
	switch (token->m_token)
	{
	case TokenKind_EofToken:
		node = &m_module->m_nodeMgr.m_eofTokenNode;
		break;

	case TokenKind_AnyToken:
	case '.':
		node = &m_module->m_nodeMgr.m_anyTokenNode;
		break;

	case TokenKind_Identifier:
		node = m_module->m_nodeMgr.getSymbolNode(token->m_data.m_string);
		break;

	case TokenKind_Integer:
		if (!token->m_data.m_integer)
		{
			err::setFormatStringError("cannot use a reserved eof token \\00");
			return NULL;
		}

		node = m_module->m_nodeMgr.getTokenNode(token->m_data.m_integer);
		break;

	default:
		ASSERT(false);
	}

	BeaconNode* beacon = m_module->m_nodeMgr.createBeaconNode(node);
	setGrammarNodeSrcPos(beacon);

	nextToken();

	token = getToken();
	if (token->m_token != '$')
		return beacon;

	nextToken();
	token = expectToken(TokenKind_Identifier);
	if (!token)
		return NULL;

	beacon->m_label = token->m_data.m_string;
	nextToken();

	return beacon;
}

SymbolNode*
Parser::catcher()
{
	const Token* token = getToken();
	ASSERT(token->m_token == TokenKind_Catch);

	nextToken();

	token = expectToken('(');
	if (!token)
		return NULL;

	nextToken();

	GrammarNode* synchronizer = alternative();
	if (!synchronizer)
		return NULL;

	token = expectToken(')');
	if (!token)
		return NULL;

	nextToken();

	GrammarNode* production = sequence();
	if (!production)
		return NULL;

	// catcher is a temp symbol with a single production

	SymbolNode* catcher = m_module->m_nodeMgr.createCatchSymbolNode();
	catcher->m_synchronizer = synchronizer;
	catcher->m_productionArray.copy(production);
	setGrammarNodeSrcPos(catcher, production->m_srcPos);
	return catcher;
}

bool
Parser::userCode(
	int openBracket,
	sl::StringRef* string,
	lex::SrcPos* srcPos
	)
{
	bool result = userCode(openBracket, string, (lex::LineCol*)srcPos);
	if (!result)
		return false;

	srcPos->m_filePath = m_filePath;
	return true;
}

bool
Parser::userCode(
	int openBracket,
	sl::StringRef* string,
	lex::LineCol* lineCol
	)
{
	const Token* token = expectToken(openBracket);
	if (!token)
		return false;

	gotoState(getMachineState(LexerMachine_UserCode), token, GotoStateKind_ReparseToken);

	token = getToken();

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
		ASSERT(false);

		err::setFormatStringError("invalid user code opener '%s'", Token::getName (openBracket));
		return false;
	}

	const char* begin = token->m_pos.m_p + token->m_pos.m_length;

	*lineCol = token->m_pos;

	nextToken();

	int level = 1;

	for (;;)
	{
		token = getToken();

		if (token->m_token == TokenKind_Eof)
		{
			lex::setUnexpectedTokenError("eof", "user-code");
			return false;
		}
		else if (token->m_token == TokenKind_Error)
		{
			err::setFormatStringError("invalid character '\\x%02x'", (uchar_t) token->m_data.m_integer);
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

		nextToken();
	}

	const char* end = token->m_pos.m_p;

	// skip the first and the last empty lines

	for (const char* p = begin; p < end; p++)
	{
		uchar_t c = *p;
		if (c == '\n')
		{
			begin = p + 1;
			lineCol->m_line++;
			lineCol->m_col = 0;
			break;
		}

		if (!isspace(c))
			break;
	}

	for (const char* p = end - 1; p > begin; p--)
	{
		uchar_t c = *p;
		if (c == '\n')
		{
			end = p;
			break;
		}

		if (!isspace(c))
			break;
	}

	token = getToken();
	ASSERT(token->m_token == closeBracket);

	*string = sl::StringRef(begin, end - begin).getRightTimmedString();

	gotoState(getMachineState(LexerMachine_Main), token, GotoStateKind_EatToken);

	return true;
}

//..............................................................................
