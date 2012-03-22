#include <sys/stat.h>
#include <sys/types.h> 
#include <string>
#include <sstream>
#include <iterator>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <cctype>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>

#include <errno.h>

#include "SrvConf.h"
#include "ServerCommon.h"

#include "DataRetainer.h"
#include "AppLogger.h"
#include "Exception.h"

using namespace std;
using namespace OpenSOAP;

//#define DEBUG

string DataRetainer::def_name[] = {"Status", "HttpBody", "DataHead", "SoapEnvelope", "DataTail", "HttpBodyOld"};

DataRetainer::DataRetainer(const string& aNamePath) 
	: namePath(aNamePath)
{
	Id = "";
	
	//musk = 0x000;
	//cur_musk = umask(musk);
}

DataRetainer::~DataRetainer(void)
{
	//umask(cur_musk);
}

void DataRetainer::saveStatus()
{
	ofstream out(filename[0].c_str(), ios::binary | ios::trunc);

	if(!out.is_open())
		throw "cannot open file in DataRetainer::saveStatus: ";
	
	struct stat st;
	stat(filename[0].c_str(), &st);
	if (st.st_mode!=0666 && st.st_uid==geteuid())
		chmod(filename[0].c_str(), 0666);
	
	int entries = (int) Status.size();

	try {
		out << entries << endl;
		
		for(map<string, string>::const_iterator i = Status.begin(); i != Status.end(); i++)
			out << i->first << endl << i->second << endl;
	
		out.close();
	}
	catch(...) {
		out.close();
		throw "cannot write file in DataRetainer::saveStatus: ";
	}
}

void DataRetainer::loadStatus()
{
	ifstream in(filename[0].c_str());
	if(!in.is_open())
		return;	// exit this function
	
	try {
		string buf;
		getline(in, buf);
		int entries = atoi(buf.c_str());
		string skey;
		while(entries--) {
			getline(in, skey);
			getline(in, buf);
			Status[skey] = buf;
		}
		in.close();
	}
	catch(...) {
		in.close();
		throw "cannot read file in DataRetainer::loadStatus:";
	}
}

void DataRetainer::loadSoapEnvelope()
{
	ifstream in(filename[3].c_str(), ios::binary);
	if(!in.is_open())
		throw "cannot open file in DataRetainer::loadSoapEnvelope: ";

	try{
		stringstream strst;
		strst << in.rdbuf();
		SoapEnvelope = strst.str();
		in.close();
	}
	catch(...) {
		in.close();
		throw "cannot read file in DataRetainer::loadSoapEnvelope: ";
	}
}

void DataRetainer::saveSoapEnvelope()
{
	ofstream out(filename[3].c_str(), ios::binary);
	if(!out.is_open())
		throw "cannot open file in DataRetainer::saveSoapEnvelope: ";

	try{
		out << SoapEnvelope;;
		out.close();
	}
	catch(...) {
		out.close();
		throw "cannot write file in DataRetainer::saveSoapEnvelope: ";
	}
}

void DataRetainer::setInternalStatus(string key, string st)
{
	try {
		AddHttpHeaderElement("dr_status_" + key, st);
	}
	catch(char* e) {
		char error[256];
		strcpy(error, e); strcat(error, " through AddHttpHeaderElement");
		throw error;
	}
}

bool DataRetainer::getInternalStatus(string key, string& st)
{
	try {
		bool rt = GetHttpHeaderElement("dr_status_" + key, st);
		return rt;
	}
	catch(char* e) {
		char error[256];
		strcpy(error, e); strcat(error, " through GetHttpHeaderElement");
		throw error;
	}
}

