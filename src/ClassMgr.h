// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "DefineMgr.h"

//.............................................................................

enum ClassFlagKind
{
	ClassFlagKind_Default   = 0x01,
	ClassFlagKind_Named     = 0x02,
	ClassFlagKind_Defined   = 0x04,
	ClassFlagKind_Used      = 0x08,
	ClassFlagKind_Reachable = 0x10,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Class: public rtl::ListLink
{
public:
	int m_flags;

	lex::SrcPos m_srcPos;

	Class* m_baseClass;

	rtl::String m_name;
	rtl::String m_members;

	Class ()
	{
		m_flags = 0;
		m_baseClass = NULL;
	}

	void luaExport (lua::LuaState* luaState);
};

//.............................................................................

class ClassMgr
{
protected:
	rtl::StdList <Class> m_classList;
	rtl::StringHashTableMap <Class*> m_classMap; 

public:
	bool
	isEmpty ()
	{
		return m_classList.isEmpty ();
	}
	
	size_t
	getCount ()
	{
		return m_classList.getCount ();
	}

	rtl::Iterator <Class>
	getHead ()
	{
		return m_classList.getHead ();
	}

	void
	clear ()
	{
		m_classList.clear ();
		m_classMap.clear ();
	}

	Class*
	getClass (const rtl::String& name);

	Class*
	createUnnamedClass ();
	
	Class*
	findClass (const char* name)
	{
		rtl::StringHashTableMapIterator <Class*> it = m_classMap.find (name);
		return it ? it->m_value : NULL;
	}

	void
	deleteClass (Class* cls);

	bool
	verify ();

	void
	deleteUnusedClasses ();

	void
	deleteUnreachableClasses ();
};

//.............................................................................
