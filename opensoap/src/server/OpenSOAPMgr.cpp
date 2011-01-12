/*
 *-----------------------------------------------------------------------------
 * File		: OpenSOAPMgr.cpp
 * Version	: 1.0
 * Project	: OpenSOAP
 * Module	: OpenSOAP Process Manager
 * Date		: 2003/12/02
 * Author	: Conchan
 * Copyright(C) Technoface Corporation
 * http://www.technoface.co.jp/
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <sys/types.h>

#include		"PIDFactory.h"
#include		"ServerCommon.h"
#include		"SrvConfAttrHandler.h"

#include		"SrvConf.h"

using namespace std;
using namespace OpenSOAP;

#include		"OpenSOAPMgr.h"
#include <cctype>

OSPROC_STRUCT		osproc_str[U_OSPROCS];
bool			running;
bool			halting;
char			keystring[256];
PIDFactory*		pidFactory;

std::vector<std::string>	log_attrs;
std::vector<std::string>	key_attrs;


/*
 * OpenSOAPMgr main()
 */
int main(int argc, char *argv[])
{
	int				prev_pid;
	int				fork_mgr;
	int				pipe_read;
	char				pipe_buffer[8];
	int				main_fds[2];

	int				ret_opt;
	bool				use_timeout = false;
	int				timeout_sec = 30;

/*
 * Command line options
 */
	while ((ret_opt = getopt(argc, argv, "t:")) > 0){
		switch(ret_opt){
			case 't' :
				use_timeout = true;
				timeout_sec = atoi(optarg);
				break;
			default :
				break;
		}
	}

/*
 * Previous instance termination checking
 */

	if (use_timeout){
		if (timeout_sec == 0){	// Wait for termination of previous instance
			while(1){
				if (MgrVerifyKilledProc(0, "OpenSOAPMgr.pid")){
					break;
				}
				sleep(1);
			}
		}
		else{			// Timeout limited wait
			while(timeout_sec > 0){
				if (MgrVerifyKilledProc(0, "OpenSOAPMgr.pid")){
					break;
				}
				sleep(1);
				timeout_sec--;
			}
			if (timeout_sec <= 0){
				fprintf(stderr, "OpenSOAPMgr:: Timeout for termination of previous instance of OpenSOAPMgr\n");
				exit(0);
			}
		}
	}

/*
 * Duplicity checking ...
 */
	if ((prev_pid = MgrProcExist("OpenSOAPMgr.pid")) != 0){	// Already running
	/*
	 * OK, so OpenSOAPMgr is already running
	 * Run a client version, that is allowed to speak to the already running
	 * server(default) version, so that some sort of control can be exerted
	 * on the server. Care here to prevent more than 1 clent version. The server
	 * should know how many clients are connected. Well, by default thanks to
	 * accept(), only one at a given time.
	 */

	
		fprintf(stderr, "OpenSOAPMgr:: OpenSOAPMgr already running!!\n");
		exit(0);
	}
	
	/*
	 * No existing OpenSOAPMgr running, so initiate a server version
	 */

	fprintf(stderr, "OpenSOAPMgr:: Started.\n");

	MgrInit();
	
/*
 * Add stuff for user verification(root), user migration(opensoap ?), etc
 */
	if (MgrMigration() < 0){
		fprintf(stderr, "OpenSOAPMgr:: Must be root to run.Exiting.\n");
		exit(0);
	}

/*
 * Signals, anybody ?
 */
	MgrSignals();

	pipe(main_fds);
	fork_mgr = fork();
	
	if (fork_mgr == 0){
		MgrRun(main_fds);
	}
	else{
		while((pipe_read = read(main_fds[0], (char *)pipe_buffer, 1)) <= 0){
			fprintf(stderr, "Waiting\n");
		}
		if (pipe_buffer[0] == '0'){
			fprintf(stderr, "OpenSOAPMgr:: OpenSOAP Server start Failed.\n");
		}
		else{
		/*
	 	* Miller time. We are outta here!
	 	*/
			fprintf(stderr, "OpenSOAPMgr:: OpenSOAP Server started. (PID: %d)\n", fork_mgr);
		}
		close(main_fds[0]);
		close(main_fds[1]);
		exit(0);
	}
		
	return(0);
}


