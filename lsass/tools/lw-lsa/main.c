#include "main.h"
#include "common.h"

int
pb_mode = 0;

static
int
lsa_main(
    int argc,
    char* argv[]
    );

static
int
pbis_main(
    int argc,
    char* argv[]
    );

static
main_entry*
find_entry(
    const char* name
    );

static main_entry entry_list[] =
{
    {"lsa", lsa_main, 0x0},
    {"pbis", pbis_main, 0x0},
    {"ad-cache", ad_cache_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"ad-get-machine", ad_get_machine_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"add-group", add_group_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"add-user", add_user_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"del-group", del_group_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"del-user", del_user_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"enum-groups", enum_groups_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"enum-users", enum_users_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"find-by-sid", find_by_sid_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"find-group-by-id", find_group_by_id_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"find-group-by-name", find_group_by_name_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"find-user-by-id", find_user_by_id_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"find-user-by-name", find_user_by_name_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"get-metrics", get_metrics_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"get-status", get_status_main, PB_MODE_LSA},
    {"status", get_status_main, PB_MODE_PBIS},
    {"list-groups-for-user", list_groups_for_user_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"ypcat", lw_ypcat_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"ypmatch", lw_ypmatch_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"mod-group", mod_group_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"mod-user", mod_user_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"set-default-domain", set_default_domain_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"set-machine-sid", set_machine_sid_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"set-machine-name", set_machine_name_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"trace-info", trace_info_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"find-objects", FindObjectsMain, PB_MODE_LSA | PB_MODE_PBIS},
    {"enum-objects", EnumObjectsMain, PB_MODE_LSA | PB_MODE_PBIS},
    {"enum-members", EnumMembersMain, PB_MODE_LSA | PB_MODE_PBIS},
    {"query-member-of", QueryMemberOfMain, PB_MODE_LSA | PB_MODE_PBIS},
    {"account-rights", account_rights_main, PB_MODE_LSA | PB_MODE_PBIS},
    {"authenticate-user", AuthenticateUserMain, PB_MODE_LSA | PB_MODE_PBIS},
    {"join", JoinMain, PB_MODE_LSA | PB_MODE_PBIS},
    {"leave", LeaveMain, PB_MODE_LSA | PB_MODE_PBIS},
    {NULL, NULL}
};

int main(int argc, char** argv)
{
    main_entry *entry = NULL;
    char* name = basename(argv[0]);

    if (!strncmp(name, "pbis-", 5))
    {
        pb_mode = PB_MODE_PBIS;
        name += 5;
    }
    else if (!strncmp(name, "lsa-", 4))
    {
        name += 4;
    }
    else if (!strncmp(name, "lw-", 3))
    {
        name += 3;
    }

    entry = find_entry(name);

    if (!entry)
    {
        fprintf(stderr, "Unrecognized mode: %s\n", name);
        return -1;
    }
    else
    {
        return entry->func(argc, argv);
    }
}

static
int
cli_main(
    int argc,
    char* argv[]
    )
{
    main_entry *entry = NULL;
    int i;

    if (argc < 2 || (argc == 2 &&
            (!strcmp(argv[1], "--help") || !strcmp(argv[1], "--usage"))))
    {
        fprintf(stderr, "Usage: %s mode ...\n\n", argv[0]);
        fprintf(stderr, "Available modes:\n");
        
        for (i = 0; entry_list[i].name; i++)
        {
            if (entry_list[i].mode & pb_mode)
            {
                fprintf(stderr, "    %s\n", entry_list[i].name);
            }
        }

        return -1;
    }
    entry = find_entry(argv[1]);

    if (!entry)
    {
        fprintf(stderr, "Unrecognized mode: %s\n", argv[1]);
        return -1;
    }
    else
    {
        return entry->func(argc - 1, argv + 1);
    }
}

static
int
lsa_main(
    int argc,
    char* argv[]
    )
{
    pb_mode = PB_MODE_LSA;

    return cli_main(argc, argv);
}

static
int
pbis_main(
    int argc,
    char* argv[]
    )
{
    pb_mode = PB_MODE_PBIS;

    return cli_main(argc, argv);
}

static
main_entry*
find_entry(
    const char* name
    )
{
    int i;

    for (i = 0; entry_list[i].name; i++)
    {
        if (!strcmp(name, entry_list[i].name))
        {
            return &entry_list[i];
        }
    }

    return NULL;
}
