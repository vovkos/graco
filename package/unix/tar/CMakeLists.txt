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

axl_configure_file_w_permissions (
	build-tar.sh.in
	build-tar.sh
	OWNER_READ OWNER_WRITE OWNER_EXECUTE
	GROUP_READ GROUP_WRITE GROUP_EXECUTE
	WORLD_READ WORLD_WRITE WORLD_EXECUTE
	)

#...............................................................................
