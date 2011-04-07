#
# Copyright (c) Brian Koropoff
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the MakeKit project nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#

BEGIN {
    in_doc = 0;
    saved_len = 0;
    explicit = 0;
    explicit_func = 0;
    explicit_var = 0;
}

/^#</ {
    in_doc = 1;
    next;
}

/^#>/ {
    in_doc = 0;
    if (explicit) {
        if (explicit_func) {
            process_func(explicit_func);
            explicit_func = 0;
        } else if (explicit_var) {
            process_var(explicit_var);
            explicit_var = 0;
        }       
        explicit = 0;
        saved_len = 0;
    }
    next;
}

/^# ?@function/ {
    if (in_doc) {
         sub(/^# ?@function */, "");
         explicit = 1;
         explicit_func = $0;
    }
    next;
}

/^# ?@var/ {
    if (in_doc) {
         sub(/^# ?@var */, "");
         explicit = 1;
         explicit_var = $0;
    }
}

/^#/ {
    if (in_doc) {
        sub(/^# ?/, "");
        saved[saved_len++] = $0;
    }
    next;
}

/^[a-zA-Z0-9_]+\(\)/ {
    if (saved_len > 0) {
        sub("\\(.*", "");
        process_func($0);
        saved_len = 0;
    }
    next;
}

function quote(str) {
    gsub("\\\\", "\\\\", str);
    gsub("\"", "\\\"", str);
    return str;
}

function process_func(func_name) {
    brief=""
    usage_len = 0;
    desc_len = 0;
    example_len = 0;
    option_len = 0;
    for (i = 0; i < saved_len; i++) {
        line = saved[i];
        if (line ~ /^@brief/) {
            sub("^@brief *", "", line);
            brief = line;
        } else if (line ~ /^@usage/) {
            sub("^@usage *", "", line);
            usage[usage_len++] = line;
        } else if (line ~ /^@option/) {
            sub("^@option *", "", line)
            while (i+1 < saved_len && saved[i+1] != "" && !(saved[i+1] ~ /^@/))
                line = line " " saved[++i];
            option[option_len++] = line;
        } else if (line ~ /^@example/) {
            while (i+1 < saved_len && !(saved[i+1] ~ /^@endexample/))
                example[example_len++] = saved[++i];
        } else if (!(line ~ /^@endexample/)) {
            desc[desc_len++] = line;
        }
    }

    printf("<function name=\"%s\" brief=\"%s\">\n", func_name, quote(brief));
    for (i = 0; i < usage_len; i++) {
        printf("<usage>\n");
        args_len = split(usage[i], args);
        for (j = 1; j <= args_len; j++) {
            if (args[j] ~ "=") {
                split(args[j], parts, "=");
                printf("<param key=\"%s\">%s</param>\n", parts[1], parts[2]);
            } else if (args[j] ~ "\\.\\.\\.$") {
                arg = args[j];
                sub("\\.\\.\\.$", "", arg);
                printf("<param repeat=\"yes\">%s</param>\n", arg);
            } else if (args[j] == "--") {
                printf("<paramsep/>\n");
            } else {
                printf("<param>%s</param>\n", args[j]);
            }
        }
        printf("</usage>\n");
    }
    for (i = 0; i < option_len; i++) {
        split(option[i], parts);
        part_count = split(parts[1], parts, "=");
        if (part_count >= 2) {
            printf("<option key=\"%s\" param=\"%s\">\n", parts[1], parts[2]);
        } else {
            printf("<option param=\"%s\">\n", parts[1]);
        }
        
        text = option[i];
        sub("[^ ]* ", "", text);
        print text;
        printf("</option>\n");
    }
    printf("<description>\n<para>\n");
    for (i = 0; i < desc_len && desc[i] == ""; i++);
    new_para = 0;
    for (; i < desc_len; i++) {
        if (desc[i] == "") {
            new_para = 1;
        } else {
            if (new_para) {
                printf("</para>\n<para>\n");
                new_para = 0;
            }
            print desc[i];
        }
    }
    printf("</para>\n</description>\n");
    if (example_len > 0) {
        printf("<example>\n")
        for (i = 0; i < example_len; i++) {
            print example[i];
        }
        printf("</example>\n");;
    }
    printf("</function>\n");
}


function process_var(var_name) {
    brief=""
    usage_len = 0;
    desc_len = 0;
    value_len = 0;
    is_exported = 0;
    is_inherited = 0;
    is_system = 0;
    is_output = 0;
    for (i = 0; i < saved_len; i++) {
        line = saved[i];
        if (line ~ /^@brief/) {
            sub("^@brief *", "", line);
            brief = line;
        } else if (line ~ /^@value/) {
            sub("^@value *", "", line)
            while (i+1 < saved_len && saved[i+1] != "" && !(saved[i+1] ~ /^@/))
                line = line " " saved[++i];
            value[value_len++] = line;
        } else if (line ~ /^@export/) {
            is_exported = 1;
        } else if (line ~ /^@inherit/) {
            is_inherited = 1;
        } else if (line ~ /^@system/) {
            is_system = 1;
        } else if (line ~ /^@output/) {
            is_output = 1;
        } else {
            desc[desc_len++] = line;
        }
    }

    printf("<variable name=\"%s\" brief=\"%s\"", var_name, quote(brief));
    if (is_exported) {
        printf(" export=\"true\"");
    }
    if (is_inherited) {
        printf(" inherit=\"true\"");
    }
    if (is_system) {
        printf(" system=\"true\"");
    }
    if (is_output) {
        printf(" output=\"true\"");
    }
    printf(">\n");
    for (i = 0; i < value_len; i++) {
         split(value[i], parts);
         printf("<value val=\"%s\">\n", quote(parts[1]))
         text = value[i];
         sub("[^ ]* ", "", text);
         print text;
         printf("</value>\n");
    }
    printf("<description>\n<para>\n");
    for (i = 0; i < desc_len && desc[i] == ""; i++);
    new_para = 0;
    for (; i < desc_len; i++) {
        if (desc[i] == "") {
            new_para = 1;
        } else {
            if (new_para) {
                printf("</para>\n<para>\n");
                new_para = 0;
            }
            print desc[i];
        }
    }
    printf("</para>\n</description>\n");
    printf("</variable>\n");
}