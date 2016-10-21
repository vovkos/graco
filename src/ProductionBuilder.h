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

#include "NodeMgr.h"
#include "Lexer.h"

//..............................................................................

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

	sl::Array <ActionNode*> m_actionArray;
	sl::Array <ArgumentNode*> m_argumentArray;
	sl::Array <BeaconNode*> m_beaconArray;
	sl::Array <BeaconNode*> m_beaconDeleteArray;
	sl::StringHashTableMap <BeaconNode*> m_beaconMap;

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
		sl::String* userCode,
		GrammarNode* resolver
		);

	VariableKind
	findVariable (
		int index,
		BeaconNode** beacon
		);

	VariableKind
	findVariable (
		const sl::StringRef& name,
		BeaconNode** beacon
		);
};

//..............................................................................
