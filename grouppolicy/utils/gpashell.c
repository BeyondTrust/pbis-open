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

#include "includes.h"

#define SIGIL '%'

#ifndef HAVE__NSGETENVIRON
extern char **environ;
#endif

typedef GPADynamicArray StringBuffer;

typedef struct
{
    int fds[2];
    GPAShellVar* variable;
    StringBuffer buffer;
} Pipe;

typedef struct
{
    StringBuffer buffer;
    GPAHashTable* variables;
    GPADynamicArray pipes;
    unsigned int num_pipes;
} Command;

struct GPAShellVar __GPAVarInteger(const char* name, int value)
{
    GPAShellVar result;
    result.type = SVAR_INT;
    result.name = name;
    result.value.integer = value;

    return result;
}

struct GPAShellVar __GPAVarString(const char* name, const char* value)
{
    GPAShellVar result;
    result.type = SVAR_STR;
    result.name = name;
    result.value.string = value;

    return result;
}

struct GPAShellVar __GPAVarArray(const char* name, char const * const * value)
{
    GPAShellVar result;
    result.type = SVAR_ARR;
    result.name = name;
    result.value.array = value;

    return result;
}

struct GPAShellVar __GPAVarOut(const char* name, char** value)
{
    GPAShellVar result;
    result.type = SVAR_OUT;
    result.name = name;
    result.value.out = value;

    return result;
}

struct GPAShellVar __GPAVarZero(const char* name)
{
    GPAShellVar result;
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

    LW_SAFE_FREE_STRING(key);
}

static CENTERROR
CountVariables(const char* format, unsigned int* result)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    GPAHashTable* table = NULL;
    unsigned int i;

    table = gpa_hash_table_new_full (gpa_str_hash, gpa_str_equal,
				     (GPADestroyNotify)free_variable_key,
				     NULL);

    if (!table)
    {
        ceError = CENTERROR_OUT_OF_MEMORY;
        BAIL_ON_GPA_ERROR(ceError);
    }

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
            
            ceError = LwStrndup(format+i, length, &variable);
            BAIL_ON_GPA_ERROR (ceError);
            
            if (!gpa_hash_table_lookup(table, variable+1))
            {
                gpa_hash_table_insert(table, variable+1, variable+1);
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

    *result = gpa_hash_table_size(table);

error:
    if (table)
        gpa_hash_table_destroy(table);

    return ceError;
}

static void
FreeVariableEntry(gpapointer key, gpapointer value, gpapointer data)
{
    LwFreeMemory(value);
}

static void
DestroyVariableTable(GPAHashTable* table)
{
    gpa_hash_table_foreach(table, FreeVariableEntry, NULL);
    gpa_hash_table_destroy(table);
}

static CENTERROR
BuildVariableTable(unsigned int size, va_list ap, GPAHashTable** table)
{   
    unsigned int i;
    CENTERROR ceError = CENTERROR_SUCCESS;
    *table = gpa_hash_table_new(gpa_str_hash, gpa_str_equal);

    if (!*table)
    {
        ceError = CENTERROR_OUT_OF_MEMORY;
        BAIL_ON_GPA_ERROR(ceError);
    }

    for (i = 0; i < size; i++)
    {
        GPAShellVar *var;

        ceError = LwAllocateMemory(sizeof(GPAShellVar), (PVOID*)&var);
        BAIL_ON_GPA_ERROR(ceError);
        
        *var = va_arg(ap, GPAShellVar);

        gpa_hash_table_insert(*table, (void*) var->name, var);
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

static CENTERROR NullTerminate(StringBuffer *buffer)
{
    CENTERROR ceError = GPAArrayAppend(buffer, 1, "\0", 1);
    BAIL_ON_GPA_ERROR(ceError);
    buffer->size--;
error:
    return ceError;
}

CENTERROR
GPAStringBufferAppendLength(StringBuffer* buffer, const char* str, unsigned int length)
{
    CENTERROR ceError = GPAArrayAppend(buffer, 1, str, length);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = NullTerminate(buffer);
    BAIL_ON_GPA_ERROR(ceError);

error:
    return ceError;
}

CENTERROR
GPAStringBufferAppendChar(StringBuffer* buffer, char c)
{
    return GPAStringBufferAppendLength(buffer, &c, 1);
}

CENTERROR
GPAStringBufferAppend(StringBuffer* buffer, const char* str)
{
    return GPAStringBufferAppendLength(buffer, str, strlen(str));
}

CENTERROR
GPAStringBufferConstruct(StringBuffer* buffer)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    ceError = GPAArrayConstruct(buffer, 1);
    BAIL_ON_GPA_ERROR(ceError);

error:

    return ceError;
}

void
GPAStringBufferDestroy(StringBuffer* buffer)
{
    GPAArrayFree(buffer);
}


static CENTERROR
AppendStringEscaped(StringBuffer* buffer, const char* str, int length, BOOLEAN inDouble)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned int i;

    if (inDouble)
    {
	    ceError = GPAStringBufferAppendChar(buffer, '\"');
	    BAIL_ON_GPA_ERROR(ceError); 
    }

    ceError = GPAStringBufferAppendChar(buffer, '\'');
    BAIL_ON_GPA_ERROR(ceError);
    
    for (i = 0; str[i] && (length == -1 || i < (unsigned int) length); i++)
    {
		if (str[i] == '\'')
		{
			ceError = GPAStringBufferAppendChar(buffer, '\'');
			BAIL_ON_GPA_ERROR(ceError);

			ceError = GPAStringBufferAppendChar(buffer, '\\');
			BAIL_ON_GPA_ERROR(ceError);

			ceError = GPAStringBufferAppendChar(buffer, '\'');
			BAIL_ON_GPA_ERROR(ceError);

			ceError = GPAStringBufferAppendChar(buffer, '\'');
			BAIL_ON_GPA_ERROR(ceError);

		}
		else
		{
			ceError = GPAStringBufferAppendChar(buffer, str[i]);
			BAIL_ON_GPA_ERROR(ceError);
		}
    }
    
    ceError = GPAStringBufferAppendChar(buffer, '\'');
    BAIL_ON_GPA_ERROR(ceError);

    if (inDouble)
    {
	    ceError = GPAStringBufferAppendChar(buffer, '\"');
	    BAIL_ON_GPA_ERROR(ceError);
    }

error:
    return ceError;
}

