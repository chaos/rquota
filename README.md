### rquota

`rquota` provides drop-in replacements for the `quota` and `repquota`
utilities on Linux and other UNIX-like operating systems.
Its configuration file makes it possible to limit the file systems
queried to only those of interest, and to alias them for human readability
in quota output.  Block counts are converted to human readable units.

`rquota` supports remote NFS and, if configured `--with-lustre`,
Lustre file systems.  At this time it does not support quotas on
local file systems.

### Config file

`/etc/quota.conf` consists of one line per file system, with `#`
indicating that the remainder of a line is a comment.

Each line consists of the following fields delimited by colons:
* _description_: the path that will be displayed by  the  quota  program,
typically the local mount point
* _hostname_: the name of the NFS server exporting the file system, or
the `lustre` if the file system type is Lustre.
* _remote path_: the NFS server-side path of file system,  or  the  local
mount point if the file system type is Lustre.
* _percent_: the  percentage of hard quota which, when exceeded, causes
quota to warn the user.  This can be used as a simpler  alternative  to
soft quotas if desired.  Set to zero to disable.
* _nolimit_: an optional flag that indicates that this file system has no
limits set so don’t bother querying it when quota is run without the -v option.

### Origin

This utility was originally written by Jim Garlick for California State
University, Chico in the mid 1990s, for a network of HP-UX systems.
CSU, Chico graciously allowed it to be released as open source.
