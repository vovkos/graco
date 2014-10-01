#include "pch.h"
#include "CmdLine.h"

//.............................................................................

bool
CmdLineParser::onValue (const char* value)
{
	m_cmdLine->m_inputFileName = value;
	return true;
}

bool
CmdLineParser::onSwitch (
	SwitchKind switchKind,
	const char* value
	)
{
	switch (switchKind)
	{
	case CmdLineSwitchKind_Help:
		m_cmdLine->m_flags |= CmdLineFlagKind_Help;
		break;

	case CmdLineSwitchKind_Version:
		m_cmdLine->m_flags |= CmdLineFlagKind_Version;
		break;

	case CmdLineSwitchKind_NoPpLine:
		m_cmdLine->m_flags |= CmdLineFlagKind_NoPpLine;
		break;

	case CmdLineSwitchKind_Verbose:
		m_cmdLine->m_flags |= CmdLineFlagKind_Verbose;
		break;

	case CmdLineSwitchKind_OutputFileName:
		m_cmdLine->m_outputFileNameList.insertTail (value);
		break;

	case CmdLineSwitchKind_FrameFileName:
		m_cmdLine->m_frameFileNameList.insertTail (value);
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
		m_cmdLine->m_frameDirList.insertTail (value);
		break;

	case CmdLineSwitchKind_ImportDir:
		m_cmdLine->m_importDirList.insertTail (value);
		break;
	}

	return true;
}

bool
CmdLineParser::finalize ()
{
	if (m_cmdLine->m_outputFileNameList.getCount () != 
		m_cmdLine->m_frameFileNameList.getCount ())
	{
		err::setStringError ("output-file-count vs frame-file-count mismatch\n");
		return false;
	}

	return true;
}

//.............................................................................
