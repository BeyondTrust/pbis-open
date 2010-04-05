#ifndef __GPACSESUTIL_H__
#define __GPACSESUTIL_H__

#define CONFIG_FILE_PATH_SEC        "ConfigFilePath"
#define SUSE_RELEASE_FILE           "/etc/SuSE-release"
#define STATIC_PATH_BUFFER_SIZE     256

static const DWORD COMMAND_OUT_SIZE       = 4096;
static const PSTR CENTERIS_GP_DIRECTORY_  = CACHEDIR "/";

CENTERROR
IsSuSeLinux(
    PBOOLEAN pbValue
    );

#endif /* __GPACSESUTIL_H__ */