void DataRetainer::decompose()
{
	string status;
	try {
		getInternalStatus("division", status);
	}
	catch(char* e) {
		char error[256];
		strcpy(error, e); strcat(error, " through getInternalStatus");
		throw error;
	}
		
	// if already composed, then return
	if((status=="decomposed") ||(status=="created"))
		return;

	try {
		GetHttpHeaderElement("content-type", status);
	}
	catch(char* e) {
		char error[256];
		strcpy(error, e); strcat(error, " through GetHttpHeaderElement");
		throw error;
	}
	
	if(status == "application/dime") {
		// for dime
		try {
			decomposeDime();
		}
		catch(char* e) {
			char error[256];
			strcpy(error, e); strcat(error, " through decomposeDime");
			throw error;
		}
	} else if(status.find("multipart/related") != string::npos) {
		// for mime
		try {
			decomposeMime();
		}
		catch(char* e) {
			char error[256];
			strcpy(error, e); strcat(error, " through decomposeMime");
			throw error;
		}
	} else {
		// for simple soap
		// rename HttpBody to SoapEnvelope
		try {
			rename(filename[1].c_str(), filename[3].c_str());
		}
		catch(...) {
			throw "cannot rename HttpBody file to SoapEnvelope";
		}
	}
	
	try {
		setInternalStatus("division", "decomposed");
	}
	catch(char* e) {
		char error[256];
		strcpy(error, e); strcat(error, " through setInternalStatus");
		throw error;
	}
}

void DataRetainer::compose()
{
	// if http body has created already, then return
	try {
		struct stat tmp_sb;
		int rs1 = stat(filename[1].c_str(), &tmp_sb);
		int rs3 = stat(filename[3].c_str(), &tmp_sb);

		if(rs1==-1 && rs3==-1)
			return;
	}
	catch(...) {
		throw " stat(file name) error";
	}
	
	try {
		string status;
		getInternalStatus("division", status);
		
		// if already composed, then return
		if(status=="composed")
			return;

		try {
			GetHttpHeaderElement("content-type", status);
		}
		catch(char* e) {
			char error[256];
			strcpy(error, e); strcat(error, " through GetHttpHeaderElement");
			throw error;
		}
		
		if(status == "application/dime") {
			// for dime
			try {
				composeDime();
			}
			catch(char* e) {
				char error[256];
				strcpy(error, e); strcat(error, " through composeDime");
				throw error;
			}
		} else if(status.find("multipart/related") != string::npos) {
			// for mime
			try {
				composeMime();
			}
			catch(char* e) {
				char error[256];
				strcpy(error, e); strcat(error, " through composeMime");
				throw error;
			}
		} else {
			// for simple soap
			// rename SoapEnvelope to HttpBody
			try {
				rename(filename[3].c_str(), filename[1].c_str());	
			}
			catch(...) {
				throw "cannot rename SoapEnvelope file to HttpBody";
			}
		}
	}
	catch(char* e) {
		char error[256];
		strcpy(error, e); strcat(error, " through getInternalStatus");
		throw error;
	}
		
	
	try {
		setInternalStatus("division", "composed");
	}
	catch(char* e) {
		char error[256];
		strcpy(error, e); strcat(error, " through setInternalStatus");
		throw error;
	}
}

void DataRetainer::decomposeDime()
{
	struct DimeHeader {
		unsigned char header[3][2]; // option, id, type
		unsigned char payload[4];
	} dh;

	// get header parameters from http body file
	ifstream ifs(filename[1].c_str(), ios::binary);
	
	if(!ifs.is_open())
		throw "cannot open input file in DataRetainer::decomposeDime: ";
	
	try {
		ifs.seekg(2,ios::beg);
		ifs.read((char*)&dh, sizeof(dh));
	}
	catch(...) {
		throw "cannot read file in DataRetainer::decomposeDime: ";
	}

	unsigned long header[3] = {0, 0, 0};

	int i;
	// get length of option, id, type
	for(i=0; i<3; i++) {
		for(int j=0; j<2; j++) {
			header[i] <<= 8;
			header[i] += dh.header[i][j];
		}
		//for zero padding
		if(header[i]%4)
			header[i] += (4 - header[i]%4);
	}
	
	// get soap envelope length
	unsigned long payload = 0;
	for(i=0; i<4; i++) {
		payload <<= 8;
		payload += dh.payload[i];
	}

	// zero padding length of soap envelope field
	unsigned long payload_padding = 0;
	if(payload%4)
		payload_padding = (4 - payload%4);

	// get three file length from header parameters
	unsigned long flen[3] ={12, 0, 0}; 
	
	for(i=0; i<3; i++)
		flen[0] += header[i];
	
	flen[1] = payload;
	
	ifs.seekg(0, ios::end );
	flen[2] = (unsigned long)ifs.tellg() - (flen[0] + flen[1] + payload_padding); 
	
	ifs.seekg(0, ios::beg); // rewind
	
	// split file into three files; DataHead, SoapEnvelope, DataTail
	ofstream ofs[3];
	for(i=0; i<3; i++) {
		ofs[i].open(filename[i+2].c_str(), ios::binary | ios::trunc);

		if(!ofs[i].is_open())
			throw "cannot open output file in DataRetainer::decomposeDime: ";
	}
	
	try {
		char buf[DR_SIZE_BUF];
		unsigned int size_buf;

		unsigned int pos_file;
		for(i=0; i<3; i++) {
			pos_file = 0;
			while(pos_file < flen[i]) {
				if(flen[i] - pos_file < DR_SIZE_BUF)
					size_buf = (unsigned int)flen[i] - pos_file;
				else
					size_buf = DR_SIZE_BUF;

				ifs.read(buf, size_buf);
				ofs[i].write(buf, size_buf);
				pos_file += size_buf;
			}
			if(i==1 && payload_padding>0) // for envelope 0 padding
				ifs.seekg(payload_padding, ios::cur);
		}
	}
	catch(...) {
		ifs.close();
		for(i=0; i<3; i++)
			ofs[i].close();
		throw "cannot split HttpBody file into three files";
	}

	ifs.close();
	for(i=0; i<3; i++)
		ofs[i].close();

	// rename HttpBody to HttpBodyOld
	//rename(filename[1].c_str(), filename[5].c_str());
	try {
		remove(filename[1].c_str());
	}
	catch(...) {
		throw "cannot remove HttpBody file";
	}
}

