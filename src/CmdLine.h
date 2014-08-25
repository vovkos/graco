#pragma once

//.............................................................................

enum ECmdLineFlag
{
	ECmdLineFlag_Help     = 0x01,
	ECmdLineFlag_Version  = 0x02,
	ECmdLineFlag_Verbose  = 0x04,
	ECmdLineFlag_NoPpLine = 0x08,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct TCmdLine
{
	uint_t m_Flags;
	rtl::CString m_InputFileName;
	rtl::CBoxListT <rtl::CString> m_OutputFileNameList;
	rtl::CBoxListT <rtl::CString> m_FrameFileNameList;
	rtl::CString m_BnfFileName;
	rtl::CString m_TraceFileName;

	rtl::CString  m_OutputDir;
	rtl::CBoxListT <rtl::CString> m_FrameDirList;
	rtl::CBoxListT <rtl::CString> m_ImportDirList;

	TCmdLine ()
	{
		m_Flags = 0;
	}
};

//.............................................................................

enum ECmdLineSwitch
{
	ECmdLineSwitch_None,
	ECmdLineSwitch_Help,
	ECmdLineSwitch_Version,
	ECmdLineSwitch_NoPpLine,
	ECmdLineSwitch_Verbose,

	ECmdLineSwitch_OutputFileName = rtl::ECmdLineSwitchFlag_HasValue,
	ECmdLineSwitch_FrameFileName,
	ECmdLineSwitch_BnfFileName,
	ECmdLineSwitch_TraceFileName,
	ECmdLineSwitch_OutputDir,
	ECmdLineSwitch_FrameDir,
	ECmdLineSwitch_ImportDir,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

AXL_RTL_BEGIN_CMD_LINE_SWITCH_TABLE (CCmdLineSwitchTable, ECmdLineSwitch)
	AXL_RTL_CMD_LINE_SWITCH_GROUP ("General options")
	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_Help,
		"h", "help", NULL,
		"Display this help"
		)
	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_Version,
		"v", "version", NULL,
		"Display Bulldozer version"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_NoPpLine,
		"l", "no-line", NULL,
		"Suppress #line preprocessor directives"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_Verbose,
		"z", "verbose", NULL,
		"Verbose mode"
		)

	AXL_RTL_CMD_LINE_SWITCH_GROUP ("Files")
	
	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_OutputFileName,
		"o", "output", "<file>",
		"Specify output file (multiple allowed)"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_FrameFileName,
		"f", "frame", "<file>",
		"Specify Lua frame file (multiple allowed)"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_BnfFileName,
		"b", "bnf", "<file>",
		"Generate \"clean\" EBNF file"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_TraceFileName,
		"t", "trace", "<file>",
		"Write verbose information into trace file"
		)

	AXL_RTL_CMD_LINE_SWITCH_GROUP ("Directories")

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_OutputDir,
		"O", "output-dir", "<dir>",
		"Specify output directory"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_FrameDir,
		"F", "frame-dir", "<dir>",
		"Add Lua frame directory (multiple allowed)"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		ECmdLineSwitch_ImportDir,
		"I", "import-dir", "<dir>",
		"Add import directory (multiple allowed)"
		)
AXL_RTL_END_CMD_LINE_SWITCH_TABLE ()

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class CCmdLineParser: public rtl::CCmdLineParserT <CCmdLineParser, CCmdLineSwitchTable>
{
	friend class rtl::CCmdLineParserT <CCmdLineParser, CCmdLineSwitchTable>;

protected:
	TCmdLine* m_pCmdLine;
	size_t m_TargetIdx;

public:
	CCmdLineParser (TCmdLine* pCmdLine)
	{
		m_pCmdLine = pCmdLine;
		m_TargetIdx = 0;
	}

protected:
	bool
	OnValue (const char* pValue);

	bool
	OnSwitch (
		ESwitch Switch,
		const char* pValue
		);

	bool
	Finalize ();
};

//.............................................................................
