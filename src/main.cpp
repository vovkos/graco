#include "pch.h"
#include "Parser.h"
#include "Generator.h"
#include "CmdLine.h"
#include "Version.h"

//.............................................................................

enum EError
{
	EError_Success = 0,
	EError_InvalidCmdLine,
	EError_ParseFailure,
	EError_BuildFailure,
	EError_GenerateFailure,
};

//.............................................................................

void
PrintVersion ()
{
	printf (
		"Bulldozer (%s) v%d.%d.%d\n",
		_AXL_CPU_STRING,
		VERSION_MAJOR,
		VERSION_MINOR,
		VERSION_REVISION
		);
}

void
PrintUsage ()
{
	PrintVersion ();

	rtl::CString HelpString = CCmdLineSwitchTable::GetHelpString ();
	printf ("Usage: bulldozer [<options>...] <source_file>\n%s", HelpString.cc ());
}

//.............................................................................

#if (_AXL_ENV == AXL_ENV_WIN)
int
wmain (
	int argc,
	wchar_t* argv []
	)
#else
int
main (
	int argc,
	char* argv []
	)
#endif
{
	bool Result;

	err::CParseErrorProvider::Register ();

	TCmdLine CmdLine;
	CCmdLineParser CmdLineParser (&CmdLine);
	Result = CmdLineParser.Parse (argc, argv);
	if (!Result)
		return false;

	if (CmdLine.m_InputFileName.IsEmpty ())
	{
		PrintUsage ();
		return EError_Success;
	}

	rtl::CString SrcFilePath = io::GetFullFilePath (CmdLine.m_InputFileName);
	if (SrcFilePath.IsEmpty ())
	{
		printf (
			"Cannot get full file path of '%s': %s\n",
			CmdLine.m_InputFileName.cc (), // thanks a lot gcc
			err::GetError ()->GetDescription ().cc ()
			);
		return EError_ParseFailure;
	}

	//if (pTraceFileName)
	//	stdout = fopen (pTraceFileName, "rwt");

	CModule Module;
	CParser Parser;

	Result = Parser.ParseFile (&Module, &CmdLine, SrcFilePath);
	if (!Result)
	{
		printf ("%s\n", err::GetError ()->GetDescription ().cc ());
		return EError_ParseFailure;
	}

	if (!Module.m_ImportList.IsEmpty ())
	{
		rtl::CStringHashTable FilePathSet;
		FilePathSet.Goto (SrcFilePath);

		rtl::CBoxIteratorT <rtl::CString> Import = Module.m_ImportList.GetHead ();
		for (; Import; Import++)
		{
			rtl::CString ImportFilePath = *Import;
			if (FilePathSet.Find (ImportFilePath))
				continue;

			Result = Parser.ParseFile (&Module, &CmdLine, ImportFilePath);
			if (!Result)
			{
				printf ("%s\n", err::GetError ()->GetDescription ().cc ());
				return EError_ParseFailure;
			}

			FilePathSet.Goto (ImportFilePath);
		}
	}

	if (!CmdLine.m_BnfFileName.IsEmpty ())
	{
		Result = Module.WriteBnfFile (CmdLine.m_BnfFileName);
		if (!Result)
		{
			printf ("%s\n", err::GetError ()->GetDescription ().cc ());
			return EError_BuildFailure;
		}
	}

	Result = Module.Build (&CmdLine);
	if (!Result)
	{
		printf ("%s\n", err::GetError ()->GetDescription ().cc ());
		return EError_BuildFailure;
	}

	if (CmdLine.m_Flags & ECmdLineFlag_Verbose)
		Module.Trace ();

	CGenerator Generator;
	Generator.Prepare (&Module);
	Generator.m_pCmdLine = &CmdLine;

	ASSERT (CmdLine.m_OutputFileNameList.GetCount () == CmdLine.m_FrameFileNameList.GetCount ());
	rtl::CBoxIteratorT <rtl::CString> OutputFileNameIt = CmdLine.m_OutputFileNameList.GetHead ();
	rtl::CBoxIteratorT <rtl::CString> FrameFileNameIt = CmdLine.m_FrameFileNameList.GetHead ();

	for (; OutputFileNameIt && FrameFileNameIt; OutputFileNameIt++, FrameFileNameIt++)
	{
		Result = Generator.Generate (*OutputFileNameIt, *FrameFileNameIt);
		if (!Result)
		{
			printf ("%s\n", err::GetError ()->GetDescription ().cc ());
			return EError_GenerateFailure;
		}
	}

	return EError_Success;
}

//.............................................................................