static void
FreePipe(Pipe* pipe)
{
    if (pipe->buffer.data)
        GPAStringBufferDestroy(&pipe->buffer);
    if (pipe->fds[0] >= 0)
        close(pipe->fds[0]);
    if (pipe->fds[1] >= 0)
        close(pipe->fds[1]);
    LwFreeMemory(pipe);
}

static CENTERROR
AppendVariable(Command* result, struct GPAShellVar* var, BOOLEAN inDouble)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    char* str = NULL;
    char const * const * ptr = NULL;
    Pipe* ppipe = NULL;
    int fd;
    
    if (!var)
    {
        ceError = CENTERROR_SHELL_VARIABLE_UNKNOWN;
        BAIL_ON_GPA_ERROR(ceError);
    }

    switch (var->type)
    {
    case SVAR_INT:
        LwAllocateStringPrintf(&str, "%i", var->value.integer);
        BAIL_ON_GPA_ERROR(ceError);
        GPAStringBufferAppend(&result->buffer, str);
        BAIL_ON_GPA_ERROR(ceError);
        break;
    case SVAR_STR:
        ceError = AppendStringEscaped(&result->buffer, var->value.string, -1, inDouble);
        BAIL_ON_GPA_ERROR(ceError);
        break;
    case SVAR_ARR:
        for (ptr = var->value.array; *ptr; ptr++)
        {
            if (ptr != var->value.array)
            {
                ceError = GPAStringBufferAppendChar(&result->buffer, ' ');
                BAIL_ON_GPA_ERROR(ceError);
            }
            
            ceError = AppendStringEscaped(&result->buffer, *ptr, -1, inDouble);
            BAIL_ON_GPA_ERROR(ceError);
        }
        break;
    case SVAR_OUT:
        ceError = LwAllocateMemory(sizeof(Pipe), (PVOID*)&ppipe);
        BAIL_ON_GPA_ERROR(ceError);
        ceError = GPAArrayAppend(&result->pipes, sizeof(Pipe*), &ppipe, 1);
        BAIL_ON_GPA_ERROR(ceError);

        ppipe->variable = var;
        if (pipe(ppipe->fds))
        {
            ceError = LwMapErrnoToLwError(errno);
            BAIL_ON_GPA_ERROR(ceError);
        }

        ceError = GPAStringBufferConstruct(&ppipe->buffer);
        BAIL_ON_GPA_ERROR(ceError);

        fd = result->pipes.size - 1 + 3;

        ceError = LwAllocateStringPrintf(&str, "&%i %i>&-", fd, fd);
        BAIL_ON_GPA_ERROR(ceError);

        ceError = GPAStringBufferAppend(&result->buffer, str);
        BAIL_ON_GPA_ERROR(ceError);

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
    LW_SAFE_FREE_STRING(str);
    return ceError;
}

static CENTERROR
CommandConstruct(Command* command)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    memset(command, 0, sizeof(Command));

    ceError = GPAStringBufferConstruct(&command->buffer);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = GPAArrayConstruct(&command->pipes, sizeof(Pipe*));
    BAIL_ON_GPA_ERROR(ceError);
error:
    if (ceError)
    {
        if (command->buffer.data)
            GPAStringBufferDestroy(&command->buffer);
        if (command->pipes.data)
            GPAArrayFree(&command->pipes);
    }
    return ceError;
}

