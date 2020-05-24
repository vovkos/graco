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

#pragma once

#include "Lexer.h"
#include "Module.h"
#include "CmdLine.h"

//..............................................................................

class Parser: protected Lexer
{
protected:
	struct ProductionSpecifiers
	{
		sl::StringRef m_valueBlock;
		lex::LineCol m_valueLineCol;
		uint_t m_flags;

		ProductionSpecifiers()
		{
			m_flags = 0;
		}
	};

protected:
	const CmdLine* m_cmdLine;
	Module* m_module;
	sl::String m_dir;

public:
	Parser(
		const CmdLine* cmdLine,
		Module* module
		)
	{
		m_cmdLine = cmdLine;
		m_module = module;
	}

	bool
	parse(
		const sl::StringRef& filePath,
		const sl::StringRef& source
		);

	bool
	parseFile(const sl::StringRef& filePath);

protected:
	// grammar

	bool
	program();

	bool
	lookaheadStatement();

	bool
	importStatement();

	bool
	declarationStatement();

	bool
	productionSpecifiers(ProductionSpecifiers* specifiers);

	bool
	sizeSpecifier(
		TokenKind tokenKind,
		size_t* size
		);

	bool
	nodeSpecifier(
		TokenKind tokenKind,
		GrammarNode** node
		);

	bool
	defineStatement();

	bool
	production(const ProductionSpecifiers* specifiers);

	GrammarNode*
	alternative();

	GrammarNode*
	sequence();

	GrammarNode*
	quantifier();

	GrammarNode*
	primary();

	SymbolNode*
	catcher();

	GrammarNode*
	lookahead();

	SymbolNode*
	resolver();

	BeaconNode*
	beacon();

	bool
	userCode(
		int openBracket,
		sl::StringRef* string,
		lex::SrcPos* srcPos
		);

	bool
	userCode(
		int openBracket,
		sl::StringRef* string,
		lex::LineCol* lineCol
		);

	bool
	customizeSymbol(SymbolNode* node);

	bool
	processParamBlock(SymbolNode* node);

	bool
	processLocalBlock(SymbolNode* node);

	bool
	processEnterLeaveBlock(
		SymbolNode* node,
		sl::StringRef* string
		);

	bool
	processActualArgList(
		ArgumentNode* node,
		const sl::StringRef& string
		);

	void
	setGrammarNodeSrcPos(
		GrammarNode* node,
		const lex::LineCol& lineCol
		);

	void
	setGrammarNodeSrcPos(GrammarNode* node)
	{
		setGrammarNodeSrcPos(node, m_lastTokenPos);
	}
};

//..............................................................................
