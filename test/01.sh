#!/bin/sh

quota=$1
repquota=$2

cat >x.conf <<EOT
/foo:test:nothing:0
EOT
$quota -v -f x.conf 100
