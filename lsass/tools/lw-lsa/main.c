#include "main.h"

static
int
lsa_main(
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
    {"lsa", lsa_main},
    {"ad-cache", ad_cache_main},
    {"ad-get-machine", ad_get_machine_main},
    {"add-group", add_group_main},
    {"add-user", add_user_main},
    {"del-group", del_group_main},
    {"del-user", del_user_main},
    {"enum-groups", enum_groups_main},
    {"enum-users", enum_users_main},
    {"find-by-sid", find_by_sid_main},
    {"find-group-by-id", find_group_by_id_main},
    {"find-group-by-name", find_group_by_name_main},
    {"find-user-by-id", find_user_by_id_main},
    {"find-user-by-name", find_user_by_name_main},
    {"get-metrics", get_metrics_main},
    {"get-status", get_status_main},
    {"list-groups-for-user", list_groups_for_user_main},
    {"ypcat", lw_ypcat_main},
    {"ypmatch", lw_ypmatch_main},
    {"mod-group", mod_group_main},
    {"mod-user", mod_user_main},
    {"set-default-domain", set_default_domain_main},
    {"set-machine-sid", set_machine_sid_main},
    {"set-machine-name", set_machine_name_main},
    {"trace-info", trace_info_main},
    {"find-objects", FindObjectsMain},
    {"enum-objects", EnumObjectsMain},
    {"enum-members", EnumMembersMain},
    {"query-member-of", QueryMemberOfMain},
    {"account-rights", account_rights_main},
    {"join", JoinMain},
    {"leave", LeaveMain},
    {NULL, NULL}
};

int main(int argc, char** argv)
{
    main_entry *entry = NULL;
    char* name = basename(argv[0]);

    if (!strncmp(name, "lsa-", 4))
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
lsa_main(
    int argc,
    char* argv[]
    )
{
    main_entry *entry = NULL;
    int i;

    if (argc < 2 || (argc == 2 &&
            (!strcmp(argv[1], "--help") || !strcmp(argv[1], "--usage"))))
    {
        fprintf(stderr, "Usage: lsa mode ...\n\n");
        fprintf(stderr, "Available modes:\n");
        
        for (i = 0; entry_list[i].name; i++)
        {
            fprintf(stderr, "    %s\n", entry_list[i].name);
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
