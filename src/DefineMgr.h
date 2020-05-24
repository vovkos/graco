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
	DefineKind_Integer,
	DefineKind_Bool,
	DefineKind_Default = DefineKind_String,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class Define: public sl::ListLink
{
public:
	DefineKind m_defineKind;
	lex::SrcPos m_srcPos;
	sl::StringRef m_name;
	sl::StringRef m_stringValue;
	int m_integerValue;

	Define()
	{
		m_defineKind = DefineKind_Default;
		m_integerValue = 0;
	}
};

//..............................................................................

class DefineMgr
{
protected:
	sl::List<Define> m_defineList;
	sl::StringHashTable<Define*> m_defineMap;

public:
	bool
	isEmpty()
	{
		return m_defineList.isEmpty();
	}

	size_t
	getCount()
	{
		return m_defineList.getCount();
	}

	void
	clear()
	{
		m_defineList.clear();
		m_defineMap.clear();
	}

	void
	luaExport(lua::LuaState* luaState);

	sl::Iterator<Define>
	getHead()
	{
		return m_defineList.getHead();
	}

	Define*
	getDefine(const sl::StringRef& name);

	Define*
	findDefine(const sl::StringRef& name)
	{
		sl::StringHashTableIterator<Define*> it = m_defineMap.find(name);
		return it ? it->m_value : NULL;
	}
};

//..............................................................................
