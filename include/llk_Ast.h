// This file is part of AXL (R) Library
// Tibbo Technology Inc (C) 2004-2013. All rights reserved
// Author: Vladimir Gladkov

#pragma once

#define _LLK_AST_H

namespace llk {
	
//.............................................................................

template <class TToken>
class CAstNodeT: public axl::rtl::TListLink
{
public:
	typedef TToken CToken;

public:
	int m_Kind;

	CToken m_FirstToken;
	CToken m_LastToken;		

	// later create a wrapper for ast tree 
	// we don't really need tree fields and listlink until we plan to keep ast nodes

	CAstNodeT* m_pParent;
	axl::rtl::CArrayT <CAstNodeT*> m_Children;

public:
	CAstNodeT ()
	{
		m_Kind = -1;
		m_pParent = NULL;
	}

	virtual
	~CAstNodeT () // could be subclassed
	{
	}
};

//.............................................................................

template <class TAstNode>
class CAstT
{
public:
	typedef TAstNode CAstNode;

protected:
	axl::rtl::CStdListT <CAstNode> m_List;
	CAstNode* m_pRoot;

public:
	CAstT ()
	{
		m_pRoot = NULL;
	}

	CAstNode*
	GetRoot ()
	{
		return m_pRoot;
	}

	void
	Clear ()
	{
		m_List.Clear ();
		m_pRoot = NULL;
	}

	void
	Add (CAstNode* pAstNode)
	{
		m_List.InsertTail (pAstNode);
		if (!m_pRoot)
			m_pRoot = pAstNode;
	}
};

//.............................................................................

} // namespace llk
