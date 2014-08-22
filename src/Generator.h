// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#include "Config.h"

//.............................................................................

class CTarget: public rtl::TListLink
{
public:
	rtl::CString m_FileName;
	rtl::CString m_FrameFileName;
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class CGenerator
{
protected:
	lua::CStringTemplate m_StringTemplate;
	rtl::CString m_Buffer;

public:
	const CConfig* m_pConfig;

public:
	CGenerator ()
	{
		m_pConfig = NULL;
	}

	void
	Prepare (class CModule* pModule);
	
	bool
	Generate (
		const char* pFileName,
		const char* pFrameFileName
		);

	bool
	Generate (CTarget* pTarget)
	{
		return Generate (pTarget->m_FileName, pTarget->m_FrameFileName);
	}

	bool
	GenerateList (rtl::CIteratorT <CTarget> Target);
};

//.............................................................................
