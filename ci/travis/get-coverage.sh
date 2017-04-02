#...............................................................................
#
#  This file is part of the Graco toolkit.
#
#  Graco is distributed under the MIT license.
#  For details see accompanying license.txt file,
#  the public copy of which is also available at:
#  http://tibbo.com/downloads/archive/graco/license.txt
#
#...............................................................................

if [ "$TRAVIS_OS_NAME" == "linux" ] && [ "$CC" == "clang" ]; then
	return # lcov doesn't work with clang on ubuntu out-of-the-box
fi

lcov --capture --directory . --no-external --output-file coverage.info
lcov --remove coverage.info '*/axl/*' --output-file coverage.info
lcov --list coverage.info

curl -s https://codecov.io/bash | bash
