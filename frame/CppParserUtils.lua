-------------------------------------------------------------------------------

if NoPpLine then
	PpLinePrefix = "//"
end

if ParserClassName == nil then
	ParserClassName = "Parser"
end

if TokenClassName == nil then
	TokenClassName = "Token"
end

AstNodeVariableName = "__astNode"
SymbolVariableName  = "__symbol"

ClassCount      = #ClassTable
TokenCount      = #TokenTable
SymbolCount     = #SymbolTable
SequenceCount   = #SequenceTable
ActionCount     = #ActionTable
ArgumentCount   = #ArgumentTable
BeaconCount     = #BeaconTable
DispatcherCount = #DispatcherTable
LaDfaCount      = #LaDfaTable

TotalCount      = TokenCount + SymbolCount + SequenceCount + ActionCount + ArgumentCount + BeaconCount + LaDfaCount

TokenEnd        = TokenCount
NamedSymbolEnd  = TokenEnd + NamedSymbolCount
SymbolEnd       = TokenEnd + SymbolCount
SequenceEnd     = SymbolEnd + SequenceCount
ActionEnd       = SequenceEnd + ActionCount
ArgumentEnd     = ActionEnd + ArgumentCount
BeaconEnd       = ArgumentEnd + BeaconCount
LaDfaEnd        = BeaconEnd + LaDfaCount

-------------------------------------------------------------------------------

function GetPpLine (
	FilePath,
	Line
	)
	return string.format (
		"%d \"%s\"",
		Line + 1,
		string.gsub (FilePath, "\\", "/")
		)
end

function GetPpLineDefault ()
	return GetPpLine (TargetFilePath, getLine () + 1)
end

function GetTokenString (Token)
	if Token.IsEofToken then
		return "'\\00'"
	elseif Token.IsAnyToken then
		return "'\\01'"
	elseif Token.Name then
		return Token.Name
	else
		TokenChar = string.char (Token.Token)
		if string.match (TokenChar, "[%g ]") then
			return string.format ("'%s'", TokenChar)
		else
			return Token.Token
		end
	end
end

-------------------------------------------------------------------------------

function GetSymbolDeclaration (
	Symbol,
	Name,
	Value
	)
	if Symbol.IsCustom then
		return string.format ("SymbolNode_%s* %s = (SymbolNode_%s*) %s;", Symbol.Name, Name, Symbol.Name, Value)
	else
		return string.format ("SymbolNode* %s = %s;", Name, Value)
	end
end

-------------------------------------------------------------------------------

function GetAstDeclaration (
	Symbol,
	AstName,
	SymbolName
	)
	if Symbol.Class then
		return string.format ("%s* %s = (%s*) %s->m_astNode;", Symbol.Class, AstName, Symbol.Class, SymbolName)
	else
		return string.format ("AstNode* %s = %s->m_astNode;", AstName, SymbolName)
	end
end

-------------------------------------------------------------------------------

function ProcessActionUserCode (
	UserCode,
	Dispatcher,
	SymbolName,
	AstName
	)
	return (string.gsub (
		UserCode,
		"%$(%w*)",
		function (s)
			if s == "" or s == "ast" then
				return string.format ("(*%s)", AstName)
			elseif s == "arg" then
				return string.format ("%s->m_arg", SymbolName)
			elseif s == "local" then
				return string.format ("%s->m_local", SymbolName)
			elseif not Dispatcher then
				error (string.format ("invalid locator $%s", s))
			else
				SlotIndex = tonumber (s)
				Symbol = Dispatcher.BeaconTable [SlotIndex + 1].Symbol

				if not Symbol then
					return string.format ("(*getTokenLocator (%d))", SlotIndex)
				elseif Symbol.Class then
					return string.format ("(*(%s*) getAstLocator (%d))", Symbol.Class, SlotIndex)
				else
					return string.format ("(*getAstLocator (%d))", SlotIndex)
				end
			end
		end
		)) -- get rid of second value returned by string.gsub
end

-------------------------------------------------------------------------------
