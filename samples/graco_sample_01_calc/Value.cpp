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
#include "Value.h"

//..............................................................................

const char*
getUnOpKindString (UnOpKind opKind)
{
	const char* stringTable [UnOpKind__Count] =
	{
		"-", // UnOpKind_Minus,
		"~", // UnOpKind_BitwiseNot,
	};

	ASSERT ((size_t) opKind < countof (stringTable));
	return stringTable [(size_t) opKind];
}

const char*
getBinOpKindString (BinOpKind opKind)
{
	const char* stringTable [BinOpKind__Count] =
	{
		"+",  // BinOpKind_Add,
		"-",  // BinOpKind_Sub,
		"*",  // BinOpKind_Mul,
		"/",  // BinOpKind_Div,
		"%",  // BinOpKind_Mod,
		"<<", // BinOpKind_Shl,
		">>", // BinOpKind_Shr,
		"&",  // BinOpKind_And,
		"^",  // BinOpKind_Xor,
		"|",  // BinOpKind_Or,
	};

	ASSERT ((size_t) opKind < countof (stringTable));
	return stringTable [(size_t) opKind];
}

const char*
getRelOpKindString (RelOpKind opKind)
{
	const char* stringTable [RelOpKind__Count] =
	{
		"==", // RelOpKind_Eq,
		"!=", // RelOpKind_Ne,
		"<",  // RelOpKind_Lt,
		">",  // RelOpKind_Gt,
		"<=", // RelOpKind_Le,
		">=", // RelOpKind_Ge,
	};

	ASSERT ((size_t) opKind < countof (stringTable));
	return stringTable [(size_t) opKind];
}

//..............................................................................

template <
	typename Type,
	typename Functor
	>
Type
unOpFunc (Type value)
{
	return Functor () (value);
}

template <
	typename Type,
	typename Functor
	>
Type
binOpFunc (
	Type value1,
	Type value2
	)
{
	return Functor () (value1, value2);
}

template <
	typename Type,
	typename Functor
	>
bool
relOpFunc (
	Type value1,
	Type value2
	)
{
	return Functor () (value1, value2);
}

//..............................................................................

Value::Value ()
{
	m_type = Type_Null;
	m_variable = NULL;
	m_fp = 0;
}

Value::Value (int integer)
{
	m_type = Type_Int;
	m_variable = NULL;
	m_integer = integer;
}

Value::Value (double fp)
{
	m_type = Type_Fp;
	m_variable = NULL;
	m_fp = fp;
}

Value::Value (Variable* variable)
{
	m_type = variable->m_value.m_type;
	m_variable = variable;
	m_fp = variable->m_value.m_fp;
}

bool
Value::isTrue () const
{
	return
		m_type == Type_Int && m_integer != 0 ||
		m_type == Type_Fp && m_fp != 0.0;
}

bool
Value::lvalueCheck () const
{
	if (!m_variable)
	{
		err::setError ("not l-value");
		return false;
	}

	if (m_variable->m_isConst)
	{
		err::setFormatStringError ("'%s' is constant", m_variable->m_name.sz ());
		return false;
	}

	return true;
}

sl::String
Value::getString () const
{
	switch (m_type)
	{
	case Type_Null:
		return "null";

	case Type_Int:
		return sl::formatString ("%d (0x%x)", m_integer, m_integer);

	case Type_Fp:
		return sl::formatString ("%.2f (%e)", m_fp, m_fp);

	default:
		ASSERT (false);
		return "<invalid-value>";
	}
}

bool
Value::unaryOperator (UnOpKind opKind)
{
	struct Operator
	{
		int
		(*m_intFunc) (int);

		double
		(*m_fpFunc) (double);
	};

	static Operator operatorTable [UnOpKind__Count] =
	{
		{ unOpFunc <int, sl::Minus <int> >, unOpFunc <double, sl::Minus <double> > }, // UnOpKind_Minus,
		{ unOpFunc <int, sl::Not <int> >,   NULL }                                    // UnOpKind_BitwiseNot,
	};

	ASSERT (opKind < countof (operatorTable));
	Operator* op = &operatorTable [opKind];

	switch (m_type)
	{
	case Type_Null:
		err::setError ("cannot apply operators to 'null' values");
		return false;

	case Type_Int:
		m_integer = op->m_intFunc (m_integer);
		break;

	case Type_Fp:
		if (!op->m_fpFunc)
		{
			err::setFormatStringError (
				"cannot apply unary '%s' to floating-point values",
				getUnOpKindString (opKind)
				);
			return false;
		}

		m_fp = op->m_fpFunc (m_fp);
		break;

	default:
		ASSERT (false);
	}

	return true;
}

