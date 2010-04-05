/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "config.h"
#include "ctshell.h"
#include "ctarray.h"
#include <sys/types.h>
#include <sys/wait.h>

#define SIGIL '%'

#ifndef HAVE__NSGETENVIRON
extern char **environ;
#endif

typedef struct
{
    int fds[2];
    CTShellVar* variable;
    StringBuffer buffer;
} Pipe;

typedef struct
{
    StringBuffer buffer;
    LWGHashTable* variables;
    DynamicArray pipes;
    unsigned int num_pipes;
} Command;

struct CTShellVar __CTVarInteger(const char* name, int value)
{
    CTShellVar result;
    result.type = SVAR_INT;
    result.name = name;
    result.value.integer = value;

    return result;
}

struct CTShellVar __CTVarString(const char* name, const char* value)
{
    CTShellVar result;
    result.type = SVAR_STR;
    result.name = name;
    result.value.string = value;

    return result;
}

struct CTShellVar __CTVarArray(const char* name, char const * const * value)
{
    CTShellVar result;
    result.type = SVAR_ARR;
    result.name = name;
    result.value.array = value;

    return result;
}

struct CTShellVar __CTVarOut(const char* name, char** value)
{
    CTShellVar result;
    result.type = SVAR_OUT;
    result.name = name;
    result.value.out = value;

    return result;
}

struct CTShellVar __CTVarZero(const char* name)
{
    CTShellVar result;
    result.type = SVAR_ZERO;
    result.name = name;

    return result;
}

static unsigned int
VarLength(const char* format, unsigned int offset)
{
    unsigned int i;
    
    for (i = offset+1; format[i]; i++)
    {
        if (!isalnum((int) format[i]) && format[i] != '_')
            break;
    }
    
    return i - offset;
}

static unsigned int
QuoteLength(const char* string, unsigned int index)
{
    unsigned int i;
    for (i = index+1; string[i]; i++)
    {
        if (string[i] == '\\' && string[i+1])
            i++;
        else if (string[i] == '\'')
            break;
    }
    return i - index + (string[i] != 0);
}


static void
free_variable_key(void* _ptr)
{
    char* key = (char*) _ptr - 1;

    CTFreeString(key);
}

static CENTERROR
CountVariables(const char* format, unsigned int* result)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    LWGHashTable* table = NULL;
    unsigned int i;

    table = lwg_hash_table_new_full (lwg_str_hash, lwg_str_equal,
				     (LWGDestroyNotify)free_variable_key,
				     NULL);

    if (!table)
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_OUT_OF_MEMORY);

    for (i = 0; format[i]; i++)
    {
        switch (format[i])
        {
	case '\\':
	{
	    break;
	}
        case SIGIL:
        {
            unsigned int length = VarLength(format, i);
            char* variable;
            
            BAIL_ON_CENTERIS_ERROR (ceError = CTStrndup(format+i, length, &variable));
            
            if (!lwg_hash_table_lookup(table, variable+1))
            {
                lwg_hash_table_insert(table, variable+1, variable+1);
            }
            
            i += length - 1;
            break;
        }
        case '\'':
        {
            unsigned int length = QuoteLength(format, i);
            i += length - 1;
            break;
        }
        }
    }

    *result = lwg_hash_table_size(table);

error:
    if (table)
        lwg_hash_table_destroy(table);

    return ceError;
}

static void
FreeVariableEntry(lwgpointer key, lwgpointer value, lwgpointer data)
{
    CTFreeMemory(value);
}

static void
DestroyVariableTable(LWGHashTable* table)
{
    lwg_hash_table_foreach(table, FreeVariableEntry, NULL);
    lwg_hash_table_destroy(table);
}

static CENTERROR
BuildVariableTable(unsigned int size, va_list ap, LWGHashTable** table)
{   
    unsigned int i;
    CENTERROR ceError = CENTERROR_SUCCESS;
    *table = lwg_hash_table_new(lwg_str_hash, lwg_str_equal);

    if (!*table)
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_OUT_OF_MEMORY);

    for (i = 0; i < size; i++)
    {
        CTShellVar *var;

        BAIL_ON_CENTERIS_ERROR(
            ceError = CTAllocateMemory(sizeof(CTShellVar), (PVOID*)&var)
            );
        
        *var = va_arg(ap, CTShellVar);

        lwg_hash_table_insert(*table, (void*) var->name, var);
    }
    
