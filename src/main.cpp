#include "pch.h"
#include "Parser.h"
#include "Generator.h"
#include "CmdLine.h"
#include "Version.h"

//.............................................................................

enum ErrorCode
{
	ErrorCode_Success = 0,
	ErrorCode_InvalidCmdLine,
	ErrorCode_ParseFailure,
	ErrorCode_BuildFailure,
	ErrorCode_GenerateFailure,
};

//.............................................................................

void
printVersion ()
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
printUsage ()
{
	printVersion ();

	rtl::String helpString = CmdLineSwitchTable::getHelpString ();
	printf ("Usage: bulldozer [<options>...] <source_file>\n%s", helpString.cc ());
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
	bool result;

	err::registerParseErrorProvider ();

	CmdLine cmdLine;
	CmdLineParser cmdLineParser (&cmdLine);
	result = cmdLineParser.parse (argc, argv);
	if (!result)
		return false;

	if (cmdLine.m_inputFileName.isEmpty ())
	{
		printUsage ();
		return ErrorCode_Success;
	}

	rtl::String srcFilePath = io::getFullFilePath (cmdLine.m_inputFileName);
	if (srcFilePath.isEmpty ())
	{
		printf (
			"Cannot get full file path of '%s': %s\n",
			cmdLine.m_inputFileName.cc (), // thanks a lot gcc
			err::getError ()->getDescription ().cc ()
			);
		return ErrorCode_ParseFailure;
	}

	//if (pTraceFileName)
	//	stdout = fopen (pTraceFileName, "rwt");

	Module module;
	Parser parser;

	result = parser.parseFile (&module, &cmdLine, srcFilePath);
	if (!result)
	{
		printf ("%s\n", err::getError ()->getDescription ().cc ());
		return ErrorCode_ParseFailure;
	}

	if (!module.m_importList.isEmpty ())
	{
		rtl::StringHashTable filePathSet;
		filePathSet.visit (srcFilePath);

		rtl::BoxIterator <rtl::String> import = module.m_importList.getHead ();
		for (; import; import++)
		{
			rtl::String importFilePath = *import;
			if (filePathSet.find (importFilePath))
				continue;

			result = parser.parseFile (&module, &cmdLine, importFilePath);
			if (!result)
			{
				printf ("%s\n", err::getError ()->getDescription ().cc ());
				return ErrorCode_ParseFailure;
			}

			filePathSet.visit (importFilePath);
		}
	}

	if (!cmdLine.m_bnfFileName.isEmpty ())
	{
		result = module.writeBnfFile (cmdLine.m_bnfFileName);
		if (!result)
		{
			printf ("%s\n", err::getError ()->getDescription ().cc ());
			return ErrorCode_BuildFailure;
		}
	}

	result = module.build (&cmdLine);
	if (!result)
	{
		printf ("%s\n", err::getError ()->getDescription ().cc ());
		return ErrorCode_BuildFailure;
	}

	if (cmdLine.m_flags & CmdLineFlag_Verbose)
		module.trace ();

	Generator generator;
	generator.prepare (&module);
	generator.m_cmdLine = &cmdLine;

	ASSERT (cmdLine.m_outputFileNameList.getCount () == cmdLine.m_frameFileNameList.getCount ());
	rtl::BoxIterator <rtl::String> outputFileNameIt = cmdLine.m_outputFileNameList.getHead ();
	rtl::BoxIterator <rtl::String> frameFileNameIt = cmdLine.m_frameFileNameList.getHead ();

	for (; outputFileNameIt && frameFileNameIt; outputFileNameIt++, frameFileNameIt++)
	{
		result = generator.generate (*outputFileNameIt, *frameFileNameIt);
		if (!result)
		{
			printf ("%s\n", err::getError ()->getDescription ().cc ());
			return ErrorCode_GenerateFailure;
		}
	}

	return ErrorCode_Success;
}

//.............................................................................
