// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "DefineMgr.h"

//.............................................................................

enum ClassFlag
{
	ClassFlag_Default   = 0x01,
	ClassFlag_Named     = 0x02,
	ClassFlag_Defined   = 0x04,
	ClassFlag_Used      = 0x08,
	ClassFlag_Reachable = 0x10,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Class: public sl::ListLink
{
public:
	int m_flags;

	lex::SrcPos m_srcPos;

	Class* m_baseClass;

	sl::String m_name;
	sl::String m_members;

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
	sl::StdList <Class> m_classList;
	sl::StringHashTableMap <Class*> m_classMap; 

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

	sl::Iterator <Class>
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
	getClass (const sl::StringRef& name);

	Class*
	createUnnamedClass ();
	
	Class*
	findClass (const sl::StringRef& name)
	{
		sl::StringHashTableMapIterator <Class*> it = m_classMap.find (name);
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