error:
    if (ceError)
    {
        if (*table)
        {
            DestroyVariableTable(*table);
            *table = NULL;
        }
    }
    return ceError;
}

static BOOLEAN
NeedsEscape(char c, BOOLEAN inDouble)
{
    if (inDouble)
    {
	switch(c)
	{
	case '\"':
	    return TRUE;
	default:
	    return FALSE;
	}
    }
    else
    {
	switch (c)
	{
	case '\\':
	case '"':
	case '$':
	case '\'':
	case ' ':
	case '\t':
	case '(':
	case ')':
	case '&':
	case ';':
	case '|':
	    // Solaris treats ^ like |
	case '^':
	case '>':
	case '<':
	case '{':
	case '}':
	    return TRUE;
	default:
	    return FALSE;
	}
    }
}

static CENTERROR
AppendStringEscaped(StringBuffer* buffer, const char* str, int length, BOOLEAN inDouble)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned int i;

    if (inDouble)
    {
	BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\"'));
    }

    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\''));
    
    for (i = 0; str[i] && (length == -1 || i < (unsigned int) length); i++)
    {
	if (str[i] == '\'')
	{
	    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\''));
	    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\\'));
	    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\''));
	    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\''));
	}
	else
	{
	    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, str[i]));
	}
    }
    
    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\''));

    if (inDouble)
    {
	BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\"'));
    }

error:
    return ceError;
}

static void
FreePipe(Pipe* pipe)
{
    if (pipe->buffer.data)
        CTStringBufferDestroy(&pipe->buffer);
    if (pipe->fds[0] >= 0)
        close(pipe->fds[0]);
    if (pipe->fds[1] >= 0)
        close(pipe->fds[1]);
    CTFreeMemory(pipe);
}

static CENTERROR
AppendVariable(Command* result, struct CTShellVar* var, BOOLEAN inDouble)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char* str = NULL;
    char const * const * ptr = NULL;
    Pipe* ppipe = NULL;
    int fd;
    
    if (!var)
        BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_SHELL_VARIABLE_UNKNOWN);

    switch (var->type)
    {
    case SVAR_INT:
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&str, "%i", var->value.integer));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppend(&result->buffer, str));
        break;
    case SVAR_STR:
        BAIL_ON_CENTERIS_ERROR(ceError = AppendStringEscaped(&result->buffer, var->value.string, -1, inDouble));
        break;
    case SVAR_ARR:
        for (ptr = var->value.array; *ptr; ptr++)
        {
            if (ptr != var->value.array)
                BAIL_ON_CENTERIS_ERROR(ceError = 
                                       CTStringBufferAppendChar(&result->buffer, ' '));
            
            BAIL_ON_CENTERIS_ERROR(ceError = AppendStringEscaped(&result->buffer, *ptr, -1, inDouble));
        }
        break;
    case SVAR_OUT:
        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateMemory(sizeof(Pipe), (PVOID*)&ppipe));
        BAIL_ON_CENTERIS_ERROR(ceError = CTArrayAppend(&result->pipes, sizeof(Pipe*), &ppipe, 1));

        ppipe->variable = var;
        if (pipe(ppipe->fds))
            BAIL_ON_CENTERIS_ERROR(ceError = CTMapSystemError(errno));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferConstruct(&ppipe->buffer));

        fd = result->pipes.size - 1 + 3;

        BAIL_ON_CENTERIS_ERROR(ceError = CTAllocateStringPrintf(&str, "&%i %i>&-", fd, fd));
        BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppend(&result->buffer, str));

        ppipe = NULL;
	break;
    case SVAR_ZERO:
	break;
    default:
        break;
    }
error:
    if (ppipe)
        FreePipe(ppipe);
    CT_SAFE_FREE_STRING(str);
    return ceError;
}

static CENTERROR
CommandConstruct(Command* command)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    memset(command, 0, sizeof(Command));

    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferConstruct(&command->buffer));
    BAIL_ON_CENTERIS_ERROR(ceError = CTArrayConstruct(&command->pipes, sizeof(Pipe*)));
