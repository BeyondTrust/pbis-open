#ifndef __GPODEFINES_H__
#define __GPODEFINES_H__

#define GPO_FLAG_DISABLE	0x01		
#define GPO_FLAG_FORCE		0x02

#define GPO_TRUE  1
#define GPO_FALSE 0

#ifndef WIN32

#define PATH_SEPARATOR_STR "/"

#else

#define PATH_SEPARATOR_STR "\\"

#endif

#endif /* __GPODEFINES_H__ */
