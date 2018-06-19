#!/bin/sh -e
# Check that config files with embedded spaces and comments
# can be parsed properly.
TMPCONF=$(mktemp)
cat >$TMPCONF <<EOT
#
# Comment
#
/home:lustre:/home:90
/g/g1:fruity.bar.z:/vol/2/4:0  # Comment
/g/g2:pebbles.foo.org:/:0  # Comment
# Comment 

    # Comment  
EOT
echo "=== list ==="
./tconf $TMPCONF
echo "=== conf_get_bylabel ==="
./tconf $TMPCONF /home /g/g1 /g/g2 /g /g/g1/foo
echo "=== conf_get_bylabel CONF_MATCH_SUBDIR ==="
./tconf $TMPCONF -/home -/g/g1 -/g/g2 -/g -/g/g1/foo -/g/g1/a/b/c/d/e/f/g/

rm -f $TMPCONF
