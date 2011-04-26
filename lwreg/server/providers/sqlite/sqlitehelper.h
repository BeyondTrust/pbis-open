#include "includes.h"

// Sqlite provider-specific helper functions 
void
SqliteSafeFreeKeyContext(
    IN PREG_KEY_CONTEXT pKeyResult
    );

void
SqliteResetSubKeyInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
SqliteGetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

NTSTATUS
SqliteSetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

void
SqliteResetValueInfo(
    IN OUT PREG_KEY_CONTEXT pKey
    );
