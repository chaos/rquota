#!/bin/sh -e

TMPCONF=$(mktemp)
cat >$TMPCONF <<EOT
/foo:test:nothing:90
EOT
$PATH_QUOTA -v -f $TMPCONF 104
rm -f $TMPCONF
