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
#include "Parser.llk.h"
#include "Parser.llk.cpp"

//..............................................................................

Parser::RecoveryAction
Parser::processError(ErrorKind errorKind) {
	switch (errorKind) {
	case ErrorKind_Syntax:
		printf("syntax error: %s\n", err::getLastErrorDescription().sz());
		return RecoveryAction_Synchronize;

	case ErrorKind_Semantic:
		printf("semantic error: %s\n", err::getLastErrorDescription().sz());
		return RecoveryAction_Synchronize; // RecoveryAction_Continue would require more rigid checks in actions

	default:
		ASSERT(false);
		return RecoveryAction_Fail;
	}
}

Variable*
Parser::createVariable(
	const sl::String& name,
	const Value& initializer,
	bool isConst
) {
	sl::StringHashTableIterator<Variable*> it = m_variableMap.visit(name);
	if (it->m_value) {
		err::setFormatStringError("'%s': identifier redefinition", name.sz());
		return NULL;
	}

	Variable* variable = AXL_MEM_NEW(Variable);
	variable->m_name = name;
	variable->m_value = initializer;
	variable->m_isConst = isConst;
	m_variableList.insertTail(variable);
	it->m_value = variable;

	return variable;
}

bool
Parser::lookupIdentifier(
	const sl::StringRef& name,
	Value* value
) {
	sl::StringHashTableIterator<Variable*> it = m_variableMap.find(name);
	if (!it) {
		err::setFormatStringError("'%s': undeclared identifier", name.sz());
		return false;
	}

	*value = it->m_value;
	return true;
}

bool
Parser::assertionCheck(
	const Value& value,
	const Token::Pos& openPos,
	const Token::Pos& closePos
) {
	if (!value.isTrue()) {
		err::setFormatStringError(
			"Assertion failure: %s\n",
			sl::StringRef(openPos.m_p, closePos.m_p + closePos.m_length - openPos.m_p).sz()
		);
		return false;
	}

	return true;
}

//..............................................................................
