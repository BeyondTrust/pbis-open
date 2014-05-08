#include <stdio.h>
#include <syslog.h>

int main()
{
	int levels[] = {
	       LOG_EMERG,
	       LOG_ALERT,
	       LOG_CRIT,
	       LOG_ERR,
	       LOG_WARNING,
	       LOG_NOTICE,
	       LOG_INFO,
	       LOG_DEBUG,
	};

	openlog("level-test", 0, LOG_DAEMON);
	int i = 0;
	for(i = 0; i<sizeof(levels)/sizeof(levels[0]); i++)
	{
		syslog(i, "test level %d", i);
	}
	return 0;
}
