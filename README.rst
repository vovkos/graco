.. .............................................................................
..
..  This file is part of the Graco toolkit.
..
..  Graco is distributed under the MIT license.
..  For details see accompanying license.txt file,
..  the public copy of which is also available at:
..  http://tibbo.com/downloads/archive/graco/license.txt
..
.. .............................................................................

Graco
=====
.. image:: https://github.com/vovkos/graco/actions/workflows/ci.yml/badge.svg
	:target: https://github.com/vovkos/graco/actions/workflows/ci.yml
.. image:: https://ci.appveyor.com/api/projects/status/1l8srr6wo3ixnc7o?svg=true
	:target: https://ci.appveyor.com/project/vovkos/graco

Abstract
--------

Graco (stands for "GRAmmar COmpiler) is a EBNF-based generator of table-driven top-down parsers of LL(k) grammars. It was created as a parser generator for the `Jancy <https://github.com/vovkos/jancy>`__ programming language. Jancy features safe pointer arithmetics, spreadsheet-like reactive programming, the async-await paradigm, built-in regex-based lexer/scanner generator, and many other features. It is used as the scripting engine of the all-in-one programmable terminal/sniffer/protocol analyzer `IO Ninja <https://ioninja.com>`__.

Notable Features
~~~~~~~~~~~~~~~

* ANYTOKEN support;
* Predictable & configurable conflict resolution mechanism;
* Automatic error recovery via user-defined synchornization token sets;
* Retargetable back-end (via Lua string templates);
* All actions and grammar rule choises are efficient table-driven jumps;
* External tokenization loop;
* Convenient syntax for grammar rule actions, arguments, and return values.
