// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

struct TCmdLine;

//.............................................................................

class CGenerator
{
protected:
	lua::CStringTemplate m_StringTemplate;
	rtl::CString m_Buffer;

public:
	const TCmdLine* m_pCmdLine;

public:
	CGenerator ()
	{
		m_pCmdLine = NULL;
	}

	void
	Prepare (class CModule* pModule);
	
	bool
	Generate (
		const char* pFileName,
		const char* pFrameFileName
		);
};

//.............................................................................
