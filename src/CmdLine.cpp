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
#include "CmdLine.h"

//..............................................................................

CmdLine::CmdLine()
{
	m_flags = 0;
	m_lookaheadLimit = 2;
	m_conflictDepthLimit = 4;
}

//..............................................................................

bool
CmdLineParser::onValue(const sl::StringRef& value)
{
	m_cmdLine->m_inputFileName = value;
	return true;
}

bool
CmdLineParser::onSwitch(
	SwitchKind switchKind,
	const sl::StringRef& value
	)
{
	switch(switchKind)
	{
	case CmdLineSwitchKind_Help:
		m_cmdLine->m_flags |= CmdLineFlag_Help;
		break;

	case CmdLineSwitchKind_Version:
		m_cmdLine->m_flags |= CmdLineFlag_Version;
		break;

	case CmdLineSwitchKind_NoPpLine:
		m_cmdLine->m_flags |= CmdLineFlag_NoPpLine;
		break;

	case CmdLineSwitchKind_Verbose:
		m_cmdLine->m_flags |= CmdLineFlag_Verbose;
		break;

	case CmdLineSwitchKind_LookaheadLimit:
		m_cmdLine->m_lookaheadLimit = atoi(value.sz());
		break;

	case CmdLineSwitchKind_ConflictDepthLimit:
		m_cmdLine->m_conflictDepthLimit = atoi(value.sz());
		break;

	case CmdLineSwitchKind_OutputFileName:
		m_cmdLine->m_outputFileNameList.insertTail(value);
		break;

	case CmdLineSwitchKind_FrameFileName:
		m_cmdLine->m_frameFileNameList.insertTail(value);
		break;

	case CmdLineSwitchKind_BnfFileName:
		m_cmdLine->m_bnfFileName = value;
		break;

	case CmdLineSwitchKind_TraceFileName:
		m_cmdLine->m_traceFileName = value;
		break;

	case CmdLineSwitchKind_OutputDir:
		m_cmdLine->m_outputDir = value;
		break;

	case CmdLineSwitchKind_FrameDir:
		m_cmdLine->m_frameDirList.insertTail(value);
		break;

	case CmdLineSwitchKind_ImportDir:
		m_cmdLine->m_importDirList.insertTail(value);
		break;

	default:
		ASSERT(false);
	}

	return true;
}

bool
CmdLineParser::finalize()
{
	if (m_cmdLine->m_outputFileNameList.getCount() !=
		m_cmdLine->m_frameFileNameList.getCount())
	{
		err::setError("output-file-count vs frame-file-count mismatch\n");
		return false;
	}

	return true;
}

//..............................................................................