bool
Value::binaryOperator (
	BinOpKind opKind,
	const Value& value
	)
{
	struct Operator
	{
		int
		(*m_intFunc) (int, int);

		double
		(*m_fpFunc) (double, double);
	};

	static Operator operatorTable [BinOpKind__Count] =
	{
		{ binOpFunc <int, sl::Add <int> >, binOpFunc <double, sl::Add <double> > }, // BinOpKind_Add,
		{ binOpFunc <int, sl::Sub <int> >, binOpFunc <double, sl::Sub <double> > }, // BinOpKind_Sub,
		{ binOpFunc <int, sl::Mul <int> >, binOpFunc <double, sl::Mul <double> > }, // BinOpKind_Mul,
		{ binOpFunc <int, sl::Div <int> >, binOpFunc <double, sl::Div <double> > }, // BinOpKind_Div,
		{ binOpFunc <int, sl::Mod <int> >, NULL },                                  // BinOpKind_Mod,
		{ binOpFunc <int, sl::Shl <int> >, NULL },                                  // BinOpKind_Shl,
		{ binOpFunc <int, sl::Shr <int> >, NULL },                                  // BinOpKind_Shr,
		{ binOpFunc <int, sl::And <int> >, NULL },                                  // BinOpKind_And,
		{ binOpFunc <int, sl::Xor <int> >, NULL },                                  // BinOpKind_Xor,
		{ binOpFunc <int, sl::Or  <int> >, NULL },                                  // BinOpKind_Or,
	};

	if (!m_type || !value.m_type)
	{
		err::setError ("cannot apply operators to 'null' values");
		return false;
	}

	ASSERT (opKind < countof (operatorTable));
	Operator* op = &operatorTable [opKind];

	Type type = AXL_MAX (m_type, value.m_type);
	switch (type)
	{
	case Type_Int:
		m_integer = op->m_intFunc (m_integer, value.m_integer);
		break;

	case Type_Fp:
		if (!op->m_fpFunc)
		{
			err::setFormatStringError (
				"cannot apply binary '%s' to floating-point values",
				getBinOpKindString (opKind)
				);
			return false;
		}

		m_fp = op->m_fpFunc (getFp (), value.getFp ());
		m_type = Type_Fp;
		break;

	default:
		ASSERT (false);
	}

	return true;
}

bool
Value::relationalOperator (
	RelOpKind opKind,
	const Value& value
	)
{
	struct Operator
	{
		bool
		(*m_intFunc) (int, int);

		bool
		(*m_fpFunc) (double, double);
	};

	static Operator operatorTable [BinOpKind__Count] =
	{
		{ relOpFunc <int, sl::Eq <int> >, relOpFunc <double, sl::Eq <double> > }, // RelOpKind_Eq,
		{ relOpFunc <int, sl::Ne <int> >, relOpFunc <double, sl::Ne <double> > }, // RelOpKind_Ne,
		{ relOpFunc <int, sl::Lt <int> >, relOpFunc <double, sl::Lt <double> > }, // RelOpKind_Lt,
		{ relOpFunc <int, sl::Gt <int> >, relOpFunc <double, sl::Gt <double> > }, // RelOpKind_Gt,
		{ relOpFunc <int, sl::Le <int> >, relOpFunc <double, sl::Le <double> > }, // RelOpKind_Le,
		{ relOpFunc <int, sl::Ge <int> >, relOpFunc <double, sl::Ge <double> > }, // RelOpKind_Ge,
	};

	if (!m_type || !value.m_type)
	{
		err::setError ("cannot apply operators to 'null' values");
		return false;
	}

	ASSERT ((size_t) opKind < countof (operatorTable));
	Operator* op = &operatorTable [opKind];

	Type type = AXL_MAX (m_type, value.m_type);
	switch (type)
	{
	case Type_Int:
		m_integer = op->m_intFunc (m_integer, value.m_integer);
		break;

	case Type_Fp:
		m_integer = op->m_fpFunc (getFp (), value.getFp ());
		m_type = Type_Int;
		break;

	default:
		ASSERT (false);
	}

	return true;
}

//..............................................................................
