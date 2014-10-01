// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "NodeMgr.h"
#include "Lexer.h"

//.............................................................................

// processes user code within actions & arguments
// removes unnecessary locators,
// assigns slot indexes 

class ProductionBuilder: public Lexer
{
protected:
	enum VariableKind
	{
		VariableKind_Undefined = 0,
		VariableKind_TokenBeacon,
		VariableKind_SymbolBeacon,
		VariableKind_This,
		VariableKind_Arg,
		VariableKind_Local,
	};

protected:
	NodeMgr* m_nodeMgr;
	
	SymbolNode* m_symbol;
	GrammarNode* m_production;
	DispatcherNode* m_dispatcher;
	GrammarNode* m_resolver;

	rtl::Array <ActionNode*> m_actionArray;
	rtl::Array <ArgumentNode*> m_argumentArray;
	rtl::Array <BeaconNode*> m_beaconArray;
	rtl::Array <BeaconNode*> m_beaconDeleteArray;
	rtl::StringHashTableMap <BeaconNode*> m_beaconMap;

public:
	ProductionBuilder (NodeMgr* nodeMgr);

	GrammarNode*
	build (
		SymbolNode* symbol,
		GrammarNode* production
		);

protected:
	bool
	scan (GrammarNode* node);

	bool
	addBeacon (BeaconNode* beacon);

	void
	findAndReplaceUnusedBeacons (GrammarNode*& node);

	bool
	processAllUserCode ();

	bool
	processUserCode (
		lex::SrcPos& srcPos,
		rtl::String* userCode,
		GrammarNode* resolver
		);

	VariableKind
	findVariable (
		int index,
		BeaconNode** beacon
		);

	VariableKind
	findVariable (
		const char* name,
		BeaconNode** beacon
		);
};

//.............................................................................
