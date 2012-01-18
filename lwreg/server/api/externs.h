#ifndef _EXTERNS_H_
#define _EXTERNS_H_

extern PREGPROV_PROVIDER_FUNCTION_TABLE gpRegProvider;

extern const PWSTR ROOT_KEYS[NUM_ROOTKEY];

extern const wchar16_t wszRootKey[];

extern GENERIC_MAPPING gRegKeyGenericMapping;

extern PLW_MAP_SECURITY_CONTEXT gpRegLwMapSecurityCtx;

#endif /* _EXTERNS_H_ */
