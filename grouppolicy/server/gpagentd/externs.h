
#ifndef __EXTERNS_H__
#define __EXTERNS_H__

extern pthread_mutex_t ghEventLogLock;
extern HANDLE ghEventLog;

extern GPOSERVERINFO gServerInfo;

#define GPA_LOCK_SERVERINFO   pthread_mutex_lock(&gServerInfo.lock)
#define GPA_UNLOCK_SERVERINFO pthread_mutex_unlock(&gServerInfo.lock)

extern pthread_t gServerThread;
extern PVOID     pgServerThread;

extern sigset_t group_policy_signal_set;

#endif /* __EXTERNS_H__ */
