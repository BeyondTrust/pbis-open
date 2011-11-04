#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctbase.h>

#include <moonunit/interface.h>

#define MU_TRY_CTERR(_e_)					\
    do {							\
	DWORD ceError = (_e_);				\
	if (ceError)						\
	{							\
	    const char* name = LwWin32ExtErrorToName(ceError);		\
	    if (!name)						\
		name = "Unknown";				\
	    MU_FAILURE("%s failed: %s (0x%x)",			\
		       #_e_, name, (unsigned int) ceError);	\
	}						        \
    } while (0)

#define MU_TRY_ERRNO(_e_)                                       \
    do {							\
	int ret = (_e_);                                        \
	if (ret != 0)						\
	{							\
	    MU_FAILURE("%s failed: %s",                         \
		       #_e_, strerror(errno));                  \
	}						        \
    } while (0)
        

MU_TEST(CTFileUtils, CTCreateDirectory)
{
    PSTR pszPath = NULL;
    PSTR pszBasePath = NULL;
    struct stat statbuf;

    MU_TRY_CTERR(CTAllocateStringPrintf(
                     &pszBasePath,
                     "/tmp/ctfileutils-%lu",
                     (unsigned long) getpid()));

    MU_TRY_CTERR(CTAllocateStringPrintf(
                     &pszPath,
                     "%s/foo/bar/bob",
                     pszBasePath));

    MU_TRY_CTERR(CTCreateDirectory(pszPath, 0700));

    MU_TRY_ERRNO(stat(pszPath, &statbuf));
    MU_ASSERT(S_ISDIR(statbuf.st_mode));
    MU_ASSERT((statbuf.st_mode & 0777) == 0700);
    MU_TRY_CTERR(CTRemoveFiles(pszBasePath, FALSE, TRUE));
}