/*
 * MgrInit()
 */
void MgrInit()
{
	int			i;
	
	running = false;	// Manager states
	halting = false;

	for (i = 0; i < U_OSPROCS; i++){
		sprintf(osproc_str[i].prog_name, "%s/sbin/%s", PREFIX, OSProcess[i][0]);
		sprintf(osproc_str[i].prog_opt, "%s", OSProcess[i][1]);
		osproc_str[i].prog_pid = 0;	// No PID yet
		osproc_str[i].running = 0;	// Not running
	}

	return;
}

void MgrSignals()
{

	signal(SIGTERM, (void (*)(int))MgrSignalHandler);
	signal(SIGCHLD, (void (*)(int))MgrSignalHandler);

	return;
}

int MgrMigration()
{
	uid_t		this_uid;
/*
 * 1. Check that we are 'root'
 */
	this_uid = getuid();
	if (this_uid != 0){
		return(-1);
	}
/*
 * 2. Look for 'user'
 */
	/* To do */
/*
 * 3. Change to 'user'
 */
	/* To do */
	
	return(0);
}

/*
 * MgrRun()
 */
void MgrRun(int *main_fds)
{
	key_t				root_key;
	char				*params[4];
	int				i;
	int				new_pid;
	SrvConf				attrConf;
	std::string log_queryStr = "/server_conf/log/path=?";
	std::string key_queryStr = "/server_conf/security/keys/path=?";

	pidFactory = new PIDFactory();
	if (!pidFactory->makePID(OPENSOAPMGR_PID_FILE)){
		delete pidFactory;
		write(main_fds[1], "0", 1);
		close(main_fds[0]);
		close(main_fds[1]);
		exit(0);
	}
	
/*
 *-----------------------------------------------------------------------------
 * First: srvConfAttrMgr process
 *-----------------------------------------------------------------------------
 */
	params[0] = osproc_str[0].prog_name;
	if (strlen(osproc_str[0].prog_opt) > 0){
		params[1] = osproc_str[0].prog_opt;
		params[2] = NULL;
	}
	else{
		params[1] = NULL;
	}
	if (MgrForkExec(params, &new_pid, OSProcess[0][2]) < 0){
		write(main_fds[1], "0", 1);
		close(main_fds[0]);
		close(main_fds[1]);
		exit(0);
	}
	else{
		osproc_str[0].prog_pid = new_pid;
		osproc_str[0].running = true;
	}	
	fprintf(stderr, "OpenSOAPMgr:: Starting %s (PID: %d)\n", osproc_str[0].prog_name, osproc_str[0].prog_pid);
	sleep(2);	// Wait a bit...

	log_attrs.clear();
	attrConf.query(log_queryStr, log_attrs);
	log_attrs[0] += "TraceLog";
	fprintf(stderr, "OpenSOAPMgr:: Trace Log File: %s\n", log_attrs[0].c_str());
	key_attrs.clear();
	attrConf.query(key_queryStr, key_attrs);
	key_attrs[0] += "SharedKey";
	fprintf(stderr, "OpenSOAPMgr:: Shared Key File: %s\n", key_attrs[0].c_str());

	if (SharedLibPrep(key_attrs[0].c_str(), log_attrs[0].c_str(), 0, &root_key) != SHM_SUCCESS){
		fprintf(stderr, "OpenSOAPMgr:: FATAL Failed to establish trace shared resource. Exiting...\n");
		write(main_fds[1], "0", 1);	// Signal failure
		close(main_fds[0]);
		close(main_fds[1]);
		MgrHalt(SIGKILL);
		exit(0);
	}
	FileLibPrep(log_attrs[0].c_str(), false);	// Do not initialize Trace Log File

/*
 *-----------------------------------------------------------------------------
 * Next: ssmlAttrMgr, idManager, msgDrvCreator ...
 *-----------------------------------------------------------------------------
 */
	sprintf(keystring, "-k%d", root_key);
	for (i = 1; i < U_OSPROCS; i++){
		params[0] = osproc_str[i].prog_name;
		if (strlen(osproc_str[i].prog_opt) > 0){
			params[1] = keystring;
			params[2] = osproc_str[i].prog_opt;
			params[3] = NULL;
		}
		else{
			params[1] = keystring;
			params[2] = NULL;
		}
		
		if (MgrForkExec(params, &new_pid, OSProcess[i][2]) < 0){
			write(main_fds[1], "0", 1);
			close(main_fds[0]);
			close(main_fds[1]);
			MgrHalt(SIGKILL);
			SharedLibCleanup(log_attrs[0].c_str(), key_attrs[0].c_str(), false);
			exit(0);
		}
		else{
			osproc_str[i].prog_pid = new_pid;
			osproc_str[i].running = true;
		}	
		fprintf(stderr, "OpenSOAPMgr:: Starting %s (PID: %d)\n", osproc_str[i].prog_name, osproc_str[i].prog_pid);
	}
	
	running = true;
	MgrCleanZombies();
	
	write(main_fds[1], "1", 1);	// Signal independence, begin working
	
	close(main_fds[0]);
	close(main_fds[1]);

	/*
	 * And that's it !
	 * Now to kick back and catch some Zzzzzz...
	 * Ahh, the good life.
	 */
	 
	while(1){
		SharedLibForcedFlush(log_attrs[0].c_str());
		sleep(1);
	}

	return;
}

