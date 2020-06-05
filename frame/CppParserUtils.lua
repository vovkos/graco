--------------------------------------------------------------------------------
--
--  This file is part of the Graco toolkit.
--
--  Graco is distributed under the MIT license.
--  For details see accompanying license.txt file,
--  the public copy of which is also available at:
--  http://tibbo.com/downloads/archive/graco/license.txt
--
--------------------------------------------------------------------------------

if ParserClassName == nil then
	ParserClassName = "Parser"
end

if TokenClassName == nil then
	TokenClassName = "Token"
end

if SymbolVariableName == nil then
	SymbolVariableName  = "__symbol"
end

if TargetVariableName == nil then
	TargetVariableName  = "__target"
end

if NoPpLine then
	PpLineFormat = "// #line %d \"%s\""
else
	PpLineFormat = "#line %d \"%s\""
end

-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TokenCount      = #TokenTable
SymbolCount     = #SymbolTable
EnterCount      = #EnterTable
LeaveCount      = #LeaveTable
SequenceCount   = #SequenceTable
ActionCount     = #ActionTable
ArgumentCount   = #ArgumentTable
BeaconCount     = #BeaconTable
DispatcherCount = #DispatcherTable
LaDfaCount      = #LaDfaTable
TotalCount      = TokenCount + SymbolCount + SequenceCount + ActionCount + ArgumentCount + BeaconCount + LaDfaCount

TokenEnd        = TokenCount
SymbolEnd       = TokenEnd + SymbolCount
SequenceEnd     = SymbolEnd + SequenceCount
ActionEnd       = SequenceEnd + ActionCount
ArgumentEnd     = ActionEnd + ArgumentCount
BeaconEnd       = ArgumentEnd + BeaconCount
LaDfaEnd        = BeaconEnd + LaDfaCount

-------------------------------------------------------------------------------

function getPpLine(filePath, line)
	return string.format(
		PpLineFormat,
		line + 1,
		string.gsub(filePath, "\\", "/")
		)
end

function getPpLineDefault()
	return getPpLine(TargetFilePath, getLine() + 1)
end

function getTokenString(token)
	if token.isEofToken then
		return "EofToken"
	elseif token.isAnyToken then
		return "AnyToken"
	elseif token.name then
		return token.name
	else
		TokenChar = string.char(token.token)
		if string.match(TokenChar, "[%g ]") then
			return string.format("'%s'", TokenChar)
		else
			return token.token
		end
	end
end

function getSymbolDeclaration(symbol, name, value)
	if symbol.isCustomClass then
		return string.format("SymbolNode_%s* %s = (SymbolNode_%s*)%s;", symbol.name, name, symbol.name, value)
	else
		return string.format("SymbolNode* %s = %s;", name, value)
	end
end

function processActionUserCode(userCode, dispatcher, symbolName)
	return (string.gsub(
		userCode,
		"%$(%w*)",
		function(s)
			if s == "" then
				return string.format("%s->m_value", symbolName)
			elseif s == "param" then
				return string.format("%s->m_param", symbolName)
			elseif s == "local" then
				return string.format("%s->m_local", symbolName)
			elseif not dispatcher then
				error(string.format("invalid locator $%s", s))
			else
				slotIndex = tonumber(s)
				symbol = dispatcher.beaconTable[slotIndex + 1].symbol

				if not symbol then
					return string.format("(*getTokenLocator(%d))", slotIndex)
				elseif symbol.valueBlock then
					return string.format("(*(SymbolNodeValue_%s*)getSymbolLocator(%d))", symbol.name, slotIndex)
				else
					return string.format("(*getSymbolLocator(%d))", slotIndex)
				end
			end
		end
		)) -- get rid of second value returned by string.gsub
end

-------------------------------------------------------------------------------