static void
CommandDestroy(Command* command)
{
    if (command->buffer.data)
    {
        GPAStringBufferDestroy(&command->buffer);
    }
    if (command->pipes.data)
    {
        Pipe** pipes = (Pipe**) command->pipes.data;
        int i;

        for (i = 0; i < command->pipes.size; i++)
            FreePipe(pipes[i]);
        GPAArrayFree(&command->pipes);
    }
    if (command->variables)
    {
        DestroyVariableTable(command->variables);
        command->variables = NULL;
    }
}

char*
GPAStringBufferFreeze(StringBuffer* buffer)
{
    char* data = buffer->data;
    buffer->size = buffer->capacity = 0;
    buffer->data = NULL;

    return data;
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
	    ceError = GPAStringBufferAppendChar(buffer, format[i]);
	    BAIL_ON_GPA_ERROR(ceError);
	    inDouble = !inDouble;
	    break;
        case '\'':
            length = QuoteLength(format, i);	    
            ceError = AppendStringEscaped(buffer, format+i+1, length-2, inDouble);
            BAIL_ON_GPA_ERROR(ceError);
            i += length-1;
            break;
        case '\\':
            i++;
            if (NeedsEscape(format[i], inDouble))
            {
                ceError = GPAStringBufferAppendChar(buffer, '\\');
                BAIL_ON_GPA_ERROR(ceError);
            }
            ceError = GPAStringBufferAppendChar(buffer, format[i]);
            BAIL_ON_GPA_ERROR(ceError);
            break;
        case SIGIL:
            length = VarLength(format, i);
            ceError = LwStrndup(format+i, length, &variable);
            BAIL_ON_GPA_ERROR(ceError);
            ceError = AppendVariable(result, gpa_hash_table_lookup(result->variables, 
                                     (void*) (variable+1)), inDouble);
            BAIL_ON_GPA_ERROR(ceError);
            i += length-1;
	    LW_SAFE_FREE_STRING(variable);
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
	    ceError = GPAStringBufferAppendChar(buffer, format[i]);
	    BAIL_ON_GPA_ERROR(ceError);
	    break;
        default:
            if (NeedsEscape(format[i], inDouble))
            {
                ceError = GPAStringBufferAppendChar(buffer, '\\');
                BAIL_ON_GPA_ERROR(ceError);
            }
            ceError = GPAStringBufferAppendChar(buffer, format[i]);
            BAIL_ON_GPA_ERROR(ceError);
            break;
        }
    }

error:
    LW_SAFE_FREE_STRING(variable);
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
        ceError = LwMapErrnoToLwError(errno);
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
                        {
                            ceError = LwMapErrnoToLwError(errno);
                            BAIL_ON_GPA_ERROR(ceError);
                        }
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
            {
				ceError = LwMapErrnoToLwError(errno);
				BAIL_ON_GPA_ERROR(ceError);
            }

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
							{
								ceError = LwMapErrnoToLwError(errno);
								BAIL_ON_GPA_ERROR(ceError);
							}
                        }

                        if (!amount)
                        {
                            while (close(pipes[i]->fds[0]))
                            {
                                if (errno != EAGAIN)
								{
									ceError = LwMapErrnoToLwError(errno);
									BAIL_ON_GPA_ERROR(ceError);
								}
                            }
                            pipes[i]->fds[0] = -1;
                            *(pipes[i]->variable->value.out) = GPAStringBufferFreeze(&pipes[i]->buffer);
                        }
                        else
                        {
                            buffer[amount] = 0;
                            ceError = GPAStringBufferAppend(&pipes[i]->buffer, buffer);
                            BAIL_ON_GPA_ERROR( ceError);
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
    {
	    ceError = LwMapErrnoToLwError(errno);
	    BAIL_ON_GPA_ERROR(ceError);
    }
	else if (status)
    {
	    ceError = CENTERROR_COMMAND_FAILED;
	    BAIL_ON_GPA_ERROR(ceError);
    }
    }

error:
    return ceError;
}

CENTERROR
GPAShellEx(char * const envp[], const char* format, ...)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned int numvars = 0;
    va_list ap;
    Command command;

    va_start(ap, format);

    ceError = CommandConstruct(&command);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = CountVariables(format, &numvars);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = BuildVariableTable(numvars, ap, &command.variables);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = ConstructShellCommand(format, &command);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = ExecuteShellCommand(envp, &command);
    BAIL_ON_GPA_ERROR(ceError);

error:
    va_end (ap);

    CommandDestroy(&command);

    return ceError;
}

CENTERROR
GPAShell(const char* format, ...)
{
    CENTERROR ceError = CENTERROR_SUCCESS;
    unsigned int numvars = 0;
    va_list ap;
    Command command;

    va_start(ap, format);

    ceError = CommandConstruct(&command);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = CountVariables(format, &numvars);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = BuildVariableTable(numvars, ap, &command.variables);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = ConstructShellCommand(format, &command);
    BAIL_ON_GPA_ERROR(ceError);
    ceError = ExecuteShellCommand(NULL, &command);
    BAIL_ON_GPA_ERROR(ceError);

error:
    va_end (ap);

    CommandDestroy(&command);

    return ceError;
}
