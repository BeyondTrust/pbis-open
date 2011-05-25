/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/* ex: set tabstop=4 expandtab shiftwidth=4 cindent: */


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_USERSEC_H
#include <usersec.h>
#endif
#include <lwerror.h>
#include <lsa/lsa.h>

ssize_t afgets(char **strp, FILE *stream)
{
    ssize_t size = 0, capacity, read_count;
    char *buffer = NULL, *newbuffer;
    do
    {
        capacity = size*2 + 10;
        newbuffer = (char *)realloc(buffer, capacity);
        if(newbuffer == NULL)
            goto error;
        buffer = newbuffer;
        read_count = read(fileno(stream), buffer + size, capacity - size - 1);
        if(read_count == -1)
            goto error;
        size += read_count;
        buffer[size] = '\0';
    }
    while(size == capacity - 1 && buffer[size-1] != '\n');

    *strp = buffer;
    return size;
error:
    if(buffer != NULL)
        free(buffer);
    return -1;
}

int tty_conv(int num_msg, CONST_PAM_MESSAGE **msg,
        struct pam_response **resp, void *appdata_ptr)
{
    FILE *tty = (FILE *)appdata_ptr;
    int i = 0;
    char *msg_copy = NULL;
    size_t msg_len = 0;
    ssize_t resp_len = 0;
    struct termios old, new;
    struct pam_response *responses = malloc(
            num_msg * sizeof(struct pam_response));

    if(responses == NULL)
        goto error;
    for(i = 0; i < num_msg; i++)
    {
        const struct pam_message *this_msg;
        /* According to http://www.die.net/doc/linux/man/man3/pam_conv.3.html,
           on Linux the msg parameter is an array of pointers to pam_messages.
           On Solaris and HPUX, it is a pointer to an array of pam_messages.
         */
#if defined(sun) || defined(__hpux__)
        this_msg = &(*msg)[i];
#else
        this_msg = msg[i];
#endif
        /* According to http://docs.hp.com/en/B2355-90695/pam_start.3.html
           I should always print a newline at the end of the message if it is
           of type PAM_ERROR_MSG or PAM_TEXT_INFO, but not print a newline if
           is of type PAM_PROMPT_ECHO_ON or PAM_PROMPT_ECHO_OFF.
         */
        msg_copy = strdup(this_msg->msg);
        if(msg_copy == NULL)
            goto error;
        msg_len = strlen(msg_copy);
        if(msg_len > 0 && msg_copy[msg_len - 1] == '\n')
            msg_copy[msg_len - 1] = '\0';

        tcgetattr(fileno(tty), &old);
        responses[i].resp_retcode = 0;
        responses[i].resp = NULL;
        switch(this_msg->msg_style)
        {
            case PAM_PROMPT_ECHO_OFF:
                memcpy(&new, &old, sizeof(old));
                new.c_lflag &= ~(ECHO);
                tcsetattr(fileno(tty), TCSANOW, &new);
            case PAM_PROMPT_ECHO_ON:
                fprintf(tty, "%s", msg_copy);
                fflush(tty);
                resp_len = afgets(&responses[i].resp, tty);
                if(this_msg->msg_style == PAM_PROMPT_ECHO_OFF)
                {
                    fprintf(tty, "\n");
                    tcsetattr(fileno(tty), TCSANOW, &old);
                }
                if(resp_len == -1)
                    goto error;
                if(resp_len > 0 && responses[i].resp[resp_len - 1] == '\n')
                    responses[i].resp[resp_len - 1] = 0;
                break;
            case PAM_ERROR_MSG:
            case PAM_TEXT_INFO:
                fprintf(tty, "%s\n", msg_copy);
                break;
        }
        free(msg_copy);
        msg_copy = NULL;
    }

    *resp = responses;
    return PAM_SUCCESS;

error:
    if(msg_copy != NULL)
        free(msg_copy);
    if(responses != NULL)
    {
        for(; i >= 0; i--)
        {
            if(responses[i].resp != NULL)
                free(responses[i].resp);
        }
        free(responses);
    }
    return PAM_SYSTEM_ERR;
}

