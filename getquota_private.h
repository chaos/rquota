typedef enum { NONE, UNDER, NOTSTARTED, STARTED, EXPIRED } qstate_t;
#define QUOTA_MAGIC 0x3434aaaf
struct quota_struct {
    int                q_magic;
    uid_t              q_uid;
    char              *q_label;
    char              *q_rhost;        /* set to "lustre" if lustre */
    char              *q_rpath;        /* set to local mount pt if lustre */
    int                q_thresh;       /* warn if over this pct of hard quota */
                                       /*   (set to 0 if unused) */
    unsigned long long q_bytes_used;
    unsigned long long q_bytes_softlim;/* 0 = no limit */
    unsigned long long q_bytes_hardlim;/* 0 = no limit */
    unsigned long long q_bytes_secleft;/* valid if state == STARTED */
    qstate_t           q_bytes_state;

    unsigned long long q_files_used;
    unsigned long long q_files_softlim;/* 0 = no limit */
    unsigned long long q_files_hardlim;/* 0 = no limit */
    unsigned long long q_files_secleft;/* valid if state == STARTED */
    qstate_t           q_files_state;
};

int quota_get_lustre(uid_t uid, quota_t q);
int quota_get_nfs(uid_t uid, quota_t q);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
