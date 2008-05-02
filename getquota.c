#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "getconf.h"
#include "getquota.h"

int
getquota(confent_t *c, char *fsname, uid_t uid, quota_t *q)
{
    int rc;

    if (!strcmp(c->cf_host, "lustre")) {
#if WITH_LUSTRE
        rc = getquota_lustre(fsname, uid, c->cf_path, q);
#else   
        fprintf(stderr, "%s: lustre not configured at build time\n", fsname);
        rc = 1;
#endif
    } else {
#if WITH_NFS
        rc = getquota_nfs(fsname, uid, c->cf_host, c->cf_path, q);
#else
        fprintf(stderr, "%s: NFS not configured at build time\n", fsname);
        rc = 1;
#endif
    }
    return rc;
}
