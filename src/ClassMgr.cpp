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

#include "pch.h"
#include "ClassMgr.h"

//..............................................................................

void
Class::luaExport (lua::LuaState* luaState)
{
	luaState->createTable (0, 2);
	luaState->setMemberString ("Name", m_name);
	if (m_baseClass)
		luaState->setMemberString ("BaseClass", m_baseClass->m_name);

	luaState->setMemberString ("Members", m_members);

	luaState->createTable (0, 3);
	luaState->setMemberString ("FilePath", m_srcPos.m_filePath);
	luaState->setMemberInteger ("Line", m_srcPos.m_line);
	luaState->setMemberInteger ("Col", m_srcPos.m_col);
	luaState->setMember ("SrcPos");

}

//..............................................................................

Class*
ClassMgr::getClass (const sl::StringRef& name)
{
	sl::StringHashTableIterator <Class*> it = m_classMap.visit (name);
	if (it->m_value)
		return it->m_value;

	Class* cls = AXL_MEM_NEW (Class);
	cls->m_flags |= ClassFlag_Named;
	cls->m_name = name;
	m_classList.insertTail (cls);
	it->m_value = cls;
	return cls;
}

Class*
ClassMgr::createUnnamedClass ()
{
	Class* cls = AXL_MEM_NEW (Class);
	cls->m_name.format ("_cls%d", m_classList.getCount () + 1);
	m_classList.insertTail (cls); // don't add to class map
	return cls;
}

void
ClassMgr::deleteClass (Class* cls)
{
	if (cls->m_flags & ClassFlag_Named)
		m_classMap.eraseKey (cls->m_name);

	m_classList.erase (cls);
}

bool
ClassMgr::verify ()
{
	sl::Iterator <Class> it = m_classList.getHead ();
	for (; it; it++)
	{
		Class* cls = *it;

		if ((cls->m_flags & ClassFlag_Used) && !(cls->m_flags & ClassFlag_Defined))
		{
			err::setFormatStringError (
				"class '%s' is not defined",
				cls->m_name.sz ()
				);
			return false;
		}
	}

	return true;
}

void
ClassMgr::deleteUnusedClasses ()
{
	sl::Iterator <Class> it = m_classList.getHead ();
	while (it)
	{
		Class* cls = *it++;
		if (!(cls->m_flags & ClassFlag_Used))
			deleteClass (cls);
	}
}

void
ClassMgr::deleteUnreachableClasses ()
{
	sl::Iterator <Class> it = m_classList.getHead ();
	while (it)
	{
		Class* cls = *it++;
		if (!(cls->m_flags & ClassFlag_Reachable))
			deleteClass (cls);
	}
}

//..............................................................................
