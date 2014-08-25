#include "pch.h"
#include "CmdLine.h"

//.............................................................................

bool
CCmdLineParser::OnValue (const char* pValue)
{
	m_pCmdLine->m_InputFileName = pValue;
	return true;
}

bool
CCmdLineParser::OnSwitch (
	ESwitch Switch,
	const char* pValue
	)
{
	switch (Switch)
	{
	case ECmdLineSwitch_Help:
		m_pCmdLine->m_Flags |= ECmdLineFlag_Help;
		break;

	case ECmdLineSwitch_Version:
		m_pCmdLine->m_Flags |= ECmdLineFlag_Version;
		break;

	case ECmdLineSwitch_NoPpLine:
		m_pCmdLine->m_Flags |= ECmdLineFlag_NoPpLine;
		break;

	case ECmdLineSwitch_Verbose:
		m_pCmdLine->m_Flags |= ECmdLineFlag_Verbose;
		break;

	case ECmdLineSwitch_OutputFileName:
		m_pCmdLine->m_OutputFileNameList.InsertTail (pValue);
		break;

	case ECmdLineSwitch_FrameFileName:
		m_pCmdLine->m_FrameFileNameList.InsertTail (pValue);
		break;

	case ECmdLineSwitch_BnfFileName:
		m_pCmdLine->m_BnfFileName = pValue;
		break;

	case ECmdLineSwitch_TraceFileName:
		m_pCmdLine->m_TraceFileName = pValue;
		break;

	case ECmdLineSwitch_OutputDir:
		m_pCmdLine->m_OutputDir = pValue;
		break;

	case ECmdLineSwitch_FrameDir:
		m_pCmdLine->m_FrameDirList.InsertTail (pValue);
		break;

	case ECmdLineSwitch_ImportDir:
		m_pCmdLine->m_ImportDirList.InsertTail (pValue);
		break;
	}

	return true;
}

bool
CCmdLineParser::Finalize ()
{
	if (m_pCmdLine->m_OutputFileNameList.GetCount () != 
		m_pCmdLine->m_FrameFileNameList.GetCount ())
	{
		err::SetStringError ("output-file-count vs frame-file-count mismatch\n");
		return false;
	}

	return true;
}

//.............................................................................
