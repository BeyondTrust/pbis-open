/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPEnvelope.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(OpenSOAP_CPPLIB_OPENSOAPENVELOPE_H)
#define OpenSOAP_CPPLIB_OPENSOAPENVELOPE_H

#include <string>
#include <OpenSOAP/Envelope.h>
#include "OpenSOAPObject.h"

namespace COpenSOAP{

class Block;

class Envelope  : public Object
{
public:
	std::string GetObjectName() const { return "Envelope"; }

	Envelope(OpenSOAPEnvelopePtr p);
	Envelope(const std::string& var, const std::string& prefix);
	~Envelope();

//	OpenSOAPEnvelopePtr RetainOpenSOAPEnvelopePtr();
	OpenSOAPEnvelopePtr Detach();
	const OpenSOAPEnvelopePtr GetOpenSOAPEnvelopePtrConst() const;

	Block AddBodyBlockMB(const std::string& name);
	Block GetBodyBlockMB(const std::string& name);

	void SetAttributeString(const std::string& name, const std::string& value);

	void PrintEnvelope(std::ostream& os, const std::string& label) ;
private:
	Envelope(const Envelope& env); // disabled

	OpenSOAPEnvelopePtr pEnv;
	bool bRefference;
};

}; // COpenSOAP
#endif // !defined(OpenSOAP_CPPLIB_OPENSOAPENVELOPE_H)