void DataRetainer::composeDime()
{
	// oepen output http body file stream
	ofstream ofs(filename[1].c_str(), ios::binary | ios::trunc);
	if(!ofs.is_open())
		throw "cannot open output file in DataRetainer::composeDime: ";

	int i;
	ifstream ifs[3];
	for(i=0; i<3; i++) {
		ifs[i].open(filename[i+2].c_str(), ios::binary);
		if(!ifs[i].is_open())
		throw "cannot open input file in DataRetainer::composeDime: ";
	}

	try {
		char buf[DR_SIZE_BUF];
		unsigned int length_payload;
		unsigned int length_padding;
		
		// join 3 files; data head, envelope, data tail
		for(i=0; i<3; i++) {
			while(!ifs[i].eof() && ofs.good()) {
				ifs[i].read(buf, DR_SIZE_BUF);
				ofs.write(buf, ifs[i].gcount());
			}
			// soap envelope zero padding
			if(i==1) {
				ifs[i].clear();
				ifs[i].seekg(0, ios::end);
				length_payload = ifs[i].tellg();
				if(length_payload%4) {
					length_padding = (4 - length_payload%4);
					for(int j=0; j<(int)length_padding; j++)
						ofs.put('\0');
				}
			}
		}

		// overwrite payload length in dime header
		unsigned char endian[4];
		endian[0] = (length_payload >> 24);
		endian[1] = (length_payload >> 16);
		endian[2] = (length_payload >> 8);
		endian[3] = (length_payload >> 0);
		ofs.seekp(8, ios::beg);
		ofs.write((char*)endian, sizeof(endian));
	}
	catch(...) {
		ofs.close();
		for(i=0; i<3; i++)
			ifs[i].close();
		throw "cannot copy HttpBody file from three files";
	}
	
	ofs.close();
	for(i=0; i<3; i++)
		ifs[i].close();

	try {
		// remove unnecessary file
		for(i=0; i<3; i++)
			remove(filename[i+2].c_str());
	}
	catch(...) {
		throw "cannot remove unnecessary files";
	}
}

