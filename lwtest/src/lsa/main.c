#include "main.h"

static
int
lsa_main(
    int argc,
    char* argv[]
    );

static
int
lsa_all(
    int argc,
    char* argv[]
);

static main_entry entry_list[] =
{
    {"lsa", lsa_main},
    {"all", lsa_all},
    {"authenticate-user", authenticate_user_main},
    {"check-gid", check_gid_main},
    {"check-user-info", check_user_info_main},
    {"enum-groups", enum_groups_main},
    {"enum-users", enum_users_main},
    {"find-group-by-id", find_group_by_id_main},
    {"find-group-by-name", find_group_by_name_main},
    {"find-user-by-id", find_user_by_id_main},
    {"find-user-by-name", find_user_by_name_main},
    {"get-metrics", get_metrics_main},
    {"get-status", get_status_main},
    {"open-session", open_session_main},
    {"test-local-provider", test_local_provider_main},
    {"validate-groupinfo-by-api", validate_groupinfo_by_api_main},
    {"validate-groupinfo-by-id", validate_groupinfo_by_id_main},
    {"validate-groupinfo-by-name", validate_groupinfo_by_name_main},
    {"validate-user", validate_user_main},
    {"verify-sid-info", verify_sid_info_main},
    {NULL, NULL}
};

static
main_entry*
find_entry(
    const char* name
    );


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
int
lsa_all(
    int argc,
    char* argv[]
    )
{
    int i;

    for (i = 0; entry_list[i].name; i++)
    {
        if ( (strcmp("lsa", entry_list[i].name)) && (strcmp("all", entry_list[i].name)))
        {
            entry_list[i].func(argc, argv);
        }
    }

    return 0;
}


int 
main(
    int argc, 
    char** argv
    )
{
    main_entry *entry = NULL;
    char* name = basename(argv[0]);

    if (!strncmp(name, "lwt-lsa-", 8))
    {
        name += 8;
    }
    else if (!strncmp(name, "lwtest-", 7))
    {
        name += 7;
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

