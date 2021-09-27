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
#include "Generator.h"
#include "Module.h"

//..............................................................................

void
Generator::prepare(Module* module) {
	m_stringTemplate.create();
	m_stringTemplate.m_luaState.setGlobalBoolean("NoPpLine", (m_cmdLine->m_flags & CmdLineFlag_NoPpLine) != 0);
	module->luaExport(&m_stringTemplate.m_luaState);
}

bool
Generator::generate(
	const sl::StringRef& fileName,
	const sl::StringRef& frameFileName
) {
	sl::String frameFilePath;
	frameFilePath = io::findFilePath(frameFileName, &m_cmdLine->m_frameDirList);
	if (frameFilePath.isEmpty()) {
		err::setFormatStringError("frame file '%s' not found", frameFileName.sz());
		return false;
	}

	bool result;

	io::MappedFile frameFile;

	result = frameFile.open(frameFilePath, io::FileFlag_ReadOnly);
	if (!result)
		return false;

	size_t size = (size_t)frameFile.getSize();
	char* p = (char*)frameFile.view(0, size);
	if (!p)
		return false;

	m_buffer.reserve(size);

	sl::String targetFilePath = io::getFullFilePath(fileName);
	sl::String frameDir = io::getDir(frameFilePath);

	m_stringTemplate.m_luaState.setGlobalString("TargetFilePath", targetFilePath);
	m_stringTemplate.m_luaState.setGlobalString("FrameFilePath", frameFilePath);
	m_stringTemplate.m_luaState.setGlobalString("FrameDir", frameDir);

	result = m_stringTemplate.process(&m_buffer, frameFilePath, sl::StringRef(p, size));
	if (!result)
		return false;

	io::File targetFile;
	result = targetFile.open(targetFilePath);
	if (!result)
		return false;

	size = m_buffer.getLength();

	result = targetFile.write(m_buffer, size) != -1;
	if (!result)
		return false;

	targetFile.setSize(size);

	return true;
}

//..............................................................................
