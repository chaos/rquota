#!/bin/sh

cat >x.conf <<EOT
/foo:test:nothing:0
EOT
$PATH_QUOTA -v -f x.conf 105
