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

enum CmdLineFlag
{
	CmdLineFlag_Help     = 0x01,
	CmdLineFlag_Version  = 0x02,
	CmdLineFlag_Verbose  = 0x04,
	CmdLineFlag_NoPpLine = 0x08,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

struct CmdLine
{
	uint_t m_flags;
	size_t m_lookaheadLimit;
	size_t m_conflictDepthLimit;
	sl::String m_inputFileName;
	sl::BoxList<sl::String> m_outputFileNameList;
	sl::BoxList<sl::String> m_frameFileNameList;
	sl::String m_bnfFileName;
	sl::String m_traceFileName;

	sl::String m_outputDir;
	sl::BoxList<sl::String> m_frameDirList;
	sl::BoxList<sl::String> m_importDirList;

	CmdLine();
};

//..............................................................................

enum CmdLineSwitchKind
{
	CmdLineSwitchKind_Undefined = 0,
	CmdLineSwitchKind_Help,
	CmdLineSwitchKind_Version,
	CmdLineSwitchKind_NoPpLine,
	CmdLineSwitchKind_LookaheadLimit,
	CmdLineSwitchKind_ConflictDepthLimit,
	CmdLineSwitchKind_Verbose,
	CmdLineSwitchKind_OutputFileName,
	CmdLineSwitchKind_FrameFileName,
	CmdLineSwitchKind_BnfFileName,
	CmdLineSwitchKind_TraceFileName,
	CmdLineSwitchKind_OutputDir,
	CmdLineSwitchKind_FrameDir,
	CmdLineSwitchKind_ImportDir,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

AXL_SL_BEGIN_CMD_LINE_SWITCH_TABLE(CmdLineSwitchTable, CmdLineSwitchKind)
	AXL_SL_CMD_LINE_SWITCH_GROUP("General options")
	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_Help,
		"h", "help", NULL,
		"Display this help"
		)
	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_Version,
		"v", "version", NULL,
		"Display Graco version"
		)

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_NoPpLine,
		"l", "no-line", NULL,
		"Suppress #line preprocessor directives"
		)

	AXL_SL_CMD_LINE_SWITCH(
		CmdLineSwitchKind_LookaheadLimit,
		"lookahead-limit", "<limit>",
		"Specify number of tokens used for conflict resolution (defaults to 2)"
		)

	AXL_SL_CMD_LINE_SWITCH(
		CmdLineSwitchKind_ConflictDepthLimit,
		"conflict-depth-limit", "<limit>",
		"Limit the depth of nested conflicts (defaults to 2)"
		)

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_Verbose,
		"z", "verbose", NULL,
		"Verbose mode"
		)

	AXL_SL_CMD_LINE_SWITCH_GROUP("Files")

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_OutputFileName,
		"o", "output", "<file>",
		"Specify output file (multiple allowed)"
		)

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_FrameFileName,
		"f", "frame", "<file>",
		"Specify Lua frame file (multiple allowed)"
		)

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_BnfFileName,
		"b", "bnf", "<file>",
		"Generate \"clean\" EBNF file"
		)

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_TraceFileName,
		"t", "trace", "<file>",
		"Write verbose information into trace file"
		)

	AXL_SL_CMD_LINE_SWITCH_GROUP("Directories")

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_OutputDir,
		"O", "output-dir", "<dir>",
		"Specify output directory"
		)

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_FrameDir,
		"F", "frame-dir", "<dir>",
		"Add Lua frame directory (multiple allowed)"
		)

	AXL_SL_CMD_LINE_SWITCH_2(
		CmdLineSwitchKind_ImportDir,
		"I", "import-dir", "<dir>",
		"Add import directory (multiple allowed)"
		)
AXL_SL_END_CMD_LINE_SWITCH_TABLE()

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

class CmdLineParser: public sl::CmdLineParser<CmdLineParser, CmdLineSwitchTable>
{
	friend class sl::CmdLineParser<CmdLineParser, CmdLineSwitchTable>;

protected:
	CmdLine* m_cmdLine;
	size_t m_targetIdx;

public:
	CmdLineParser(CmdLine* cmdLine)
	{
		m_cmdLine = cmdLine;
		m_targetIdx = 0;
	}

protected:
	bool
	onValue(const sl::StringRef& value);

	bool
	onSwitch(
		SwitchKind switchKind,
		const sl::StringRef& value
		);

	bool
	finalize();
};

//..............................................................................
