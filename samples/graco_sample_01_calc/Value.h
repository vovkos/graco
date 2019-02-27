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

#pragma once

struct Variable;

//..............................................................................

enum Type
{
	Type_Null,
	Type_Int,
	Type_Fp,
	Type__Count,
};

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum UnOpKind
{
	UnOpKind_Minus,
	UnOpKind_BitwiseNot,
	UnOpKind__Count
};

const char*
getUnOpKindString(UnOpKind opKind);

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum BinOpKind
{
	BinOpKind_Add,
	BinOpKind_Sub,
	BinOpKind_Mul,
	BinOpKind_Div,
	BinOpKind_Mod,
	BinOpKind_Shl,
	BinOpKind_Shr,
	BinOpKind_And,
	BinOpKind_Xor,
	BinOpKind_Or,
	BinOpKind__Count
};

const char*
getBinOpKindString(BinOpKind opKind);

// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

enum RelOpKind
{
	RelOpKind_Eq,
	RelOpKind_Ne,
	RelOpKind_Lt,
	RelOpKind_Gt,
	RelOpKind_Le,
	RelOpKind_Ge,
	RelOpKind__Count
};

const char*
getRelOpKindString(RelOpKind opKind);

//..............................................................................

struct Value
{
	Type m_type;
	Variable* m_variable;

	union
	{
		int m_integer;
		double m_fp;
	};

	Value();
	Value(int integer);
	Value(double fp);
	Value(Variable* variable);

	bool
	lvalueCheck() const;

	bool
	isTrue() const;

	double
	getFp() const
	{
		return m_type == Type_Int ? (double)m_integer : m_fp;
	}

	sl::String
	getString() const;

	bool
	unaryOperator(UnOpKind opKind);

	bool
	binaryOperator(
		BinOpKind opKind,
		const Value& value
		);

	bool
	relationalOperator(
		RelOpKind opKind,
		const Value& value
		);
};

//..............................................................................

struct Variable: sl::ListLink
{
	sl::String m_name;
	Value m_value;
	bool m_isConst;

	Variable()
	{
		m_isConst = false;
	}
};

//..............................................................................
