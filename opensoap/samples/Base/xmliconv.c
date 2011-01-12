/*-----------------------------------------------------------------------------
 * $RCSfile: xmliconv.c,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if (!defined(_WIN32)) || defined(_DEBUG)
static  const   char    CVS_ID[]        =       "$Id: xmliconv.c,v 1.6 2001/12/18 02:32:18 bandou Exp $";
#endif  /* _DEBUG */

#include <OpenSOAP/String.h>
#include <OpenSOAP/OpenSOAP.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif /* EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif /* EXIT_FAILURE */

const char usageString[] =
"Usage: %s -f encoding -t encofing inputfile [outputfile]\n"
"Convert encoding of given XML file from one encoding to another.\n"
"\n"
" Parameters\n"
"  inputfile            original text\n"
"  outputfile           output file. If it\'s not given, output to stdout\n"
"\n"
" Options\n"
"  -f encoding          encoding of original text\n"
"  -t encoding          encoding for output\n"
;

static
void
printUsage(void) {
    const char *cmd_name = "xmliconv";
    fprintf(stderr, usageString, cmd_name);
}

static const char OPTION_PREFIX[] = "-";

static const char OPTION_INPUTENCOGING[] = "f";
static const char OPTION_OUTPUTENCOGING[] = "t";

typedef struct {
    char *input_encoding;
    char *output_encoding;
    FILE    *input_file;
    FILE    *output_file;
} XMLIConvAppVariables;

static
int
XMLIConvAppVariablesInitialize(XMLIConvAppVariables *app_vars,
                               int argc,
                               char **argv) {
    int ret = EXIT_FAILURE;
    const size_t OPTION_PREFIX_LEN = strlen(OPTION_PREFIX);
    char **av_end = argv + argc;

    memset(app_vars, 0, sizeof(XMLIConvAppVariables));
    app_vars->output_file = stdout;

    for (++argv; argv != av_end; ++argv) {
        const char *opt_body = *argv;
        if (strncmp(opt_body, OPTION_PREFIX, OPTION_PREFIX_LEN) == 0) {
            size_t opt_body_len = 0;
            opt_body += OPTION_PREFIX_LEN;
            opt_body_len = strlen(opt_body);
            if (strncmp(OPTION_INPUTENCOGING,
                        opt_body, opt_body_len) == 0
                && !app_vars->input_encoding) {
                ++argv;
                if (argv == av_end) {
                    break;
                }
                app_vars->input_encoding = strdup(*argv);
            }
            else if (strncmp(OPTION_OUTPUTENCOGING,
                             opt_body, opt_body_len) == 0
                     && !app_vars->output_encoding) {
                ++argv;
                if (argv == av_end) {
                    break;
                }
                app_vars->output_encoding = strdup(*argv);
            }
            else {
                argv = av_end;
                break;
            }
        }
        else {
            break;
        }
    }

    if (app_vars->input_encoding && app_vars->output_encoding
        && argv != av_end) {
        FILE *i_fp = fopen(*argv, "rb");
        if (!i_fp) {
            fprintf(stderr,
                    "Cannot open file: %s\n",
                    *argv);
            printUsage();
        }
        else {
            app_vars->input_file = i_fp;
            ++argv;
            ret = EXIT_SUCCESS;
            if (argv != av_end) {
                FILE *o_fp = fopen(*argv, "wb");
                if (!o_fp) {
                    fclose(i_fp);
                    fprintf(stderr,
                            "Cannot open file: %s\n",
                            *argv);
                    printUsage();
                    ret = EXIT_FAILURE;
                }
                app_vars->output_file = o_fp;
            }
        }
    }
    else {
        printUsage();
    }

    return ret;
}

static
int
XMLIConvAppReadFile(XMLIConvAppVariables *app_vars,
                    OpenSOAPByteArrayPtr *input_buf) {
    int ret = EXIT_FAILURE;
    int error_code = OpenSOAPByteArrayCreate(input_buf);

    if (OPENSOAP_SUCCEEDED(error_code)) {
#define READ_BUF_SIZ 64
        unsigned char read_buf[READ_BUF_SIZ];
        size_t read_size = 0;
        while (OPENSOAP_SUCCEEDED(error_code)
               && (read_size
                   = fread(read_buf, 1, READ_BUF_SIZ,
                           app_vars->input_file))) {
            error_code = OpenSOAPByteArrayAppend(*input_buf,
                                                 read_buf,
                                                 read_size);
        }
        if (OPENSOAP_FAILED(error_code)) {
            fputs("file read error\n", stderr);
        }
        else {
            ret = EXIT_SUCCESS;
        }

        if (ret == EXIT_FAILURE) {
            OpenSOAPByteArrayRelease(*input_buf);
            *input_buf = NULL;
        }
#undef  READ_BUF_SIZ
    }
    else {
        fputs("Cannot create read buf\n", stderr);
    }

    return ret;
}

