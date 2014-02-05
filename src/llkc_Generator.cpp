#include "pch.h"
#include "llkc_Generator.h"
#include "llkc_Module.h"

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
	if (m_pConfig)
	{
		FrameFilePath = io::FindFilePath (pFrameFileName, NULL, &m_pConfig->m_FrameDirList);
		if (FrameFilePath.IsEmpty ())
		{
			err::SetFormatStringError ("frame file '%s' not found", pFrameFileName);
			return false;
		}

		pFrameFileName = FrameFilePath;
	}

	bool Result;

	io::CMappedFile FrameFile;

	Result = FrameFile.Open (pFrameFileName, io::EFileFlag_ReadOnly);
	if (!Result)
		return false;

	size_t Size = (size_t) FrameFile.GetSize ();
	char* p = (char*) FrameFile.View (0, Size);
	if (!p)
		return false;

	m_Buffer.Reserve (Size);

	rtl::CString TargetFilePath = io::GetFullFilePath (pFileName);

	m_StringTemplate.m_LuaState.SetGlobalString ("TargetFilePath", TargetFilePath);
	m_StringTemplate.m_LuaState.SetGlobalBoolean ("NoPpLine", (m_pConfig->m_Flags & EConfigFlag_NoPpLine) != 0);

	Result = m_StringTemplate.Process (&m_Buffer, pFrameFileName, p, Size);
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

bool
CGenerator::GenerateList (rtl::CIteratorT <CTarget> Target)
{
	bool Result;

	for (; Target; Target++)
	{
		Result = Generate (Target->m_FileName, Target->m_FrameFileName);
		if (!Result)
			return false;
	}

	return true;
}

//.............................................................................