error:
    if (ceError)
    {
        if (command->buffer.data)
            CTStringBufferDestroy(&command->buffer);
        if (command->pipes.data)
            CTArrayFree(&command->pipes);
    }
    return ceError;
}

static void
CommandDestroy(Command* command)
{
    if (command->buffer.data)
    {
        CTStringBufferDestroy(&command->buffer);
    }
    if (command->pipes.data)
    {
        Pipe** pipes = (Pipe**) command->pipes.data;
        int i;

        for (i = 0; i < command->pipes.size; i++)
            FreePipe(pipes[i]);
        CTArrayFree(&command->pipes);
    }
    if (command->variables)
    {
        DestroyVariableTable(command->variables);
        command->variables = NULL;
    }
}

static CENTERROR
ConstructShellCommand(const char* format, Command *result)
{
    CENTERROR ceError = CENTERROR_SUCCESS;

    unsigned int i, length;
    char* variable = NULL;
    StringBuffer* buffer = &result->buffer;
    BOOLEAN inDouble = FALSE;

    for (i = 0; format[i]; i++)
    {
        switch (format[i])
        {
	case '\"':
	    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, format[i]));
	    inDouble = !inDouble;
	    break;
        case '\'':
            length = QuoteLength(format, i);	    
            BAIL_ON_CENTERIS_ERROR(ceError = AppendStringEscaped(buffer, format+i+1, length-2, inDouble));
            i += length-1;
            break;
        case '\\':
            i++;
            if (NeedsEscape(format[i], inDouble))
                BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\\'));
            BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, format[i]));
            break;
        case SIGIL:
            length = VarLength(format, i);
            BAIL_ON_CENTERIS_ERROR(ceError = CTStrndup(format+i, length, &variable));
            BAIL_ON_CENTERIS_ERROR(ceError = 
                                   AppendVariable(result, 
                                                  lwg_hash_table_lookup(result->variables, 
									(void*) (variable+1)),
						  inDouble));
            i += length-1;
	    CT_SAFE_FREE_STRING(variable);
            variable = NULL;
            break;
	// Characters to pass through unmodified to the shell
	case '&':
	case '|':
	case ';':
	case '$':
	case '(':
	case ')':
	case '{':
	case '}':
	case '>':
	case '<':
	case ' ':
	case '\t':
	    BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, format[i]));
	    break;
        default:
            if (NeedsEscape(format[i], inDouble))
                BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, '\\'));
            BAIL_ON_CENTERIS_ERROR(ceError = CTStringBufferAppendChar(buffer, format[i]));
            break;
        }
    }

error:
    CT_SAFE_FREE_STRING(variable);
    return ceError;
}

