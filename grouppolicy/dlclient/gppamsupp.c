#include "config.h"
#include "gposystem.h"
#include "grouppolicy.h"
#include "gpodefines.h"

#include "cterr.h"

#include "lwerror.h"
#include "lwmem.h"
#include "lwstr.h"
#include "lwfile.h"

#include "gpacfgparser.h"

#include "gpoutils.h"

#include "gppamsupp.h"

#define MSG_FORMAT_AGENT_ERROR          "Unable to communicate with the Likewise Enterprise Group Policy Agent."
#define MSG_FORMAT_PROCESS_LOGIN        "Unable to apply policies to login context."
#define MSG_FORMAT_PROCESS_LOGOUT       "Unable to apply policies to logout context."
#define MSG_FORMAT_PROCESS_LOGIN_CACHED "Logged in using cached credentials; user policy could not be applied"
#define MSG_FORMAT_PROCESS_LOGOUT_CACHED "Logged in using cached credentials; user policy could not be applied"

static void ConvertToPamFormat(char* Value)
{
    int i = 0, j = 0;

    if (Value) {
        while(Value[j]) {
            while(Value[i]) {
                if (Value[i] == '*') {
                    i++;

                } else {
                    i++;
                    j++;
                }
                Value[j] = Value[i];
            }
        }
        Value[j] = 0;
    }
}

#define PAM_POLICY_CONF CACHEDIR "/pam_security_policy.conf"

/* return non-zero on success, 0 on failure, returned string can be null if nothing there */
int gp_pam_get_interactive_logon_rights(char** pszValue)
{
    int ok = 0;
    CENTERROR ceError = CENTERROR_SUCCESS;
    PGPACFGSECTION pPamPolicyList = NULL;
    PSTR Value = NULL;

    /*
     * Get SeInteractiveLogonRight policy value from current
     * /var/cache/centeris/grouppolicy/pam_security_policy.conf file.
     */
    ceError = GPAParseConfigFile(PAM_POLICY_CONF,
                                &pPamPolicyList,
                                FALSE);
    if (ceError)
    {
        BOOLEAN fileExists;
        ceError = GPACheckFileExists(PAM_POLICY_CONF, &fileExists);
        GOTO_CLEANUP_ON_CENTERROR(ceError);
        if (!fileExists)
        {
            ok = 1;
            GOTO_CLEANUP();
        }
    }

    ceError = GPAGetConfigValueBySectionName(pPamPolicyList,
                                            "PAM Policies",
                                            "SeInteractiveLogonRight",
                                            &Value);
    switch (ceError)
    {
        case CENTERROR_SUCCESS:
            break;
        case CENTERROR_CFG_SECTION_NOT_FOUND:
        case CENTERROR_CFG_VALUE_NOT_FOUND:
            ok = 1;
        default:
            GOTO_CLEANUP();
            break;
    }

    ConvertToPamFormat(Value);
    ok = 1;

cleanup:
    if (ok)
    {
        *pszValue = Value;
        Value = NULL;
    }
    else
    {
        *pszValue = NULL;
    }

    LW_SAFE_FREE_STRING(Value);

    if (pPamPolicyList)
    {
        GPAFreeConfigSectionList(pPamPolicyList);
    }

    return ok;
}

int
gp_pam_process_login(
		void* context,
		const char* Username,
		int cached,
		gp_pam_msg_cb_t log_cb,
		gp_pam_msg_cb_t user_msg_cb
		)
{
    int retval = 0;
    DWORD dwError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;

    dwError = GPOClientOpenContext(&hGPConnection);
    if (dwError)
    {
        log_cb(context, 1, MSG_FORMAT_AGENT_ERROR);
        user_msg_cb(context, 1, MSG_FORMAT_AGENT_ERROR);
        goto out;
    }
    dwError = GPOClientProcessLogin(hGPConnection, Username);
    GPOClientCloseContext(hGPConnection);
    hGPConnection = (HANDLE)NULL;

    if (dwError)
    {
	if (cached)
	{
	    log_cb(context, 1, MSG_FORMAT_PROCESS_LOGIN_CACHED);
	    user_msg_cb(context, 1, MSG_FORMAT_PROCESS_LOGIN_CACHED);
	}
	else
	{
	    log_cb(context, 1, MSG_FORMAT_PROCESS_LOGIN);
	    user_msg_cb(context, 1, MSG_FORMAT_PROCESS_LOGIN);
	}
        goto out;
    }

    retval = 1;

 out:

    return retval;
}

int
gp_pam_process_logout(
		void* context,
		const char* Username,
		int cached,
		gp_pam_msg_cb_t log_cb,
		gp_pam_msg_cb_t user_msg_cb
		)
{
    int retval = 0;
    DWORD dwError = 0;
    HANDLE hGPConnection = (HANDLE)NULL;

    dwError = GPOClientOpenContext(&hGPConnection);
    if (dwError)
    {
        log_cb(context, 1, MSG_FORMAT_AGENT_ERROR);
        user_msg_cb(context, 1, MSG_FORMAT_AGENT_ERROR);
        goto out;
    }
    dwError = GPOClientProcessLogout(hGPConnection, Username);
    GPOClientCloseContext(hGPConnection);
    hGPConnection = (HANDLE)NULL;

    if (dwError)
    {
	if (cached)
	{
	    log_cb(context, 1, MSG_FORMAT_PROCESS_LOGOUT_CACHED);
	    user_msg_cb(context, 1, MSG_FORMAT_PROCESS_LOGOUT_CACHED);
	}
	else
	{
	    log_cb(context, 1, MSG_FORMAT_PROCESS_LOGOUT);
	    user_msg_cb(context, 1, MSG_FORMAT_PROCESS_LOGOUT);
	}
        goto out;
    }

    retval = 1;

 out:

    return retval;
}

void gp_pam_free_buffer(char* buf)
{
    LW_SAFE_FREE_STRING(buf);
}

