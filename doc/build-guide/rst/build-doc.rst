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

Building Documentation
======================

Graco contains two documentation packages:

* Build guide
* Manual

.. expand-macro:: build-doc-intro ./build/graco

Build Guide
-----------

.. expand-macro:: build-doc-build-guide ./build/graco

Manual
------

A manual on Graco command-line tool

Documentation sources are located at: ``./doc/manual`` (not yet, but soon)

Build steps:

.. code-block:: bash

	cd ./build/graco/doc/manual
	./build-html
	./build-pdf
