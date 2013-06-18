/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPEnvelope.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#include <iostream>

#include "OpenSOAPEnvelope.h"
#include "OpenSOAPBlock.h"

namespace COpenSOAP {

Envelope::Envelope(OpenSOAPEnvelopePtr p)
:bRefference(true)
{
	pEnv = p; 
	OpenSOAPEnvelopeRetain(pEnv);
}

Envelope::Envelope(const std::string& var, const std::string& prefix)
:bRefference(false)
{
	Try(OpenSOAPEnvelopeCreateMB(var.c_str(), prefix.c_str(), &pEnv));
}
/*
Envelope::Envelope(const Envelope& env)
{
	pEnv = env.pEnv;
	OpenSOAPEnvelopeRetain(pEnv);
}
*/
Envelope::~Envelope()
{
	if(!bRefference)
		OpenSOAPEnvelopeRelease(pEnv);
}
/*
OpenSOAPEnvelopePtr Envelope::RetainOpenSOAPEnvelopePtr()
{ 
	OpenSOAPEnvelopeRetain(pEnv);
	return pEnv;
}
*/
const OpenSOAPEnvelopePtr Envelope::GetOpenSOAPEnvelopePtrConst() const
{
	return pEnv; 
}

OpenSOAPEnvelopePtr Envelope::Detach()
{ 
	bRefference = true;
	return pEnv;
}

Block
Envelope::AddBodyBlockMB(const std::string& name)
{
	OpenSOAPBlockPtr body = NULL;
	Try(OpenSOAPEnvelopeAddBodyBlockMB(pEnv, name.c_str(), &body));
	return Block(body);
}

Block
Envelope::GetBodyBlockMB(const std::string& name)
{
	OpenSOAPBlockPtr body = NULL;
	Try(OpenSOAPEnvelopeGetBodyBlockMB(pEnv, name.c_str(), &body));
	return Block(body);
}

void Envelope::SetAttributeString(const std::string& name, const std::string& value)
{
	XMLElm(reinterpret_cast<OpenSOAPXMLElmPtr>(pEnv)).SetAttributeValueString(name, value);
}

void Envelope::PrintEnvelope(std::ostream& os, const std::string& label) 
{
	ByteArray array;
	OpenSOAPEnvelopeGetCharEncodingString(pEnv, NULL, array);
	size_t size = 0;
	const char *beg
		= reinterpret_cast<const char *>(array.begin(size));

	os << "=========== " << label << " begin =========" << std::endl;
	os << std::string(beg, size) << std::endl;	
	os << "=========== " << label << " end   =========" << std::endl;
}
};//COpenSOAP
