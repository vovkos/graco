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
.. image:: https://travis-ci.org/vovkos/graco.svg?branch=master
	:target: https://travis-ci.org/vovkos/graco
.. image:: https://ci.appveyor.com/api/projects/status/1l8srr6wo3ixnc7o?svg=true
	:target: https://ci.appveyor.com/project/vovkos/graco
.. image:: https://codecov.io/gh/vovkos/graco/branch/master/graph/badge.svg
	:target: https://codecov.io/gh/vovkos/graco

Abstract
--------

Graco is a EBNF-based generator of table-driven top-down parsers of LL(k) grammars featuring:

- predictable & configurable conflict resolution mechanism;
- retargetable back-end (via Lua string templates);
- ANYTOKEN support;
- external tokenization loop;
- convenient syntax for passing and returning rule arguments;
- and more...
