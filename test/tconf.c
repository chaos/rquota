#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "getconf.h"

static void print_conf_ent(char *key, confent_t *e);
static void usage(void);

int main(int argc, char *argv[])
{
    conf_t config;
    conf_iterator_t itr;
    confent_t *e;
    char *key;
    int i, flags;

    if (argc < 2)
        usage();
    config = conf_init(argv[1]);

    if (argc == 2) {
        itr = conf_iterator_create(config); 
        while ((e = conf_next(itr)))
            print_conf_ent(NULL, e);
        conf_iterator_destroy(itr);
    } else {
        for (i = 2; i < argc; i++) {
            if (argv[i][0] == '-') {
                key = &argv[i][1];
                flags = CONF_MATCH_SUBDIR;
            } else {
                key = argv[i];
                flags = 0;
            }
            e = conf_get_bylabel(config, key, flags);
            print_conf_ent(key, e);
        }
    }
    conf_fini(config);
}

static void 
usage(void)
{
    fprintf(stderr, "Usage: tconf config_path [label-str ...]\n");
    exit(1);
}

static void 
print_conf_ent(char *key, confent_t *e)
{
    if (key)
        printf("%s: ", key);
    if (e)
        printf("%s:%s:%s:%d\n",
               e->cf_label, e->cf_rhost, e->cf_rpath, e->cf_thresh);
    else
        printf("not found\n");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
