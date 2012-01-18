#ifndef ErrorUtil_H
#define ErrorUtil_H

//#include <map>
#include <string>

namespace OpenSOAP {

class ErrorUtil{
	public:
		ErrorUtil(){}
		~ErrorUtil(){}
		static std::string toString(int id);
		static const char * toConstChar(int id);

}; //class ErrorUtil

} //namespace OpenSOAP
#endif //ErrorUtil_H
