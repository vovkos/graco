// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

struct CmdLine;

//.............................................................................

class Generator
{
protected:
	lua::StringTemplate m_stringTemplate;
	sl::String m_buffer;

public:
	const CmdLine* m_cmdLine;

public:
	Generator ()
	{
		m_cmdLine = NULL;
	}

	void
	prepare (class Module* module);
	
	bool
	generate (
		const char* fileName,
		const char* frameFileName
		);
};

//.............................................................................
