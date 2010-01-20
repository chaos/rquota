#!/bin/sh

cat >x.conf <<EOT
/foo:test:nothing:0
EOT
$PATH_QUOTA -f x.conf 102
