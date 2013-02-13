#include <stdio.h>
#include <iostream>
#include <map>
#include <string>

using namespace std;

#define DR_NUM_FILE 6
#define DR_SIZE_BUF 1024

namespace OpenSOAP {
	class DataRetainer
	{
	private:
		string namePath;
		static string def_name[DR_NUM_FILE];
		string filename[DR_NUM_FILE];
		
		mode_t musk, cur_musk;

	private:
		string Id;
		map<string, string> Status;
		string SoapEnvelope;

	private:
		void saveStatus();
		void loadStatus();

		void loadSoapEnvelope();
		void saveSoapEnvelope();

	private:
		void decompose();
		void compose();

		void decomposeDime();
		void composeDime();

		void decomposeMime();
		void composeMime();

		void setInternalStatus(string, string);
		bool getInternalStatus(string, string&);

	public:
		DataRetainer(const string& aNamePath);
		virtual ~DataRetainer(void);

		int Create();
		void SetId(const string);

		void GetId(string&);

		string GetHttpBodyFileName();
		void DeleteFiles();

		void AddHttpHeaderElement(string key, string val);
		bool GetHttpHeaderElement(string key, string& val);

		void GetSoapEnvelope(string& str);
		void SetSoapEnvelope(const string str);
		void SetSoapFault(const string str, const string error);

                typedef enum {
                    KEEP,
                    DONE,
                } LifeStatus;
                void SetLifeStatus(const LifeStatus& st);

		// Old functions
		void UpdateSoapEnvelope(const string str) { SetSoapEnvelope(str); }
		void Compose() { }
		void Decompose() { }
		
	};
}
