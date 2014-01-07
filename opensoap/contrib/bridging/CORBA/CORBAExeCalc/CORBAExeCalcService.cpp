/*-----------------------------------------------------------------------------
 * $RCSfile: CORBAExeCalcService.cpp,v $
 *
 * See Copyright for the status of this software.
 *
 * The OpenSOAP Project
 * http://opensoap.jp/
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>


#include <iostream.h>

#include "calc.hh"

#include <OpenSOAP/OpenSOAP.h>
#include <OpenSOAP/Service.h>


#if !defined(CONNECT_TYPE)
//# define CONNECT_TYPE "stdio"
# define CONNECT_TYPE "cgi"
#endif /* CONNECT_TYPE */

static
const   char
CALC_METHOD_NAMESPACE_URI[] =
"http://tempuri.org/message/";
static
const   char
CALC_METHOD_NAMESPACE_PREFIX[] =
"m";

static
const char
SERVICE_OPERAND_A_NAME[] = "A";
static
const char
SERVICE_OPERAND_B_NAME[] = "B";

static
const char
SERVICE_RESPONSE_RESULT_NAME[] = "Result";


typedef 
int
(*CalcFunctionType)(double a, double b, double *r);

static CORBA::Object_ptr
getObjectReference(CORBA::ORB_ptr orb)
{
  CosNaming::NamingContext_var rootContext;
  cerr << "enter the getObjectReference." << endl;
  cerr << "orb = " << orb << endl;
  
  try {
    // Obtain a reference to the root context of the Name service:
    CORBA::Object_var obj;
    cerr << "before resolve: obj = " << obj << endl;
    obj = orb->resolve_initial_references("NameService");
    cerr << "after resolve_initial_references." << endl;
    cerr << "obj = " << obj << endl;

    // Narrow the reference returned.
    cerr << "before NameingContext::_narrow." << endl;
    rootContext = CosNaming::NamingContext::_narrow(obj);
    if( CORBA::is_nil(rootContext) ) {
      cerr << "Failed to narrow the root naming context." << endl;
      return CORBA::Object::_nil();
    }   
    cerr << "after NameingContext::_narrow." << endl;

  }
  catch(CORBA::ORB::InvalidName& ex) {
    // This should not happen!
    cerr << "Service required is invalid [does not exist]." << endl;
    return CORBA::Object::_nil();
  }
  //  catch(CORBA::SystemException&) {
  //  cerr << "Caught a CORBA::SystemException after resolve_initial_references."
  //	 << endl;
  // return CORBA::Object::_nil();
  //} 

  // Create a name object, containing the name test/context:
  CosNaming::Name name;
  name.length(2);

  name[0].id   = (const char*) "test";       // string copied
  name[0].kind = (const char*) "my_context"; // string copied
  name[1].id   = (const char*) "calc";
  name[1].kind = (const char*) "Object";
  // Note on kind: The kind field is used to indicate the type
  // of the object. This is to avoid conventions such as that used
  // by files (name.type -- e.g. test.ps = postscript etc.)


  try {
    // Resolve the name to an object reference.
    cerr << "before rootContext->resolve(name)." << endl;
    return rootContext->resolve(name);
  }

  catch(CosNaming::NamingContext::NotFound& ex) {
    // This exception is thrown if any of the components of the
    // path [contexts or the object] aren't found:
    cerr << "Context not found." << endl;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "naming service." << endl;
  }
  catch(CORBA::SystemException&) {
    cerr << "Caught a CORBA::SystemException while using the naming service."
	 << endl;
  }

  cerr << "before CORBA::Object::_nil()" << endl;
  return CORBA::Object::_nil();
}



static void addOmni(calc_ptr addSv)
{
	if( CORBA::is_nil(addSv) ) {
		cerr << "add: The object reference is nil! \n" << endl;
		return;
	}
	CORBA::Double a = 10.5;
	CORBA::Double b = 6.3;
	CORBA::Double result = addSv->add(a,b);

	cerr << "a = " << a << endl
	     << "b = " << b << endl
	     << "The Calc object result: a + b = " << result << endl;
}


