void
FreeClientExtensionList(
    PGROUP_POLICY_CLIENT_EXTENSION pGroupPolicyClientExtensions
    );

CENTERROR
GetCSEListFromRegistry(
    PSTR pszFilePath,
    PGROUP_POLICY_CLIENT_EXTENSION * ppGroupPolicyClientExtensions
    );

CENTERROR
ParseClientExtension(
    PGROUP_POLICY_CLIENT_EXTENSION * ppGPClientExtension
    );

CENTERROR
ValidateGUID(
    PGPA_TOKEN * ppGPAToken
    );
