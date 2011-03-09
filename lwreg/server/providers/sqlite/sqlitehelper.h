#include "includes.h"

// Sqlite provider-specific helper functions 
void
RegSrvSafeFreeKeyContext(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvGetKeyRefCount(
    IN PREG_KEY_CONTEXT pKeyResult
    );

void
RegSrvResetSubKeyInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

BOOLEAN
RegSrvHasSubKeyInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvSubKeyNum(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvSubKeyNameMaxLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

PCWSTR
RegSrvSubKeyName(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN DWORD dwIndex
    );

BOOLEAN
RegSrvHasSecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult
    );

ULONG
RegSrvGetKeySecurityDescriptorSize(
    IN PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegSrvGetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

NTSTATUS
RegSrvGetKeySecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

NTSTATUS
RegSrvSetKeySecurityDescriptor_inlock(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

NTSTATUS
RegSrvSetKeySecurityDescriptor(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN ULONG ulSecDescRelLen
    );

void
RegSrvResetValueInfo(
    IN OUT PREG_KEY_CONTEXT pKey
    );

BOOLEAN
RegSrvHasValueInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    );

BOOLEAN
RegSrvHasDefaultValueInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvValueNum(
    IN PREG_KEY_CONTEXT pKeyResult
    );

DWORD
RegSrvDefaultValueNum(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvMaxValueNameLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );

size_t
RegSrvMaxValueLen(
    IN PREG_KEY_CONTEXT pKeyResult
    );
