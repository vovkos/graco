#include "pch.h"
#include "DefineMgr.h"

//.............................................................................
	
Define*
DefineMgr::getDefine (const rtl::String& name)
{
	rtl::StringHashTableMapIterator <Define*> it = m_defineMap.visit (name);
	if (it->m_value)
		return it->m_value;

	Define* define = AXL_MEM_NEW (Define);
	define->m_name = name;
	m_defineList.insertTail (define);
	it->m_value = define;
	return define;
}

//.............................................................................
