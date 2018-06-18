#!/bin/sh -e

TMPCONF=$(mktemp)
cat >$TMPCONF <<EOT
/foo:test:nothing:0
EOT
$PATH_REPQUOTA -n -b 1m -f $TMPCONF -u 100-106 /foo
rm -f $TMPCONF
