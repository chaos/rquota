#!/bin/sh

cat >x.conf <<EOT
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
./tconf x.conf
echo "=== conf_get_bylabel ==="
./tconf x.conf /home /g/g1 /g/g2 /g /g/g1/foo
echo "=== conf_get_bylabel CONF_MATCH_SUBDIR ==="
./tconf x.conf -/home -/g/g1 -/g/g2 -/g -/g/g1/foo -/g/g1/a/b/c/d/e/f/g/
