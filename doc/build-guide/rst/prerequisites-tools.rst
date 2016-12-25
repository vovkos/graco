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

Tools
=====

.. expand-macro:: tools-intro Graco

Required Tools
--------------

These tools are **required** for building the Graco grammar compiler:

* CMake 3.3 or above

	.. expand-macro:: cmake-common-info Graco

* Ragel

	Graco uses Ragel as a lexer/scanner generator for the tokenization stage of its ``.llk`` files parser.

	.. expand-macro:: ragel-common-info

.. _optional-tools:

Optional Tools
--------------

These tools are **optional** and only needed if you plan to build Graco documentation:

* Python

	.. expand-macro:: python-sphinx-common-info Graco

* Sphinx

	.. expand-macro:: sphinx-common-info Graco
