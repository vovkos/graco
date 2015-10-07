// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

//.............................................................................

enum DefineKind
{
	DefineKind_String,
	DefineKind_Integer
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Define: public sl::ListLink
{
public:
	DefineKind m_kind;
	lex::SrcPos m_srcPos;
	sl::String m_name;
	sl::String m_stringValue;
	int m_integerValue;

	Define ()
	{
		m_kind = DefineKind_String;
		m_integerValue = 0;
	}
};

//.............................................................................

class DefineMgr
{
protected:
	sl::StdList <Define> m_defineList;
	sl::StringHashTableMap <Define*> m_defineMap;

public:
	bool
	isEmpty ()
	{
		return m_defineList.isEmpty ();
	}

	size_t
	getCount ()
	{
		return m_defineList.getCount ();
	}

	void
	clear ()
	{
		m_defineList.clear ();
		m_defineMap.clear ();
	}

	sl::Iterator <Define>
	getHead ()
	{
		return m_defineList.getHead ();
	}

	Define*
	getDefine (const sl::String& name);

	Define*
	findDefine (const char* name)
	{
		sl::StringHashTableMapIterator <Define*> it = m_defineMap.find (name);
		return it ? it->m_value : NULL;
	}
};

//.............................................................................
