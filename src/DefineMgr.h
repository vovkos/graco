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

class Define: public rtl::ListLink
{
public:
	DefineKind m_kind;
	lex::SrcPos m_srcPos;
	rtl::String m_name;
	rtl::String m_stringValue;
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
	rtl::StdList <Define> m_defineList;
	rtl::StringHashTableMap <Define*> m_defineMap;

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

	rtl::Iterator <Define>
	getHead ()
	{
		return m_defineList.getHead ();
	}

	Define*
	getDefine (const rtl::String& name);

	Define*
	findDefine (const char* name)
	{
		rtl::StringHashTableMapIterator <Define*> it = m_defineMap.find (name);
		return it ? it->m_value : NULL;
	}
};

//.............................................................................
