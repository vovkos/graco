#pragma once

//.............................................................................

enum CmdLineFlagKind
{
	CmdLineFlagKind_Help     = 0x01,
	CmdLineFlagKind_Version  = 0x02,
	CmdLineFlagKind_Verbose  = 0x04,
	CmdLineFlagKind_NoPpLine = 0x08,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct CmdLine
{
	uint_t m_flags;
	rtl::String m_inputFileName;
	rtl::BoxList <rtl::String> m_outputFileNameList;
	rtl::BoxList <rtl::String> m_frameFileNameList;
	rtl::String m_bnfFileName;
	rtl::String m_traceFileName;

	rtl::String  m_outputDir;
	rtl::BoxList <rtl::String> m_frameDirList;
	rtl::BoxList <rtl::String> m_importDirList;

	CmdLine ()
	{
		m_flags = 0;
	}
};

//.............................................................................

enum CmdLineSwitchKind
{
	CmdLineSwitchKind_None,
	CmdLineSwitchKind_Help,
	CmdLineSwitchKind_Version,
	CmdLineSwitchKind_NoPpLine,
	CmdLineSwitchKind_Verbose,

	CmdLineSwitchKind_OutputFileName = rtl::CmdLineSwitchFlagKind_HasValue,
	CmdLineSwitchKind_FrameFileName,
	CmdLineSwitchKind_BnfFileName,
	CmdLineSwitchKind_TraceFileName,
	CmdLineSwitchKind_OutputDir,
	CmdLineSwitchKind_FrameDir,
	CmdLineSwitchKind_ImportDir,
};

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

AXL_RTL_BEGIN_CMD_LINE_SWITCH_TABLE (CmdLineSwitchTable, CmdLineSwitchKind)
	AXL_RTL_CMD_LINE_SWITCH_GROUP ("General options")
	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_Help,
		"h", "help", NULL,
		"Display this help"
		)
	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_Version,
		"v", "version", NULL,
		"Display Bulldozer version"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_NoPpLine,
		"l", "no-line", NULL,
		"Suppress #line preprocessor directives"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_Verbose,
		"z", "verbose", NULL,
		"Verbose mode"
		)

	AXL_RTL_CMD_LINE_SWITCH_GROUP ("Files")
	
	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_OutputFileName,
		"o", "output", "<file>",
		"Specify output file (multiple allowed)"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_FrameFileName,
		"f", "frame", "<file>",
		"Specify Lua frame file (multiple allowed)"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_BnfFileName,
		"b", "bnf", "<file>",
		"Generate \"clean\" EBNF file"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_TraceFileName,
		"t", "trace", "<file>",
		"Write verbose information into trace file"
		)

	AXL_RTL_CMD_LINE_SWITCH_GROUP ("Directories")

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_OutputDir,
		"O", "output-dir", "<dir>",
		"Specify output directory"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_FrameDir,
		"F", "frame-dir", "<dir>",
		"Add Lua frame directory (multiple allowed)"
		)

	AXL_RTL_CMD_LINE_SWITCH_2 (
		CmdLineSwitchKind_ImportDir,
		"I", "import-dir", "<dir>",
		"Add import directory (multiple allowed)"
		)
AXL_RTL_END_CMD_LINE_SWITCH_TABLE ()

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class CmdLineParser: public rtl::CmdLineParser <CmdLineParser, CmdLineSwitchTable>
{
	friend class rtl::CmdLineParser <CmdLineParser, CmdLineSwitchTable>;

protected:
	CmdLine* m_cmdLine;
	size_t m_targetIdx;

public:
	CmdLineParser (CmdLine* cmdLine)
	{
		m_cmdLine = cmdLine;
		m_targetIdx = 0;
	}

protected:
	bool
	onValue (const char* value);

	bool
	onSwitch (
		SwitchKind switchKind,
		const char* value
		);

	bool
	finalize ();
};

//.............................................................................