static
int
Add(double a,
    double b,
    double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    cerr << "the begining of Add" << endl;

 try {
   //    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB3");
    int argc_add = 0;
    CORBA::ORB_var orb = CORBA::ORB_init(argc_add, NULL, "omniORB3");
    cerr << "after CORBA::ORB_init" << endl;
    cerr << "orb = " << orb << endl;

    CORBA::Object_var obj = getObjectReference(orb);
    cerr << "after getObjectReference(orb)" << endl;

    calc_var calcref = calc::_narrow(obj);
    cerr << "calc::_narrow succeeded" << endl;

    //    addOmni(calcref);
    if( CORBA::is_nil(calcref) ) {
	cerr << "add: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = calcref->add(a,b)" << endl;

    *r = calcref->add(a,b);
    cerr << "after *r = calcref->add(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a + b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  
  return ret;
}

static
int
Subtract(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    cerr << "the begining of Subtract" << endl;

 try {
   //    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB3");
    int argc_sub = 0;
    CORBA::ORB_var orb = CORBA::ORB_init(argc_sub, NULL, "omniORB3");

    CORBA::Object_var obj = getObjectReference(orb);
    cerr << "after getObjectReference(orb)" << endl;

    calc_var calcref = calc::_narrow(obj);
    cerr << "calc::_narrow succeeded" << endl;

    if( CORBA::is_nil(calcref) ) {
	cerr << "add: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = calcref->subtract(a,b)" << endl;

    *r = calcref->subtract(a,b);
    cerr << "after *r = calcref->subtract(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a - b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  
  return ret;
}

static
int
Multiply(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    cerr << "the begining of Subtract" << endl;

   try {
   //    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB3");
    int argc_multi = 0;
    CORBA::ORB_var orb = CORBA::ORB_init(argc_multi, NULL, "omniORB3");

    CORBA::Object_var obj = getObjectReference(orb);
    cerr << "after getObjectReference(orb)" << endl;

    calc_var calcref = calc::_narrow(obj);
    cerr << "calc::_narrow succeeded" << endl;

    if( CORBA::is_nil(calcref) ) {
	cerr << "add: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = calcref->multiply(a,b)" << endl;

    *r = calcref->multiply(a,b);
    cerr << "after *r = calcref->multiply(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a * b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  

    return ret;
}

static
int
Divide(double a,
       double b,
       double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    cerr << "the begining of Divide" << endl;

    if( !r || !b ) {
     	return ret;
    }

 try {
   //    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB3");
    int argc_div = 0;
    CORBA::ORB_var orb = CORBA::ORB_init(argc_div, NULL, "omniORB3");

    CORBA::Object_var obj = getObjectReference(orb);
    cerr << "after getObjectReference(orb)" << endl;

    calc_var calcref = calc::_narrow(obj);
    cerr << "calc::_narrow succeeded" << endl;

    if( CORBA::is_nil(calcref) ) {
	cerr << "calc: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = calcref->divide(a,b)" << endl;

    *r = calcref->divide(a,b);
    cerr << "after *r = calcref->divide(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a / b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_PARAMETER_BADVALUE;
  }
  
    return ret;
}

typedef struct {
    const char *requestName;
    const char *responseName;
    CalcFunctionType calcFunc;
} CalcServiceMethodMapItem;

static
int
CalcServiceGetParameter(OpenSOAPEnvelopePtr /* [in] */ request,
                        const char * /* [in] */ request_name,
                        double * /* [out] */ a,
                        double * /* [out] */ b) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (a && b) {
        OpenSOAPBlockPtr body_block = NULL;
        ret = OpenSOAPEnvelopeGetBodyBlockMB(request,
                                             request_name,
                                             &body_block);
        if (OPENSOAP_SUCCEEDED(ret)) {
            int is_same_ns = 0;
            ret = OpenSOAPBlockIsSameNamespaceMB(body_block,
                                                 CALC_METHOD_NAMESPACE_URI,
                                                 &is_same_ns);
            if (OPENSOAP_SUCCEEDED(ret) && is_same_ns) {
                ret = OpenSOAPBlockGetChildValueMB(body_block,
                                                   SERVICE_OPERAND_A_NAME,
                                                   "double",
                                                   a);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPBlockGetChildValueMB(body_block,
                                                       SERVICE_OPERAND_B_NAME,
                                                       "double",
                                                       b);
                }
            }
        }
    }

    return ret;
}

static
int
CalcServiceCreateResponse(const char * /* [in] */ response_name,
                          double /* [in] */ r,
                          OpenSOAPEnvelopePtr * /* [out] */ response) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;

    if (response_name && *response_name) {
        ret = OpenSOAPEnvelopeCreateMB(NULL, NULL, response);
        if (OPENSOAP_SUCCEEDED(ret)) {
            OpenSOAPBlockPtr body_block = NULL;
            ret = OpenSOAPEnvelopeAddBodyBlockMB(*response,
                                                 response_name,
                                                 &body_block);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = OpenSOAPBlockSetNamespaceMB(body_block,
                                                  CALC_METHOD_NAMESPACE_URI,
                                                  CALC_METHOD_NAMESPACE_PREFIX);
                if (OPENSOAP_SUCCEEDED(ret)) {
                    ret = OpenSOAPBlockSetChildValueMB(body_block,
                                                       SERVICE_RESPONSE_RESULT_NAME,
                                                       "double",
                                                       &r);
                }
            }
        }
    }

    return ret;
}

static
int
CalcServiceFunc(OpenSOAPEnvelopePtr /* [in] */ request,
                OpenSOAPEnvelopePtr * /* [out] */ response,
                void * /* [in, out] */ opt) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    CalcServiceMethodMapItem *m_map = (CalcServiceMethodMapItem *)opt;

    if (m_map) {
        double a = 0;
        double b = 0;
		fprintf(stderr, "CORBAExeCalcService\n");
        ret = CalcServiceGetParameter(request,
                                      m_map->requestName,
                                      &a,
                                      &b);
        if (OPENSOAP_SUCCEEDED(ret)) {
            double r = 0;
            ret = (m_map->calcFunc)(a, b, &r);
            if (OPENSOAP_SUCCEEDED(ret)) {
                ret = CalcServiceCreateResponse(m_map->responseName,
                                                r,
                                                response);
            }
        }
    }

    return ret;
}

static
const CalcServiceMethodMapItem
CalcServiceMethodMap[] = {
    {"Add", "AddResponse", Add},
    {"Subtract", "SubtractResponse", Subtract},
    {"Multiply", "MultiplyResponse", Multiply},
    {"Divide", "DivideResponse", Divide},
    {NULL, NULL, NULL}
};

/*
 */
int
main(void) {
    int ret = 0;
    OpenSOAPServicePtr calc_service = NULL;
    int error_code
        = OpenSOAPInitialize(NULL);
    if (OPENSOAP_SUCCEEDED(error_code)) {
        const CalcServiceMethodMapItem *m_map_i = CalcServiceMethodMap;
        error_code 
            = OpenSOAPServiceCreateMB(&calc_service,
                                      "CalcService",
                                      CONNECT_TYPE,
                                      0);
        for (; OPENSOAP_SUCCEEDED(error_code) && m_map_i->requestName;
            ++m_map_i) {
            error_code
                = OpenSOAPServiceRegisterMB(calc_service,
                                            m_map_i->requestName,
                                            CalcServiceFunc,
                                            (void *)m_map_i);
        }
        if (OPENSOAP_SUCCEEDED(error_code)) {
            error_code 
                = OpenSOAPServiceRun(calc_service);
        }

        OpenSOAPServiceRelease(calc_service);

        OpenSOAPUltimate();
    }

    return ret;
}