void DataRetainer::decomposeMime()
{
	// open input http body file stream
	ifstream ifs(filename[1].c_str(), ios::binary);
	if(!ifs.is_open())
		throw "cannot open input file in DataRetainer::decomposeMime: ";
	
	ifs.seekg(0, ios::beg);

	try {
		unsigned long flen[3]; // each file length of envelope, etc. 
		string boundary; // for mime boundary
		string str;
		bool start_mime_header = false;
		bool end_mime_header = false;
		bool parse_success = false;

		while(!ifs.eof())
		{
			getline(ifs, str);
			
			if(start_mime_header==false) { // set boudary
				if (str.length()!=1) {
					boundary = str;
					start_mime_header = true;					
				}
			}
		
			// get location of end header
			if(start_mime_header==true && end_mime_header==false) {
				if(str.length()==1) {
					flen[0] = (unsigned long)ifs.tellg();
					end_mime_header = true;
				}
			}
		
			if(end_mime_header==true && boundary == str) { // end of soap envelope
				flen[1] = (unsigned long)ifs.tellg() - flen[0] - boundary.length() - 1;
				parse_success = true;
				break;
			}
		}

		ifs.seekg(0, ios::end);
		
		// in the case of bad mime message
		if(parse_success==false) {
			throw "Invalide Mime Message";
		}
		
		flen[2] = (unsigned long)ifs.tellg() - (flen[0] + flen[1]); 

		ifs.seekg(0, ios::beg); // rewind
		
		try {
			ofstream ofs[3];
			for(int i=0; i<3; i++)
				ofs[i].open(filename[i+2].c_str(), ios::binary | ios::trunc);
			
			try {
				char buf[DR_SIZE_BUF];
				unsigned long size_buf;

				// split into 3 files
				int i;
				unsigned long pos_file;
				for(i=0; i<3; i++) {
					pos_file = 0;
					while(pos_file < flen[i]) {
						if(flen[i] - pos_file < DR_SIZE_BUF)
							size_buf = flen[i] - pos_file;
						else
							size_buf = DR_SIZE_BUF;
				
						ifs.read(buf, size_buf);
						pos_file += ifs.gcount();
						ofs[i].write(buf, ifs.gcount());
					}
				}
				ifs.close();
				for(i=0; i<3; i++)
					ofs[i].close();
			}
			catch(...) {
				ifs.close();
				int i;
				for(i=0; i<3; i++)
					ofs[i].close();
				throw "cannot split file into 3 files";
			}
		}
		catch(...) {
			throw "cannot split files in DataRetainer::decomposeMime: ";
		}
	}
	catch(...) {
			throw "cannot open output file in DataRetainer::decomposeMime: ";
	}
	

	try {
		// rename HttpBody to HttpBodyOld
		//rename(filename[1].c_str(), filename[5].c_str());
		remove(filename[1].c_str());
	}
	catch(...) {
		throw "cannot rename HttpBody to HttpBodyOld";
	}
}

void DataRetainer::composeMime()
{
	// oepen output http body file stream
	ofstream ofs(filename[1].c_str(), ios::binary | ios::trunc);
	if(!ofs.is_open())
		throw "cannot open output file in DataRetainer::composeMime: ";
	
	int i;
	ifstream ifs[3];
	for(i=0; i<3; i++) {
		ifs[i].open(filename[i+2].c_str(), ios::binary);
		if(!ifs[i].is_open())
			throw "cannot open input file in DataRetainer::composeMime: ";
	}

	try {
		char buf[DR_SIZE_BUF];
	
		// join 3 files; data head, envelope, data tail
		for(i=0; i<3; i++) {
			while(!ifs[i].eof() && ofs.good()) {
				ifs[i].read(buf, DR_SIZE_BUF);
				ofs.write(buf, ifs[i].gcount());
			}
		}
	}
	catch(...) {
		ofs.close();
		for(i=0; i<3; i++)
			ifs[i].close();
		
		throw "cannot join 3 files in DataRetainer::composeMime: ";
	}
			

	ofs.close();
	for(i=0; i<3; i++)
		ifs[i].close();
		
	try {
		// remove unnecessary file
		for(i=0; i<3; i++)
			remove(filename[i+2].c_str());
	}
	catch(...) {
		throw "remove unnecessary files in DataRetainer::composeMime: ";
	}
}

// ************* public ************* //


