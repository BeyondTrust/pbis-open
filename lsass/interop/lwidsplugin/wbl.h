/* Winbind Logon (WBL) Library (aka "wobble")

   Copyright Danilo Almeida <dalmeida@centeris.com> 2007
*/

/* Note that you need to include headers to satisfy the following:
 *
 * uint{8,32}_t (usually <stdint.h> or <inttypes.h>)
 * time_t (usually <sys/time.h>)
 * va_list (usually <stdarg.h>)
 */

#ifndef __WBL_H__
#define __WBL_H__


typedef uint32_t WBL_STATUS;

#define WBL_STATUS_OK                           0
#define WBL_STATUS_ERROR                        1
#define WBL_STATUS_MEMORY_INSUFFICIENT          2
#define WBL_STATUS_BUFFER_INSUFFICIENT          3
#define WBL_STATUS_LICENSE_ERROR                4
#define WBL_STATUS_SERVER_UNAVAILABLE           5
#define WBL_STATUS_LOGON_BAD                    6 /* NT_STATUS_LOGON_FAILURE -- standard bad user/password logon error */
#define WBL_STATUS_ACCOUNT_UNKNOWN              7
#define WBL_STATUS_ACCOUNT_INVALID              8
#define WBL_STATUS_ACCOUNT_DISABLED             9
#define WBL_STATUS_ACCOUNT_LOCKED_OUT          10
#define WBL_STATUS_ACCOUNT_EXPIRED             11

/* Account and policy logon restrictions: */
#define WBL_STATUS_LOGON_RESTRICTED_ACCOUNT    12
#define WBL_STATUS_LOGON_RESTRICTED_COMPUTER   13
#define WBL_STATUS_LOGON_RESTRICTED_TIME       14
#define WBL_STATUS_LOGON_TYPE_DENIED           15

#define WBL_STATUS_PASSWORD_EXPIRED            16 /* ok, but need to change password (NT_STATUS_PASSWORD_EXPIRED) */
#define WBL_STATUS_PASSWORD_MUST_CHANGE        17 /* ok, but need to change password (NT_STATUS_PASSWORD_MUST_CHANGE) */

/* For password change: */
#define WBL_STATUS_PASSWORD_WRONG              18 /* (NT_STATUS_WRONG_PASSWORD) -- for pwd change */
#define WBL_STATUS_PASSWORD_INVALID            19 /* (NT_STATUS_ILL_FORMED_PASSWORD) -- for pwd change */
#define WBL_STATUS_PASSWORD_RESTRICTION        20 /* (NT_STATUS_PASSWORD_RESTRICTION) -- for pwd change */

/* Specific password change restrictions: */
#define WBL_STATUS_PASSWORD_TOO_SHORT          21 /* NT_STATUS_PASSWORD_TOO_SHORT */
#define WBL_STATUS_PASSWORD_TOO_RECENT         22 /* NT_STATUS_PASSWORD_TOO_RECENT */
#define WBL_STATUS_PASSWORD_IN_HISTORY         23 /* NT_STATUS_PASSWORD_HISTORY_CONFLICT */
#define WBL_STATUS_PASSWORD_NOT_COMPLEX        24 /* REJECT_REASON_NOT_COMPLEX */

#define WBL_STATUS_INVALID_STATE               25
#define WBL_STATUS_ACCESS_DENIED               26
#define WBL_STATUS_USER_POLICY_ERROR           27


typedef uint32_t WBL_CONFIG_FLAGS;

#define WBL_CONFIG_FLAG_SILENT                0x00000001
#define WBL_CONFIG_FLAG_DEBUG                 0x00000002
#define WBL_CONFIG_FLAG_DEBUG_STATE           0x00000004

#define WBL_CONFIG_FLAG_USE_AUTHTOK           0x00000010
#define WBL_CONFIG_FLAG_TRY_FIRST_PASS        0x00000020
#define WBL_CONFIG_FLAG_USE_FIRST_PASS        0x00000040
#define WBL_CONFIG_FLAG_UNKNOWN_OK            0x00000080
#define WBL_CONFIG_FLAG_REMEMBER_CHPASS       0x00000100

/* #define WBL_CONFIG_FLAG_OLD_PASSWORD */

#define WBL_CONFIG_FLAG_EXTERNAL_MASK         0x00000FFF
/* Flags above 0xFFF are for internal use */


typedef struct _WBL_STATE WBL_STATE;


typedef struct _WBL_PASSWORD_POLICY_INFO {
	uint32_t MinimumLength;
	uint32_t History;
	bool Complexity;
	time_t ExpireTime;
	time_t MinimumAge;
} WBL_PASSWORD_POLICY_INFO;