#define ERROR_CANT_START_PAM    -2

int ChangePasswordViaPam(const char *username, FILE *tty)
{
    pam_handle_t *pamh = NULL;
    int retCode = -1;
    int error_code = 0;
    struct pam_conv conversation = { tty_conv, tty };
    if(pam_start("passwd", username, &conversation, &pamh) != PAM_SUCCESS)
    {
        retCode = ERROR_CANT_START_PAM;
        goto error;
    }
    /* Solaris needs this for authentication to pass */
    pam_set_item(pamh, PAM_TTY, "/dev/tty");

    /* Solaris needs this to change the password of local users */
#ifdef HAVE_STRUCT_PAM_REPOSITORY
    struct pam_repository files = { "files", "lwidentity", 10 };
    pam_set_item(pamh, PAM_REPOSITORY, &files);
#endif

    /* On most systems a password change is done as follows:
     * pam_chauthtok with PAM_PRELIIM_CHECK
        - some module prompts for the current password and stores it in
          PAM_OLDAUTHTOK
        - all modules verify the current password
     * pam_chauthtok with PAM_UPDATE_AUTHTOK
        - some module prompts for the new password, and asks the user to
          re-enter it. The new password is stored in PAM_AUTHTOK
        - modules try to set the new password and refer to PAM_OLDAUTHTOK if
          necessary

     * Here is how password changes are performed on Solaris:
     * pam_authenticate
        - some module prompts for the current password and stores it in
          PAM_AUTHTOK
        - all modules verify the current password
     * pam_chauthtok with PAM_PRELIIM_CHECK
        - some module prompts for the new password and stores it in
          PAM_AUTHTOK, overwriting the current password
     * pam_chauthtok with PAM_UPDATE_AUTHTOK
        - some module prompts to re-enter the new password and verifies that
          it matches PAM_AUTHTOK
        - modules try to set the new password using only PAM_AUTHTOK

     * So on non-solaris machines, just pam_chauthtok needs to be called, but
     * on Solaris machines, authenticate needs to be called first. 
     */
#ifdef HAVE_STRUCT_PAM_REPOSITORY
    /* This is a Solaris machine. pam_authenticate should be called before
     * pam_chauthtok.
     */
    error_code = pam_authenticate(pamh, 0);
    if(error_code != PAM_SUCCESS)
    {
        fprintf(stderr, "passwd: %s\n", pam_strerror(pamh, error_code));
        goto error;
    }
#endif

    error_code = pam_chauthtok(pamh, 0);
    fprintf(stderr, "passwd: %s\n", pam_strerror(pamh, error_code));
    if(error_code != PAM_SUCCESS)
        goto error;
    retCode = 0;
error:
    if(pamh != NULL)
        pam_end(pamh, error_code);
    return retCode;
}

#ifdef HAVE_CHPASS

static char 
mangle_digit(unsigned int digit)
{
    if (digit <= 9)
    {
	return '0' + digit;
    }
    else
    {
	return 'a' + (digit - 10);
    }
}

void
WblMangleAIX(unsigned int id, char buffer[9])
{
    buffer[0] = '_';
    int i;

    if (id <= 9999999)
    {
	sprintf(buffer + 1, "%07u", id);
    }
    else for (i = 7; i >= 1; i--)
    {
	buffer[i] = mangle_digit((id % 32) + (i == 1 ? 10 : 0));
	id /= 32;
    }
    
    buffer[8] = '\0';
}

