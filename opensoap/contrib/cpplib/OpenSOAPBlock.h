/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPBlock.h,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */
#if !defined(OpenSOAP_CPPLIB_OPENSOAPBLOCK_H)
#define OpenSOAP_CPPLIB_OPENSOAPBLOCK_H

#include <OpenSOAP/Block.h>
#include "OpenSOAPXMLElm.h"

namespace COpenSOAP {
	class Envelope;

class Block : public XMLElm
{
	friend class Envelope;
private:
	Block(OpenSOAPBlockPtr block);
public:
	std::string GetObjectName() const { return "Block"; }
	~Block();

public:
};

}; // COpenSOAP
#endif // !defined(OpenSOAP_CPPLIB_OPENSOAPBLOCK_H)