/* Handling ERROR, WARN, and INFO are required for messages */
typedef enum {
	WBL_LOG_LEVEL_NONE,
	WBL_LOG_LEVEL_ERROR,
	WBL_LOG_LEVEL_WARN,
	WBL_LOG_LEVEL_INFO,
	WBL_LOG_LEVEL_VERBOSE,
	WBL_LOG_LEVEL_DEBUG
} WBL_LOG_LEVEL;

typedef void (*WBL_LOG_CALLBACK)(
	void* Context,
	WBL_LOG_LEVEL Level,
	const char* Format,
	va_list Args
	);


/* Helpers */
bool WblStatusIsAuthenticated(WBL_STATUS Status);
bool WblStatusIsPasswordChangeRequired(WBL_STATUS Status);
bool WblStatusHavePolicyRestriction(WBL_STATUS Status);

bool WblStateIsAuthenticated(WBL_STATE* State);
bool WblStateIsPasswordChangeRequiredFromState(WBL_STATE* State);
bool WblStateHavePolicyRestriction(WBL_STATE* State);

void WblShowStatusMessages(WBL_STATE *State, WBL_STATUS Status);

/*
time_t WblGetPasswordLastSet(WBL_STATE* State);
*/

const char* WblStateGetCanonicalUserName(WBL_STATE* State);
const char* WblGetUserPrincipalName(WBL_STATE* State);
const char* WblStateGetHomeDirectory(WBL_STATE* State);
const char* WblStateGetLogonScript(WBL_STATE* State);
const char* WblStateGetLogonServer(WBL_STATE* State);
const char* WblStateGetProfilePath(WBL_STATE* State);
const char* WblStateGetKrb5CCacheName(WBL_STATE* State);
const char* WblStateGetKrb5ConfigPath(WBL_STATE* State);

bool WblStateIsCachedLogon(WBL_STATE* State);
bool WblStateIsGraceLogon(WBL_STATE* State);
bool WblStateIsKrb5ClockSkewFailure(WBL_STATE* State);
bool WblStateIsPasswordChanged(WBL_STATE* State);

WBL_CONFIG_FLAGS WblStateGetConfigFlags(WBL_STATE* State);
const char* WblStateGetConfigString(WBL_STATE* State, const char* Key);

/* returns false if there is no expiration information available */
bool WblStateGetNextPasswordChangeTime(WBL_STATE* State, time_t* NextChangeTime);
bool WblStateGetNextPasswordChangeTimeWarningSeconds(WBL_STATE* State, int* Seconds);
const WBL_PASSWORD_POLICY_INFO* WblStateGetPasswordPolicy(WBL_STATE* State);


/* Main functions */

WBL_STATUS
WblStateCreate(
	/* OUT */ WBL_STATE** State,
	/* IN */ WBL_LOG_CALLBACK LogFunction,
	/* IN */ WBL_LOG_CALLBACK MessageFunction,
	/* OPTIONAL IN */ void* Context,
	/* OPTIONAL IN */ const char** AdditionalKeys,
	/* IN */ const char* DefaultConfigPath,
	/* IN */ int argc,
	/* IN */ const char** argv
	);

void
WblStateRelease(
	/* IN OUT */ WBL_STATE* State
	);

WBL_STATUS
WblAuthenticate(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* IN */ const char* Password
	);

/* We expose these directly because the calling plug-in mechanism
   may or may not have access to previous state variables and/or
   be or not be in root context, etc. */
WBL_STATUS
WblCreateK5Login(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* OPTIONAL IN */ const char* UserPrincipalName
	);

WBL_STATUS
WblCreateHomeDirectory(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	);

WBL_STATUS
WblApplyUserLoginPolicies(
        /* IN */ WBL_STATE* State,
        /* IN */ const char* Username
        );

WBL_STATUS
WblApplyUserLogoutPolicies(
        /* IN */ WBL_STATE* State,
        /* IN */ const char* Username
        );

WBL_STATUS
WblLogoff(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* OPTIONAL IN */ const char* Krb5CCacheName
	);

WBL_STATUS
WblAuthorize(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	);

WBL_STATUS
WblChangePassword(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* IN */ const char* OldPassword,
	/* IN */ const char* NewPassword
	);

WBL_STATUS
WblIsKnownUser(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	);

bool
WblPing(
	/* IN OUT */ WBL_STATE* State
	);

WBL_STATUS
WblSetUsername(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username
	);

const char*
WblGetUsername(
	/* IN */ WBL_STATE* State
	);

WBL_STATUS
WblQueryUserPrincipalName(
	/* IN OUT */ WBL_STATE* State,
	/* IN */ const char* Username,
	/* OUT */ const char** PrincipalName
	);

#if defined(AIX)

void WblMangleAIX(unsigned int id, char buffer[9]);
unsigned int WblUnmangleAIX(const char* buffer);
bool WblIsMangledAIX(const char* buffer);

#endif /* AIX */

#endif /* __WBL_H__ */
