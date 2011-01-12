/*-----------------------------------------------------------------------------
 * $RCSfile: OpenSOAPBlock.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <OpenSOAP/String.h>

#include "OpenSOAPBlock.h"
#include "OpenSOAPString.h"
#include "OpenSOAPXMLElm.h"

namespace COpenSOAP {

Block::Block(OpenSOAPBlockPtr block)
:XMLElm(0)
{
	pElm = (OpenSOAPXMLElmPtr)block;
}

Block::~Block()
{
}

};
//// method