/*
 * Fork() & Exec() & Verify()
 */
int MgrForkExec(char *params[], int *pid, const char *pidFileName)
{
	int		fork_ret;
	int		loop;
	
	if ((fork_ret = fork()) < 0){
		fprintf(stderr, "OpenSOAPMgr:: MgrForkExec() FATAL Unable to fork()\n");
		return(-1);
	}
	
	if (fork_ret == 0){
		if (execv(params[0], params) < 0){
			exit(0);
		}
	}
	else{
		*pid = fork_ret;
	}
	
	loop = 0;
	while(!MgrVerifyProcPID(fork_ret, pidFileName)){
		loop++;
		if (loop == MAX_TRIES){
			fprintf(stderr, "OpenSOAPMgr:: MgrFOrkExec() FATAL Unable to start %s\n", params[0]);
			return(-1);
		}
		sleep(1);
	}
	return(0);
}

void MgrSignalHandler(int sig)
{
	int		pid;
	
	switch(sig){
		case SIGCHLD :
			if (running){
				while ((pid = waitpid(-1, NULL, WNOHANG)) > 0){
					if (!halting){
						fprintf(stderr, "OpenSOAPMgr:: WARNING: Caught signal SIGCHLD from (%d).\n", pid);
						MgrProcRestart(pid);
					}
					else{
						fprintf(stderr, "OpenSOAPMgr:: Caught signal SIGCHLD from (%d).\n", pid);
						MgrProcSetStatus(pid, false);	// Modify proper process
					}
				}
			}
			break;
		case SIGTERM :
			fprintf(stderr, "OpenSOAPMgr:: Caught signal SIGTERM.\n");
			MgrHalt(SIGTERM);
			SharedLibForcedFlush(log_attrs[0].c_str());
			SharedLibCleanup(log_attrs[0].c_str(), key_attrs[0].c_str(), false);
			delete pidFactory;
			fprintf(stderr, "OpenSOAPMgr:: Stopped.\n\n");
			exit(0);
			break;
		default :
			break;
	}
	
	return;
}

void MgrProcSetStatus(int pid, bool value)
{
	int		i;
	
	for (i = 0; i < U_OSPROCS; i++){
		if (osproc_str[i].prog_pid == pid){
			osproc_str[i].running = value;
			break;
		}
	}

	return;
}