CENTERROR
ExecuteShellCommand(char * const envp[], Command* command)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    pid_t pid, wpid;

    pid = fork();
    if (pid < 0)
    {
        ceError = CTMapSystemError(errno);
        goto error;
    }
    else if (pid == 0)
    {
        // Child process
        Pipe** pipes = (Pipe**) command->pipes.data;
        unsigned int i;

        // Close unneeded fds
        for (i = 0; i < command->pipes.size; i++)
        {
            while (close(pipes[i]->fds[0]))
            {
                if (errno != EAGAIN)
                    abort();
            }
        }

        // Remap fds to lowest numbers possible
        for (i = 0; i < command->pipes.size; i++)
        {
            dup2(pipes[i]->fds[1], i+3);
            close(pipes[i]->fds[1]);
        }

	if (envp)
	    execle("/bin/sh", (const char*) "sh", (const char*) "-c", (const char*) command->buffer.data, (const char*) NULL, envp);
	else
	    execl("/bin/sh", (const char*) "sh", (const char*) "-c", (const char*) command->buffer.data, (const char*) NULL);
	exit(1);
    }
    else
    {
        int status = 0;
        while (1)
        {
            // Parent
            Pipe** pipes = (Pipe**) command->pipes.data;
            unsigned int i;
            fd_set readfds, exceptfds;
            BOOLEAN active = FALSE;
            int ready, maxfd = -1;
                      
            FD_ZERO(&readfds);
            FD_ZERO(&exceptfds);

            for (i = 0; i < command->pipes.size; i++)
            {
                if (pipes[i]->fds[1] >= 0)
                {
                    while (close(pipes[i]->fds[1]))
                    {
                        if (errno != EAGAIN)
                            BAIL_ON_CENTERIS_ERROR(ceError = CTMapSystemError(errno));
                    }
                    pipes[i]->fds[1] = -1;
                }

                if (pipes[i]->fds[0] >= 0)
                {
                    if (maxfd < pipes[i]->fds[0])
                        maxfd = pipes[i]->fds[0];
                    FD_SET(pipes[i]->fds[0], &readfds);
                    FD_SET(pipes[i]->fds[0], &exceptfds);
                    active = TRUE;
                }
            }

            if (!active)
                break;

            ready = select(maxfd+1, &readfds, NULL, &exceptfds, NULL);

            if (ready < 0 && errno != EINTR)
                BAIL_ON_CENTERIS_ERROR(ceError = CTMapSystemError(errno));

            if (ready > 0)
            {
                for (i = 0; i < command->pipes.size; i++)
                {
                    if (pipes[i]->fds[0] >= 0 && FD_ISSET(pipes[i]->fds[0], &readfds))
                    {
                        char buffer[1024];
                        int amount;
			
                        while ((amount = read(pipes[i]->fds[0], buffer, sizeof(buffer)-1)) < 0)
                        {
                            if (errno != EAGAIN)
                                BAIL_ON_CENTERIS_ERROR(ceError = CTMapSystemError(errno));
                        }

                        if (!amount)
                        {
                            while (close(pipes[i]->fds[0]))
                            {
                                if (errno != EAGAIN)
                                    BAIL_ON_CENTERIS_ERROR(ceError = CTMapSystemError(errno));
                            }
                            pipes[i]->fds[0] = -1;
                            *(pipes[i]->variable->value.out) = CTStringBufferFreeze(&pipes[i]->buffer);
                        }
                        else
                        {
                            buffer[amount] = 0;
                            BAIL_ON_CENTERIS_ERROR(
                                ceError = CTStringBufferAppend(&pipes[i]->buffer, buffer)
                                );
                        }
                    }
                }
            }
        }
	
	do
	{
	    wpid = waitpid(pid, &status, 0);
	} while (wpid != pid && errno == EINTR);
	if (wpid != pid)
	    BAIL_ON_CENTERIS_ERROR(ceError = CTMapSystemError(errno));
	else if (status)
	    BAIL_ON_CENTERIS_ERROR(ceError = CENTERROR_COMMAND_FAILED);
    }

error:
    return ceError;
}

CENTERROR
CTShellEx(char * const envp[], const char* format, ...)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned int numvars = 0;
    va_list ap;
    Command command;

    va_start(ap, format);

    BAIL_ON_CENTERIS_ERROR(ceError = CommandConstruct(&command));
    BAIL_ON_CENTERIS_ERROR(ceError = CountVariables(format, &numvars));
    BAIL_ON_CENTERIS_ERROR(ceError = BuildVariableTable(numvars, ap, &command.variables));
    BAIL_ON_CENTERIS_ERROR(ceError = ConstructShellCommand(format, &command));
    BAIL_ON_CENTERIS_ERROR(ceError = ExecuteShellCommand(envp, &command));

error:
    va_end (ap);

    CommandDestroy(&command);

    return ceError;
}

CENTERROR
CTShell(const char* format, ...)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned int numvars = 0;
    va_list ap;
    Command command;

    va_start(ap, format);

    BAIL_ON_CENTERIS_ERROR(ceError = CommandConstruct(&command));
    BAIL_ON_CENTERIS_ERROR(ceError = CountVariables(format, &numvars));
    BAIL_ON_CENTERIS_ERROR(ceError = BuildVariableTable(numvars, ap,
                &command.variables));
    BAIL_ON_CENTERIS_ERROR(ceError = ConstructShellCommand(format, &command));
    BAIL_ON_CENTERIS_ERROR(ceError = ExecuteShellCommand(NULL, &command));

error:
    va_end (ap);

    CommandDestroy(&command);

    return ceError;
}
