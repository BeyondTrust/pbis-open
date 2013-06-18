#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>	//open fstat
#include <sys/stat.h>	//open fstat
#include <stdarg.h>		//vsnprintf,vfprintf
#include <exception>
#include <syslog.h>		//LOG_XXX
#include <time.h>		//localtime time
#include <unistd.h>		//gethostname fstat
#include <errno.h>

#include "FileLogger.h"
#include "Exception.h"
using namespace std;
using namespace OpenSOAP;

static const std::string CLASS_SIG = "FileLogger::";

/*============================================================================
FileLogger CLASS 
=============================================================================*/
FileLogger::FileLogger(){
	FileName="";
	FilePointer=NULL;
	FileDescriptor=0;
	OutputLevel=LOG_INFO;
	Initialize();
}

FileLogger::~FileLogger(){
	Close();
}

/*
関数	：ログ用グローバル変数の初期化
備考	：初期化処理は起動に行われるものなので、try,catchでエラー処理をする。
戻り値	：戻り値はなし（エラー時は例外使用）
*/
void FileLogger::Initialize(){
	//初期化処理
	return ;
}

void FileLogger::SetFileName(const string& fn){
	FileName=fn;
}
void FileLogger::SetFilePointer(FILE * fp){
	FilePointer=fp;
}
string& FileLogger::GetFileName(){
	return FileName;
}
FILE * FileLogger::GetFilePointer(){
	return FilePointer;
}

void FileLogger::Open(){
	struct stat st;
	if (FileDescriptor>=2) {
		//既にオープンされている場合は、何もしない。
		return ;
	}
	
	//no set filename
	if (FileName.length()<=0) {
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("No set log filename");
		throw e;
	}

	// open log file
	FileDescriptor=open(FileName.c_str(),O_CREAT|O_RDWR);//,mode);
	if (FileDescriptor==-1) {
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File open error");
		printf("FileOpenError!");
		throw e;
	}
	
	// change file mode
	fstat(FileDescriptor,&st);
	if (st.st_mode!=0666 && st.st_uid==geteuid()&&
		fchmod(FileDescriptor,0666)) {
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File mode change error");
		throw e;
	}
}
void FileLogger::Close(){
	if (FileDescriptor<2) {
		return;
	}

	// close log file
	if (close(FileDescriptor)) {
		FileDescriptor=0;
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File close error");
		throw e;
	}
	FileDescriptor=0;
}
void FileLogger::FFlush(){
//	if (!FilePointer) {
//		return;
//	}
//	std::fflush(FilePointer);
}
//内容：先頭に移動
void FileLogger::SeekHead(){
	if (FileDescriptor<2) {
		return;
	} 
	if(lseek(FileDescriptor, 0L, SEEK_SET)==(off_t)-1){
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File seek(head) error");
		throw e;
	}
}
//内容：指定された場所に移動
void FileLogger::SeekSet(long l){
	if (FileDescriptor<2) {
		return;
	}
	if(lseek(FileDescriptor, l, SEEK_SET)==(off_t)-1){
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File seek(set) error");
		throw e;
	} 
}
//内容：末尾に移動
//注意：末尾は、変動するため、正確な末尾への移動は、
//		Lock後に移動する必要があります。
void FileLogger::SeekEnd(){
	if (FileDescriptor<2) {
		return;
	} 
	if(lseek(FileDescriptor, 0L, SEEK_END)==(off_t)-1){
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File seek(end) error");
		throw e;
	}
}
int FileLogger::GetSize(){
    struct stat fst;
    int      s=0;
    fst.st_size=0;
	
	if (FileName.length()) {
        s=stat(FileName.c_str(),&fst);   
	}
    return fst.st_size;
}
//内容：ファイルにロックをする。
void FileLogger::Lock(){
	struct flock lock;
	lock.l_type=F_WRLCK;
	lock.l_whence=0;
	lock.l_start=0;
	lock.l_len=0;
	struct stat st;
	char buf[16];
	int  lockid=0;
	int rtn=0;
//	extern int errno;
//	errno=0;

	if (FileDescriptor<2){
		return ;
	}

	while ((rtn=fcntl(FileDescriptor,F_SETLK,&lock))!=0){
		if (errno!=EACCES&&errno!=EAGAIN){
			Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
			e.SetErrText("File lock error");
			throw e;
		}
		lockid=LockID;
#ifdef _LINUX
		sprintf(buf,"/proc/$d",lockid);
		if (stat(buf,&st)){
			UnLock();
		}
		else {
			usleep(5);
		}
#endif
	}
	LockID=getpid();
}
//内容：ファイルのロックをはずす。
void FileLogger::UnLock(){
	int pid=(int)getpid();
	UnLock(pid);
}
void FileLogger::UnLock(int id){
	struct flock lock;
	lock.l_type=F_UNLCK;
	lock.l_whence=0;
	lock.l_start=0;
	lock.l_len=0;

	if (FileDescriptor<2 || id!=LockID){
		return ;
	}

	if (fcntl(FileDescriptor,F_SETLKW,&lock)){
		//unlock error
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File unlock error");
		throw e;
	}
}

//直接指定した文字を書き込む場合用
int FileLogger::RawWrite(int level,const char * fmt,va_list args){
	int wsize=0;
	int size=0;
	char buf1[4096];
	char * buf=buf1;
	char * buf2=NULL;

	if (!fmt){
		return size;
	}

	if(FileDescriptor<2) {
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File write error");
		throw e;
	}

	size=vsnprintf(buf1,4095,fmt,args);
	if (level > OutputLevel){
		return size;
	}

	//buf1のバッファサイズで足りず、
	//buf2のアロケーションもきなかった場合のために最後をNULLで閉じておく。
	*(buf1+4095)='\0';

	//buf1のサイズが足りない場合はmallocする。
	if (size >= 4095 && (buf2=(char *)malloc(size+1))!=NULL){
		*(buf2+size)='\0';
		size=vsnprintf(buf2,size+1,fmt,args);
		buf=buf2;
	}

	if ((wsize=write(FileDescriptor,buf,size))!=size){
		if (buf2) free(buf2);
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("File write error");
		throw e;
	}

	if (buf2) free(buf2);

	return size;
}

void FileLogger::Rotate(){
	char suffix[10];
	struct stat st;
	string dist=FileName;
	time_t t=time(&t);
	struct tm * tm=localtime(&t);
	bool openflg=false;

	sprintf(suffix,".%04d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday);
	dist+=suffix;

	if (!stat(dist.c_str(),&st)){
		//File Exist !
		Exception e(-1,OPENSOAP_FILE_ERROR,LOG_ERR,__FILE__,__LINE__);
		e.SetErrText("Rotate file exist");
		throw e;
	}

	if (FileDescriptor>=2) {
		openflg=true;
		Close();
	}
	rename(FileName.c_str(),dist.c_str());
	if (openflg) {
		Open();
	}
}