void MgrProcRestart(int pid)
{
	int		i;
	int		fork_ret;
	char		*params[4];
	
	for (i = 0; i < U_OSPROCS; i++){
		if (osproc_str[i].prog_pid == pid){
		
			running = false;

			params[0] = osproc_str[i].prog_name;
			if (strlen(osproc_str[i].prog_opt) > 0){
				params[1] = keystring;
				params[2] = osproc_str[i].prog_opt;
				params[3] = NULL;
			}
			else{
				params[1] = keystring;
				params[2] = NULL;
			}

			fork_ret = fork();
			if (fork_ret == 0){
				execv(osproc_str[i].prog_name, params);
			}
			else{
				osproc_str[i].prog_pid = fork_ret;
				osproc_str[i].running = true;
			}
			fprintf(stderr, "OpenSOAPMgr:: Restarting %s(%d)\n", osproc_str[i].prog_name, osproc_str[i].prog_pid);
			sleep(1);
			
			running = true;
			
			break;
		}
	}
	
	return;
}

void MgrCleanZombies()
{
	int		pid;
	
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0){
		fprintf(stderr, "OpenSOAPMgr:: Cleaning Zombie(%d)\n", pid);
	}

	return;
}

void MgrHalt(int the_signal)
{
	int		i;
	int		loop;

	halting = true;
	for (i = 0; i < U_OSPROCS; i++){
		fprintf(stderr, "OpenSOAPMgr:: Halting %s (PID: %d)\n",
			osproc_str[U_OSPROCS - (i + 1)].prog_name, osproc_str[U_OSPROCS - (i + 1)].prog_pid);
		if (osproc_str[U_OSPROCS - (i + 1)].running){
			kill(osproc_str[U_OSPROCS - (i + 1)].prog_pid, the_signal);
		}
/*
 * Unused
		loop = 0;
 *
 */
		while(osproc_str[U_OSPROCS - (i + 1)].running){
			sleep(1);
			if (MgrVerifyKilledProc(osproc_str[U_OSPROCS - (i + 1)].prog_pid, OSProcess[U_OSPROCS - (i + 1)][2])){
				osproc_str[U_OSPROCS - (i + 1)].running = false;
			}
		//	osproc_str[U_OSPROCS - (i + 1)].running = false;
/*
 * Unused
			loop++;
			if (loop == MAX_TRIES){
				fprintf(stderr, "OpenSOAPMgr:: WARNING: Unable to halt %s. (PID: %d)\n",
					osproc_str[U_OSPROCS - (i + 1)].prog_name,
					osproc_str[U_OSPROCS - (i + 1)].prog_pid);
				osproc_str[U_OSPROCS - (i + 1)].running = false;
				break;
			}
 *
 */
		}
	}
	
	running = false;
	halting = false;
	
	return;
}

int MgrProcExist(const char *pidFileName)
{
	int		ret_pid;
	PIDFactory*	pidFactory;

	pidFactory = new PIDFactory();
	ret_pid = pidFactory->checkPID(pidFileName);
	delete pidFactory;

	return(ret_pid);
}

bool MgrVerifyProcPID(int pid, const char *pidFileName)
{
	bool		ret_value;
	int		ret_pid;
	PIDFactory*	pidFactInst;

	pidFactInst = new PIDFactory();
	ret_pid = pidFactInst->checkPID(pidFileName);
	delete pidFactInst;
	
	if (pid == ret_pid){
		ret_value = true;
	}
	else{
		ret_value = false;
	}
	

	return(ret_value);
}

bool MgrVerifyKilledProc(int pid, const char *pidFileName)
{
	bool		ret_value;
	bool		ret_exist;
	int		ret_pid;
	PIDFactory*	pidFactInst;

	pidFactInst = new PIDFactory();
	
//	ret_exist = pidFactInst->existPIDFile(pidFileName);
	ret_pid = pidFactInst->checkPID(pidFileName);
	delete pidFactInst;

	if (ret_pid > 0){
		ret_value = false;
	}
	else{
		ret_value = true;
	}

	return(ret_value);
}




