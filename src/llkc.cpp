#include "pch.h"
#include "llkc_Parser.h"
#include "llkc_Generator.h"
#include "llkc_Config.h"

//.............................................................................

enum EError
{
	EError_Success = 0,
	EError_NoGrammarFile,
	EError_InvalidSwitch,
	EError_FrameCountMismatch,
	EError_ParseFailure,
	EError_BuildFailure,
	EError_GenerateFailure,
};

//.............................................................................

void
PrintUsage ()
{
	printf (
		"Gepard LL(k) grammar compiler\n"
		"Usage:\n"
		"llkc [options] <grammar_file>\n"
		"    -?, -h, -H        print this usage and exit\n"
		"    -o <output_file>  generate <output_file> (multiple allowed)\n"
		"    -O <output_dir>   set output directory to <output_dir>\n"
		"    -f <frame_file>   use LUA frame <frame_file> (multiple allowed)\n"
		"    -F <frame_dir>    add frame file directory <frame_dir> (multiple allowed)\n"
		"    -I <import_dir>   add import file directory <import_dir> (multiple allowed)\n"
		"    -t <trace_file>   write debug information into <trace_file>\n"
		"    -l                suppress #line preprocessor directives\n"
		"    -v                verbose mode\n"
		);
}

//. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

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

	CConfig Config;

	// analyze command line

	rtl::CString SrcFileName;
	rtl::CString TraceFileName;
	rtl::CStdArrayListT <CTarget> TargetList;

	size_t OutputCount = 0;
	size_t FrameCount = 0;

	CTarget* pTarget;
	rtl::CString String;

	for (int i = 1; i < argc; i++)
	{
		if (argv [i] [0] == '-') // switch
		{
			switch (argv [i] [1])
			{
			case '?': case 'h': case 'H':
				PrintUsage ();
				return EError_Success;

			case 'o':
				pTarget = TargetList.Get (OutputCount++);
				pTarget->m_FileName = argv [i] [2] ? &argv [i] [2] : ++i < argc ? argv [i] : NULL;
				break;

			case 'O':
				Config.m_OutputDir = argv [i] [2] ? &argv [i] [2] : ++i < argc ? argv [i] : NULL;
				break;

			case 'f':
				pTarget = TargetList.Get (FrameCount++);
				pTarget->m_FrameFileName = argv [i] [2] ? &argv [i] [2] : ++i < argc ? argv [i] : NULL;
				break;

			case 'F':
				String = argv [i] [2] ? &argv [i] [2] : ++i < argc ? argv [i] : NULL;
				Config.m_FrameDirList.InsertTail (String);
				break;

			case 'I':
				String = argv [i] [2] ? &argv [i] [2] : ++i < argc ? argv [i] : NULL;
				Config.m_ImportDirList.InsertTail (String);
				break;

			case 't':
				TraceFileName = argv [i] [2] ? &argv [i] [2] : ++i < argc ? argv [i] : NULL;
				break;

			case 'v':
				Config.m_Flags |= EConfigFlag_Verbose;
				break;

			case 'l':
				Config.m_Flags |= EConfigFlag_NoPpLine;
				break;

			default:
				printf ("unknown switch '-%c'\n", argv [i] [1]);
				return EError_InvalidSwitch;
			}
		}
		else
		{
			SrcFileName = argv [i];
		}
	}

	if (SrcFileName.IsEmpty ())
	{
		PrintUsage ();
		return EError_NoGrammarFile;
	}

	if (OutputCount != FrameCount)
	{
		printf ("output-file-count vs frame-file-count mismatch\n");
		return EError_FrameCountMismatch;
	}

	rtl::CString SrcFilePath = io::GetFullFilePath (SrcFileName);
	if (SrcFilePath.IsEmpty ())
	{
		printf (
			"Cannot get full file path of '%s': %s\n",
			SrcFileName.cc (), // thanks a lot gcc
			err::GetError ()->GetDescription ().cc ()
			);
		return EError_ParseFailure;
	}

	//if (pTraceFileName)
	//	stdout = fopen (pTraceFileName, "rwt");

	CModule Module;
	CParser Parser;

	Result = Parser.ParseFile (&Module, &Config, SrcFilePath);
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

			Result = Parser.ParseFile (&Module, &Config, ImportFilePath);
			if (!Result)
			{
				printf ("%s\n", err::GetError ()->GetDescription ().cc ());
				return EError_ParseFailure;
			}

			FilePathSet.Goto (ImportFilePath);
		}
	}

	Result = Module.Build (&Config);
	if (!Result)
	{
		printf ("%s\n", err::GetError ()->GetDescription ().cc ());
		return EError_BuildFailure;
	}

	if (Config.m_Flags & EConfigFlag_Verbose)
		Module.Trace ();

	CGenerator Generator;
	Generator.Prepare (&Module);
	Generator.m_pConfig = &Config;

	if (!TargetList.IsEmpty ())
	{
		Result = Generator.GenerateList (TargetList.GetHead ());
		if (!Result)
		{
			printf ("%s\n", err::GetError ()->GetDescription ().cc ());
			return EError_GenerateFailure;
		}
	}

	return EError_Success;
}

//.............................................................................