static
int
XMLIConvAppGetReplaceString(const char *input_encoding,
                            OpenSOAPByteArrayPtr input_buf,
                            const char *output_encoding,
                            OpenSOAPStringPtr * replace_str) {
    int ret = EXIT_FAILURE;
    int error_code
        = OpenSOAPStringCreateWithCharEncodingString(input_encoding,
                                                     input_buf,
                                                     replace_str);

    if (OPENSOAP_FAILED(error_code)) {
        fputs("Cannot create OpenSOAPString\n", stderr);
    }
    else {
        const wchar_t XMLPROC_BEGIN[] = L"<?xml";
        const wchar_t XMLPROC_END[] = L"?>";
        const wchar_t XMLENCODING[] = L"encoding=";
        size_t xmlproc_beg_idx = 0;

        ret = EXIT_SUCCESS;
        error_code
            = OpenSOAPStringFindStringWC(*replace_str,
                                         XMLPROC_BEGIN,
                                         &xmlproc_beg_idx);
        if (OPENSOAP_SUCCEEDED(error_code)
            && xmlproc_beg_idx != (size_t)(-1)) {
            size_t xmlproc_ed_idx = xmlproc_beg_idx;
            error_code
                = OpenSOAPStringFindStringWC(*replace_str,
                                             XMLPROC_END,
                                             &xmlproc_ed_idx);
            if (OPENSOAP_SUCCEEDED(error_code)
                && xmlproc_ed_idx != (size_t)(-1)) {
                size_t xmlenc_idx = xmlproc_beg_idx;
                error_code
                    = OpenSOAPStringFindStringWC(*replace_str,
                                                 XMLENCODING,
                                                 &xmlenc_idx);
                if (OPENSOAP_SUCCEEDED(error_code)
                    && xmlenc_idx != (size_t)(-1)
                    && xmlenc_idx < xmlproc_ed_idx) {
                    /* encoding find */
                    size_t enc_idx = xmlenc_idx;
                    error_code
                        = OpenSOAPStringFindStringMB(*replace_str,
                                                     input_encoding,
                                                     &enc_idx);
                    if (OPENSOAP_SUCCEEDED(error_code)
                        && enc_idx != (size_t)(-1)
                        && enc_idx < xmlproc_ed_idx) {
                        error_code
                            = OpenSOAPStringReplaceStringMB(*replace_str,
                                                            input_encoding,
                                                            output_encoding,
                                                            &enc_idx);
                        if (OPENSOAP_FAILED(error_code)
                            || enc_idx == (size_t)(-1)) {
                            fputs("XML encoding replace failure\n",
                                  stderr);
                            ret = EXIT_FAILURE;
                        }
                    }
                }                
            }
        }
        
        if (ret != EXIT_SUCCESS) {
            OpenSOAPStringRelease(*replace_str);
            *replace_str = NULL;
        }
    }

    return ret;
}

static
int
XMLIConvAppReplaceEncoding(XMLIConvAppVariables *app_vars,
                           OpenSOAPByteArrayPtr input_buf,
                           OpenSOAPByteArrayPtr *output_buf) {
    OpenSOAPStringPtr replace_str = NULL;
    int ret
        = XMLIConvAppGetReplaceString(app_vars->input_encoding,
                                      input_buf,
                                      app_vars->output_encoding,
                                      &replace_str);
    if (ret == EXIT_SUCCESS) {
        int error_code
            = OpenSOAPByteArrayCreate(output_buf);

        ret = EXIT_FAILURE;
        if (OPENSOAP_FAILED(error_code)) {
            fputs("temporay memory allocation error\n", stderr);
        }
        else {
            error_code
                = OpenSOAPStringGetCharEncodingString(replace_str,
                                                      app_vars
                                                      ->output_encoding,
                                                      *output_buf);
            if (OPENSOAP_FAILED(error_code)) {
                fputs("encoding convert error\n", stderr);
            }
            else {
                ret = EXIT_SUCCESS;
            }
            
            if (ret != EXIT_SUCCESS) {
                OpenSOAPByteArrayRelease(*output_buf);
                *output_buf = NULL;
            }
        }

        OpenSOAPStringRelease(replace_str);
    }

    return ret;
}

static
int
XMLIConvAppConvert(XMLIConvAppVariables *app_vars) {
    OpenSOAPByteArrayPtr input_buf  = NULL;
    int ret = XMLIConvAppReadFile(app_vars, &input_buf);

    if (ret == EXIT_SUCCESS) {
        OpenSOAPByteArrayPtr output_buf = NULL;
        ret = XMLIConvAppReplaceEncoding(app_vars, input_buf, &output_buf);
        if (ret == EXIT_SUCCESS) {
            const unsigned char *output_beg = NULL;
            size_t output_size = 0;
            int error_code
                = OpenSOAPByteArrayGetBeginSizeConst(output_buf,
                                                     &output_beg,
                                                     &output_size);
            if (OPENSOAP_SUCCEEDED(error_code)) {
                fwrite(output_beg, 1, output_size,
                       app_vars->output_file);
                ret = EXIT_SUCCESS;
            }
            
            OpenSOAPByteArrayRelease(output_buf);
        }        
        OpenSOAPByteArrayRelease(input_buf);
    }

    return ret;
}

int
main(int argc,
     char **argv) {
    XMLIConvAppVariables app_vars;
    int ret = EXIT_FAILURE;
    int error_code = OpenSOAPInitialize(NULL);

    if (OPENSOAP_SUCCEEDED(error_code)) {
        ret = XMLIConvAppVariablesInitialize(&app_vars,
                                             argc, argv);

        if (ret == EXIT_SUCCESS) {
            ret = XMLIConvAppConvert(&app_vars);
        }
        OpenSOAPUltimate();
    }
    

    return ret;
}
