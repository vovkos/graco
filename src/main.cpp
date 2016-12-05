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
#include "Parser.h"
#include "Generator.h"
#include "CmdLine.h"
#include "version.h"

//..............................................................................

enum ErrorCode
{
	ErrorCode_Success = 0,
	ErrorCode_InvalidCmdLine,
	ErrorCode_ParseFailure,
	ErrorCode_BuildFailure,
	ErrorCode_GenerateFailure,
};

//..............................................................................

void
printVersion ()
{
	printf (
		"Graco Grammar Compiler v%d.%d.%d (%s%s)\n",
		VERSION_MAJOR,
		VERSION_MINOR,
		VERSION_REVISION,
		AXL_CPU_STRING,
		AXL_DEBUG_SUFFIX
		);
}

void
printUsage ()
{
	printVersion ();

	sl::String helpString = CmdLineSwitchTable::getHelpString ();
	printf ("Usage: graco [<options>...] <source_file>\n%s", helpString.sz ());
}

//..............................................................................

#if (_AXL_OS_WIN)
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

	g::getModule ()->setTag ("graco");
	lex::registerParseErrorProvider ();

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

	sl::String srcFilePath = io::getFullFilePath (cmdLine.m_inputFileName);
	if (srcFilePath.isEmpty ())
	{
		printf (
			"Cannot get full file path of '%s': %s\n",
			cmdLine.m_inputFileName.sz (),
			err::getLastErrorDescription ().sz ()
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
		printf ("%s\n", err::getLastErrorDescription ().sz ());
		return ErrorCode_ParseFailure;
	}

	if (!module.m_importList.isEmpty ())
	{
		sl::StringHashTable filePathSet;
		filePathSet.visit (srcFilePath);

		sl::BoxIterator <sl::String> import = module.m_importList.getHead ();
		for (; import; import++)
		{
			sl::String importFilePath = *import;
			if (filePathSet.find (importFilePath))
				continue;

			result = parser.parseFile (&module, &cmdLine, importFilePath);
			if (!result)
			{
				printf ("%s\n", err::getLastErrorDescription ().sz ());
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
			printf ("%s\n", err::getLastErrorDescription ().sz ());
			return ErrorCode_BuildFailure;
		}
	}

	result = module.build (&cmdLine);
	if (!result)
	{
		printf ("%s\n", err::getLastErrorDescription ().sz ());
		return ErrorCode_BuildFailure;
	}

	if (cmdLine.m_flags & CmdLineFlag_Verbose)
		module.trace ();

	Generator generator;
	generator.prepare (&module);
	generator.m_cmdLine = &cmdLine;

	ASSERT (cmdLine.m_outputFileNameList.getCount () == cmdLine.m_frameFileNameList.getCount ());
	sl::BoxIterator <sl::String> outputFileNameIt = cmdLine.m_outputFileNameList.getHead ();
	sl::BoxIterator <sl::String> frameFileNameIt = cmdLine.m_frameFileNameList.getHead ();

	for (; outputFileNameIt && frameFileNameIt; outputFileNameIt++, frameFileNameIt++)
	{
		result = generator.generate (*outputFileNameIt, *frameFileNameIt);
		if (!result)
		{
			printf ("%s\n", err::getLastErrorDescription ().sz ());
			return ErrorCode_GenerateFailure;
		}
	}

	return ErrorCode_Success;
}

//..............................................................................