int DataRetainer::Create()
{
	static char METHOD_LABEL[] = "DataRetainer::Create: ";

	try {
		typedef struct sockaddr SockAddr;
    	typedef struct sockaddr_un SockAddrAf;
		
    	int sockfd = int();
    	SockAddrAf servAddr;
    	int af = AF_UNIX;
		
    	//memset((char*)&servAddr, 0, sizeof(servAddr));
    	servAddr.sun_family = af;
    	SrvConf* srvConf = new SrvConf();
    	string idMgrSockAddr = srvConf->getSocketPath() + IDMGR_SOCKET_ADDR;
    	strcpy(servAddr.sun_path, idMgrSockAddr.c_str());
    	delete srvConf;

#if HAVE_TYPE_OF_ADDRLEN_AS_SOCKLEN_T
    	socklen_t servAddrLen = socklen_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_SIZE_T
    	size_t servAddrLen = size_t();
#endif
#if HAVE_TYPE_OF_ADDRLEN_AS_INT
    	int servAddrLen = int();
#endif
		
    	servAddrLen = sizeof(servAddr);
		
    	if (0 > (sockfd = socket(af, SOCK_STREAM, 0))) {
			throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_OPEN_ERROR
				,APLOG_ERROR,__FILE__,__LINE__);
        	return -1;
        	//throw exception();
    	}
		
		if (0 > connect(sockfd, (SockAddr*)&servAddr, servAddrLen)) {
        	//added 2003/02/06
        	CloseSocket(sockfd);
			throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_CONNECT_ERROR
						,APLOG_ERROR,__FILE__,__LINE__);
        	return -1;
        	//throw exception();
    	}
		
    	string readBuf;
    	if (0 > read(sockfd, readBuf)) {
        	//added 2003/02/06
        	CloseSocket(sockfd);
			throw Exception(-1,OPENSOAPSERVER_NETWORK_SOCKET_READ_ERROR
					,APLOG_ERROR,__FILE__,__LINE__);
        	return -1;
        	//throw exception();
    	}
		
		//added 2003/02/06
    	CloseSocket(sockfd);
		
    	Id = readBuf;
    	//error check
    	if (Id.empty()) {
			Exception e(-1,OPENSOAPSERVER_UNKNOWN_ERROR
						,APLOG_ERROR,__FILE__,__LINE__);
			e.SetErrText("unable to get id for IdManager");
			throw (e);
			//throw exception??? use okano's except
    	}
    	//return DataRetainer::Id(readBuf);
		
		
		for(int i=0; i<DR_NUM_FILE; i++) 
			filename[i] = namePath + Id + def_name[i];
	}
	catch(Exception e) {
		AppLogger::Write(APLOG_DEBUG, "%s %s" ,METHOD_LABEL,"sockAddr");
		AppLogger::Write(e);
	}
	
	return 0;
}

void DataRetainer::SetId(const string num)
{
	static char METHOD_LABEL[] = "DataRetainer::SetId: ";
	
	try {
		Id = num;
		
		for(int i=0; i<DR_NUM_FILE; i++) 
			filename[i] = namePath + Id + def_name[i];
	}
	catch(...) {
		AppLogger::Write(APLOG_DEBUG, "%s %s" ,METHOD_LABEL, "?");
		throw METHOD_LABEL;
	}
		
}

void DataRetainer::GetId(string& num)
{
	static char METHOD_LABEL[] = "DataRetainer::GetId: ";

	try {
		num = Id;
	}
	catch(...) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "?");
		throw METHOD_LABEL;
	}		
}

string DataRetainer::GetHttpBodyFileName()
{
	static char METHOD_LABEL[] = "DataRetainer::GetHttpBodyFileName: ";
	
	try {
		compose();
		return filename[1];
	}
	catch(char* e) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, e);
		throw METHOD_LABEL;
	}
}
/*
	server.conf
	before      after   status
	None/OK/NG->None  = None
	None/OK/NG->Ok    = OK
	None/OK/NG->NG    = hold
*/
void DataRetainer::DeleteFiles()
{
    static char METHOD_LABEL[] = "DataRetainer::DeleteFiles: ";
    static string backup="";
    static time_t tm=0;
    static bool msgflg=true;

    //check delete status
    string status;
    try {
        getInternalStatus("life", status);
    }
    catch(char* e) {
        char error[256];
        strcpy(error, e); strcat(error, " through getInternalStatus");
        throw error;
    }
		
    // life status is keep, then return
    if (status=="keep") {
        return;
    }

    if ((time(NULL)-tm)>5) {
        tm=time(NULL);
        try{
            static const string spoolbackup="/server_conf/spool/backup/path=?";
            SrvConf srvconf;
            std::vector<std::string> attrs;
            attrs.clear();
            srvconf.cquery(spoolbackup, attrs);
            if (attrs.size() > 0) {
                if (access(attrs[0].c_str(),R_OK|W_OK|X_OK)==0){
                    backup=attrs[0]+"/";
                    msgflg=true;
                }else {
                    if (msgflg) {
                        AppLogger::Write(APLOG_DEBUG,"invalid backup path");
                        msgflg=false;
                    }
                }
            }
            else {
                msgflg=true;
                backup="";
            }
        }
        catch(...){
            if (msgflg) {
                AppLogger::Write(APLOG_DEBUG,"can't get backup path");
            }
        }
    }
    
    try {
        int i;
        struct stat st;
        string newname;
        for ( i=0 ; i< 5 ; i++) {
            if (!stat(filename[i].c_str(),&st)) {
                if (backup.length() &&
                    access(backup.c_str(),R_OK|W_OK|X_OK)==0){
                    newname = backup + Id + def_name[i];
                    rename(filename[i].c_str(), newname.c_str());
                }
                else {
                    unlink(filename[i].c_str());
                }
            }
        }
    }
    catch(...) {
        AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "cannot rename");
        throw METHOD_LABEL;
    }
}

