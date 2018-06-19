#!/bin/sh -e

TMPCONF=$(mktemp)
cat >$TMPCONF <<EOT
/foo:test:nothing:0
EOT
$PATH_QUOTA -v -f $TMPCONF 101
rm -f $TMPCONF
