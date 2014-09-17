#include "pch.h"
#include "Generator.h"
#include "Module.h"

//.............................................................................

void
CGenerator::Prepare (CModule* pModule)
{
	pModule->Export (&m_StringTemplate.m_LuaState);
}

bool
CGenerator::Generate (
	const char* pFileName,
	const char* pFrameFileName
	)
{
	rtl::CString FrameFilePath;
	if (m_pCmdLine)
	{
		FrameFilePath = io::FindFilePath (pFrameFileName, NULL, &m_pCmdLine->m_FrameDirList);
		if (FrameFilePath.IsEmpty ())
		{
			err::SetFormatStringError ("frame file '%s' not found", pFrameFileName);
			return false;
		}
	}

	bool Result;

	io::CMappedFile FrameFile;

	Result = FrameFile.Open (FrameFilePath, io::EFileFlag_ReadOnly);
	if (!Result)
		return false;

	size_t Size = (size_t) FrameFile.GetSize ();
	char* p = (char*) FrameFile.View (0, Size);
	if (!p)
		return false;

	m_Buffer.Reserve (Size);

	rtl::CString TargetFilePath = io::GetFullFilePath (pFileName);
	rtl::CString FrameDir = io::GetDir (FrameFilePath);

	m_StringTemplate.m_LuaState.SetGlobalString ("TargetFilePath", TargetFilePath);
	m_StringTemplate.m_LuaState.SetGlobalString ("FrameFilePath", FrameFilePath);
	m_StringTemplate.m_LuaState.SetGlobalString ("FrameDir", FrameDir);
	m_StringTemplate.m_LuaState.SetGlobalBoolean ("NoPpLine", (m_pCmdLine->m_Flags & ECmdLineFlag_NoPpLine) != 0);
	
	Result = m_StringTemplate.Process (&m_Buffer, FrameFilePath, p, Size);
	if (!Result)
		return false;

	io::CFile TargetFile;
	Result = TargetFile.Open (TargetFilePath);
	if (!Result)
		return false;

	Size = m_Buffer.GetLength ();

	Result = TargetFile.Write (m_Buffer, Size) != -1;
	if (!Result)
		return false;

	TargetFile.SetSize (Size);

	return true;
}

//.............................................................................
