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
#include "DefineMgr.h"

//..............................................................................

Define*
DefineMgr::getDefine(const sl::StringRef& name)
{
	sl::StringHashTableIterator<Define*> it = m_defineMap.visit(name);
	if (it->m_value)
		return it->m_value;

	Define* define = AXL_MEM_NEW(Define);
	define->m_name = name;
	m_defineList.insertTail(define);
	it->m_value = define;
	return define;
}

//..............................................................................
