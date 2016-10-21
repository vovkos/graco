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

//..............................................................................

enum DefineKind
{
	DefineKind_String,
	DefineKind_Integer
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

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

//..............................................................................

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
	getDefine (const sl::StringRef& name);

	Define*
	findDefine (const sl::StringRef& name)
	{
		sl::StringHashTableMapIterator <Define*> it = m_defineMap.find (name);
		return it ? it->m_value : NULL;
	}
};

//..............................................................................