int ChangePasswordViaLam(const char *username, FILE *tty)
{
    int retCode = -1;
    char *response = NULL;
    int reenter = 1;
    int error_code;
    char *message;
    ssize_t resp_len;
    struct termios old, new;
    struct passwd *userinfo;
    uid_t uid;
    char fixedMessage[1024];
    char encodedUsername[10];

    userinfo = getpwnam(username);
    if (userinfo == NULL)
    {
        fprintf(stderr, "passwd: user '%s' unknown\n", username);
        goto error;
    }
    uid = userinfo->pw_uid;

    WblMangleAIX((unsigned int) uid, encodedUsername);

    tcgetattr(fileno(tty), &old);
    while(1)
    {
        error_code = chpass((char *)username, response, &reenter, &message);
        if(message != NULL)
        {
            //AIX tends to use encoded usernames in the form _0<uid> when the
            //username is too long. AIX will even put these in the messages it
            //returns. To get around this, we'll do a string replace on the
            //messages.
            char *foundPos = strstr(message, encodedUsername);
            if(foundPos != NULL)
            {
                int firstSectionLen = foundPos - message;
                if(firstSectionLen > sizeof(fixedMessage) - 1)
                    firstSectionLen = sizeof(fixedMessage) - 1;
                strncpy(fixedMessage, message, firstSectionLen);
                fixedMessage[firstSectionLen] = 0;
                strncat(fixedMessage, username,
                        sizeof(fixedMessage) - strlen(fixedMessage) -1);
                strncat(fixedMessage, foundPos + strlen(encodedUsername),
                        sizeof(fixedMessage) - strlen(fixedMessage) -1);
            }
            else
            {
                strncpy(fixedMessage, message, sizeof(fixedMessage) - 1);
                fixedMessage[sizeof(fixedMessage) - 1] = 0;
            }

            fprintf(tty, "%s", fixedMessage);
            if(fixedMessage[strlen(fixedMessage) - 1] != '\n' && !reenter)
                fprintf(tty, "\n");
            fflush(tty);
        }
        if(reenter)
        {
            if(response != NULL)
            {
                free(response);
                response = NULL;
            }
            if(message != NULL)
            {
                memcpy(&new, &old, sizeof(old));
                new.c_lflag &= ~(ECHO);
                tcsetattr(fileno(tty), TCSANOW, &new);
                resp_len = afgets(&response, tty);
                fprintf(tty, "\n");
                tcsetattr(fileno(tty), TCSANOW, &old);
                if(resp_len == -1)
                    goto error;
                if(resp_len > 0 && response[resp_len - 1] == '\n')
                    response[resp_len - 1] = 0;
            }
        }
        else
                break;
    }

    if(error_code == -1)
    {
        fprintf(stderr, "passwd: %s\n", strerror(errno));
        goto error;
    }
    else if(error_code != 0)
    {
        fprintf(stderr, "passwd: password change failed\n");
        goto error;
    }
    fprintf(stderr, "passwd: password change successful\n");
    retCode = 0;
error:
    if(response != NULL)
        free(response);
    return retCode;
}
#endif

int ChangePasswordViaLsass(const char *username, FILE *tty)
{
    int error_code = 0;
    struct pam_message message = { 0 };
    CONST_PAM_MESSAGE *message_ptr = &message;
    struct pam_response *responses = NULL;
    char *old_password = NULL;
    char *new_password = NULL;
    char *reenter_password = NULL;
    HANDLE lsa = NULL;

    message.msg_style = PAM_PROMPT_ECHO_OFF;

    message.msg = "Current password: ";
    if (tty_conv(1, &message_ptr, &responses, tty) != PAM_SUCCESS)
    {
        goto errno_failure;
    }
    old_password = responses[0].resp;
    free(responses);
    responses = NULL;

    while (1)
    {
        message.msg = "New password: ";
        if (tty_conv(1, &message_ptr, &responses, tty) != PAM_SUCCESS)
        {
            goto errno_failure;
        }
        new_password = responses[0].resp;
        free(responses);
        responses = NULL;

        message.msg = "Re-enter password: ";
        if (tty_conv(1, &message_ptr, &responses, tty) != PAM_SUCCESS)
        {
            goto errno_failure;
        }
        reenter_password = responses[0].resp;
        free(responses);
        responses = NULL;

        if (!strcmp(new_password, reenter_password))
        {
            break;
        }
        else
        {
            fprintf(tty, "Error: passwords do not match\n");

            memset(new_password, 0, strlen(new_password));
            free(new_password);
            new_password = NULL;
            memset(reenter_password, 0, strlen(reenter_password));
            free(reenter_password);
            reenter_password = NULL;
        }
    }

    error_code = LsaOpenServer(&lsa);
    if (error_code != 0)
    {
        goto lsass_failure;
    }

    error_code = LsaChangePassword(lsa, username, new_password, old_password);
    if (error_code != 0)
    {
        goto lsass_failure;
    }

    error_code = LsaCloseServer(lsa);
    lsa = NULL;
    if (error_code != 0)
    {
        goto lsass_failure;
    }

cleanup:
    free(responses);
    if (old_password)
    {
        memset(old_password, 0, strlen(old_password));
        free(old_password);
    }
    if (new_password)
    {
        memset(new_password, 0, strlen(new_password));
        free(new_password);
    }
    if (reenter_password)
    {
        memset(reenter_password, 0, strlen(reenter_password));
        free(reenter_password);
    }
    if (lsa != NULL)
    {
        LsaCloseServer(lsa);
    }
    return error_code;

errno_failure:
    fprintf(stderr, "passwd: %s\n", strerror(errno));
    error_code = -1;
    goto cleanup;

lsass_failure:
    fprintf(stderr, "passwd: %s [%d]\n%s\n",
            LwWin32ExtErrorToName(error_code),
            error_code,
            LwWin32ExtErrorToDescription(error_code));
    error_code = -1;
    goto cleanup;
}

