#!/bin/sh

cat >x.conf <<EOT
/foo:test:nothing:90
EOT
$PATH_QUOTA -v -f x.conf 104
