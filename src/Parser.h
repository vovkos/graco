// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "Lexer.h"
#include "Module.h"
#include "CmdLine.h"

//..............................................................................

class Parser: protected Lexer
{
protected:
	class ProductionSpecifiers
	{
	public:
		Class* m_class;
		int m_symbolFlags;

	public:
		ProductionSpecifiers ()
		{
			reset ();
		}

		void reset ()
		{
			m_class = NULL;
			m_symbolFlags = 0;
		}
	};

protected:
	sl::String m_dir;
	Module* m_module;
	const CmdLine* m_cmdLine;
	ProductionSpecifiers m_defaultProductionSpecifiers;

public:
	Parser ()
	{
		m_module = NULL;
		m_cmdLine = NULL;
	}

	bool
	parse (
		Module* module,
		const CmdLine* cmdLine,
		const sl::StringRef& filePath,
		const sl::StringRef& source
		);

	bool
	parseFile (
		Module* module,
		CmdLine* cmdLine,
		const sl::StringRef& filePath
		);

protected:
	// grammar

	bool
	program ();

	bool
	lookaheadStatement ();

	bool
	importStatement ();

	bool
	declarationStatement ();

	bool
	productionSpecifiers (ProductionSpecifiers* specifiers);

	bool
	classStatement ();

	bool
	usingStatement ();

	bool
	defineStatement ();

	bool
	production (const ProductionSpecifiers* specifiers);

	Class*
	classSpecifier ();

	GrammarNode*
	alternative ();

	GrammarNode*
	sequence ();

	GrammarNode*
	quantifier ();

	GrammarNode*
	primary ();

	SymbolNode*
	resolver ();

	BeaconNode*
	beacon ();

	bool
	userCode (
		int openBracket,
		sl::String* string,
		lex::SrcPos* srcPos
		);

	bool
	userCode (
		int openBracket,
		sl::String* string,
		lex::LineCol* lineCol
		);

	bool
	customizeSymbol (SymbolNode* node);

	bool
	processLocalList (SymbolNode* node);

	bool
	processFormalArgList (SymbolNode* node);

	bool
	processActualArgList (
		ArgumentNode* node,
		const sl::StringRef& string
		);

	bool
	processSymbolEventHandler (
		SymbolNode* node,
		sl::String* string
		);

	void
	setGrammarNodeSrcPos (
		GrammarNode* node,
		const lex::LineCol& lineCol
		);

	void
	setGrammarNodeSrcPos (GrammarNode* node)
	{
		setGrammarNodeSrcPos (node, m_lastTokenPos);
	}
};

//..............................................................................