inline int toLower( int x )
{
    return tolower(x);
}

void DataRetainer::AddHttpHeaderElement(string key, string data)
{
	static char METHOD_LABEL[] = "DataRetainer::AddHttpHeaderElement: ";

	try {
		loadStatus();
		transform(key.begin(), key.end(), key.begin(), toLower);
		Status[key] = data;
		saveStatus();
	}
	catch(char* e) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, e);
		throw METHOD_LABEL;
	}
	catch(...) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "?");
		throw METHOD_LABEL;
	}
}

bool DataRetainer::GetHttpHeaderElement(string key, string& data)
{
	static char METHOD_LABEL[] = "DataRetainer::GetHttpHeaderElement: ";

	try {
		loadStatus();
		transform(key.begin(), key.end(), key.begin(), toLower);
		map<string, string>::iterator i = Status.find(key);
		if ( i == Status.end() ) {
			data = "";
			return false;
		} else {
			data = i->second;
			return true;
		}
	}
	catch(char* e) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, e);
		throw METHOD_LABEL;
	}
	catch(...) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "?");
		throw METHOD_LABEL;
	}
}

void DataRetainer::GetSoapEnvelope(string& str)
{
	static char METHOD_LABEL[] = "DataRetainer::GetSoapEnvelope: ";

	try {
		decompose();
		loadSoapEnvelope();
		str = SoapEnvelope;
	}
	catch(char* e) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, e);
		throw METHOD_LABEL;
	}
	catch(...) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "?");
		throw METHOD_LABEL;
	}
}

void DataRetainer::SetSoapEnvelope(const string str)
{
	static char METHOD_LABEL[] = "DataRetainer::SetSoapEnvelope: ";

	try {
		SoapEnvelope = str;
		saveSoapEnvelope();
		setInternalStatus("division", "created");
		// in this version, "created" is not referrd anywhere
	}
	catch(char* e) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, e);
		throw METHOD_LABEL;
	}
	catch(...) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "?");
		throw METHOD_LABEL;
	}
}

void DataRetainer::SetSoapFault(const string str, const string err)
{
	static char METHOD_LABEL[] = "DataRetainer::SetSoapEnvelope: ";

	try {
		SoapEnvelope = str;
		saveSoapEnvelope();
		setInternalStatus("division", "created");
		AddHttpHeaderElement("status", err);
	}
	catch(char* e) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, e);
		throw METHOD_LABEL;
	}
	catch(...) {
		AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "?");
		throw METHOD_LABEL;
	}	
}

void DataRetainer::SetLifeStatus(const LifeStatus& st)
{
    static char METHOD_LABEL[] = "DataRetainer::SetLifeStatus: ";

    try {
        if (st == DataRetainer::KEEP) {
            setInternalStatus("life", "keep");
        }
        else if (st == DataRetainer::DONE) {
            setInternalStatus("life", "done");
        }
    }
    catch(char* e) {
        AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, e);
        throw METHOD_LABEL;
    }
    catch(...) {
        AppLogger::Write(APLOG_DEBUG, "%s %s", METHOD_LABEL, "?");
        throw METHOD_LABEL;
    }	

    return;
}
