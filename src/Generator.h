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

struct CmdLine;

//..............................................................................

class Generator {
protected:
	const CmdLine* m_cmdLine;
	st::LuaStringTemplate m_stringTemplate;
	sl::String m_buffer;

public:
	Generator(const CmdLine* cmdLine) {
		m_cmdLine = cmdLine;
	}

	void
	prepare(class Module* module);

	bool
	generate(
		const sl::StringRef& fileName,
		const sl::StringRef& frameFileName
	);
};

//..............................................................................
