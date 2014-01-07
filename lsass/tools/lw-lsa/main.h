#include "config.h"
#include "common.h"
#include "lsasystem.h"


typedef int (*main_function) (int argc, char* argv[]);

typedef struct
{
    const char* name;
    main_function func;
    int mode;
} main_entry;

int
ad_cache_main(
    int argc,
    char* argv[]
    );

int
ad_get_machine_main(
    int argc,
    char* argv[]
    );

int
add_group_main(
    int argc,
    char* argv[]
    );

int
add_user_main(
    int argc,
    char* argv[]
    );

int
del_group_main(
    int argc,
    char* argv[]
    );

int
del_user_main(
    int argc,
    char* argv[]
    );

int
enum_groups_main(
    int argc,
    char* argv[]
    );

int
enum_users_main(
    int argc,
    char* argv[]
    );

int
find_by_sid_main(
    int argc,
    char* argv[]
    );

int
find_group_by_id_main(
    int argc,
    char* argv[]
    );

int
find_group_by_name_main(
    int argc,
    char* argv[]
    );

int
find_user_by_id_main(
    int argc,
    char* argv[]
    );

int
find_user_by_name_main(
    int argc,
    char* argv[]
    );

int
get_log_info_main(
    int argc,
    char* argv[]
    );

int
get_metrics_main(
    int argc,
    char* argv[]
    );

int
get_status_main(
    int argc,
    char* argv[]
    );

int
list_groups_for_user_main(
    int argc,
    char* argv[]
    );

int
lw_domain_main(
    int argc,
    char* argv[]
    );

int
lw_ypcat_main(
    int argc,
    char* argv[]
    );

int
lw_ypmatch_main(
    int argc,
    char* argv[]
    );

int
mod_group_main(
    int argc,
    char* argv[]
    );

int
mod_user_main(
    int argc,
    char* argv[]
    );

int
refresh_configuration_main(
    int argc,
    char* argv[]
    );

int
set_default_domain_main(
    int argc,
    char* argv[]
    );

int
set_log_level_main(
    int argc,
    char* argv[]
    );

int
set_machine_sid_main(
    int argc,
    char* argv[]
    );

int
set_machine_name_main(
    int argc,
    char* argv[]
    );

int
trace_info_main(
    int argc,
    char* argv[]
    );

int
FindObjectsMain(
    int argc,
    char** ppszArgv
    );

int
EnumObjectsMain(
    int argc,
    char** ppszArgv
    );

int
EnumMembersMain(
    int argc,
    char** ppszArgv
    );

int
QueryMemberOfMain(
    int argc,
    char** ppszArgv
    );

int
account_rights_main(
    int argc,
    char* argv[]
    );

int
AuthenticateUserMain(
    int argc,
    char** ppszArgv
    );

int
JoinMain(
    int argc,
    char** argv
    );

int
LeaveMain(
    int argc,
    char** argv
    );

int
account_rights_main(
    int argc,
    char* argv[]
    );