int main(int argc, char *argv[])
{
    int showHelp = 0;
    FILE *tty = fopen("/dev/tty", "r+");
    char *username = NULL;
    int exit_code = 1;
    int argIndex = 1;
    int lsassDirect = 0;

    for (; argIndex < argc; argIndex++)
    {
        if (strcmp(argv[argIndex], "-h") == 0 ||
            strcmp(argv[argIndex], "-?") == 0 ||
            strcmp(argv[argIndex], "--help") == 0)
        {
            showHelp = 1;
        }
        else if(strcmp(argv[argIndex], "-l") == 0 ||
            strcmp(argv[argIndex], "--lsass") == 0)
        {
            lsassDirect = 1;
        }
        else if (username == NULL)
        {
            if (getuid() != 0)
            {
                fprintf(stderr, "Only root can change the password of other users\n");
                goto error;
            }
            else
                username = strdup(argv[argIndex]);
        }
        else
        {
            showHelp = 1;
        }
    }

    if (!username)
    {
        uid_t uid = getuid();
        struct passwd *calling_user = getpwuid(uid);
        if (calling_user == NULL)
        {
            fprintf(stderr, "Unable to lookup caller's username\n");
            goto error;
        }
        username = strdup(calling_user->pw_name);
    }

    if(showHelp)
    {
        printf(
"%s [options] {username}\n"
"Changes the password of a user. If the username is not specified on the\n"
"command line, the current user's password will be changed.\n"
"\n"
"Options:\n"
" --help, -h, -?\n"
"    Shows this message\n"
"\n"
" --lsass, -l\n"
"    By-passes pam and calls lsass directly. This option is used to work\n"
"    around systems with a broken pam implementation, but it will only work\n"
"    for AD users.\n",
            argv[0]);
        goto error;
    }

    if (tty == NULL)
    {
        fprintf(stderr, "Unable to open /dev/tty\n");
        goto error;
    }
    if (!isatty(fileno(tty)))
    {
        fprintf(stderr, "/dev/tty is not a terminal\n");
        goto error;
    }

    if (lsassDirect)
    {
        exit_code = ChangePasswordViaLsass(username, tty);
    }
    else
    {
        exit_code = ChangePasswordViaPam(username, tty);
#ifdef HAVE_CHPASS
        if (exit_code < 0)
        {
            exit_code = ChangePasswordViaLam(username, tty);
        }
#endif
    }
    if (exit_code == ERROR_CANT_START_PAM)
    {
        fprintf(stderr, "Unable to start pam\n");
    }

error:
    if (tty != NULL)
        fclose(tty);
    if (username != NULL)
        free(username);
    return exit_code;
}
