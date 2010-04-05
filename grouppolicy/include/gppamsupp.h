#ifndef __GPPAMSUPP_H__
#define __GPPAMSUPP_H__

typedef void (*gp_pam_msg_cb_t)(void *context, int is_err, char *format, ...);

int
gp_pam_get_interactive_logon_rights(
    char** pszValue
    );

int
gp_pam_process_login(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    );

int
gp_pam_process_logout(
    void* context,
    const char* Username,
    int cached,
    gp_pam_msg_cb_t log_cb,
    gp_pam_msg_cb_t user_msg_cb
    );

void
gp_pam_free_buffer(
    char* buf
    );

#endif /* __GPPAMSUPP_H__ */

