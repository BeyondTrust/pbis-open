void 
chk_dce_err(
    error_status_t ecode,
    char * where,
    char * why,
    unsigned int fatal
    );

#define PROTOCOL_UDP "ncadg_ip_udp"
#define PROTOCOL_TCP "ncacn_ip_tcp"
#define PROTOCOL_NP  "ncacn_np"
