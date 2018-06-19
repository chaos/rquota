#!/bin/sh -e
# Verify that '-' works as config file source
./tconf - <<EOT
#
# Comment
#
/home:lustre:/home:90
/g/g1:fruity.bar.z:/vol/2/4:0  # Comment
/g/g2:pebbles.foo.org:/:0  # Comment
# Comment 

    # Comment  
EOT
