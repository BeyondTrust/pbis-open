#include "includes.h"
typedef int (*main_function) (int argc, char* argv[]);

typedef struct
{
    const char* name;
    main_function func;
} main_entry;

int
authenticate_user_main(
    int argc,
    char* argv[]
    );

int
validate_user_main(
    int argc,
    char* argv[]
    );

int
enum_users_main(
    int argc,
    char* argv[]
    );

int
enum_groups_main(
    int argc,
    char* argv[]
    );

int
find_user_by_name_main(
    int argc,
    char* argv[]
    );

int
find_user_by_id_main(
    int argc,
    char* argv[]
    );

int
find_group_by_name_main(
    int argc,
    char* argv[]
    );

int
find_group_by_id_main(
    int argc,
    char* argv[]
    );

int
validate_groupinfo_by_name_main(
    int argc,
    char* argv[]
    );

int
validate_groupinfo_by_id_main(
    int argc,
    char* argv[]
    );

int
validate_groupinfo_by_api_main(
    int argc,
    char* argv[]
    );

int
check_user_info_main(
    int argc,
    char* argv[]
    );

int
open_session_main(
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
verify_sid_info_main(
    int argc,
    char* argv[]
    );

int
test_local_provider_main(
    int argc,
    char* argv[]
    );

int
check_gid_main(
    int argc,
    char* argv[]
    );
