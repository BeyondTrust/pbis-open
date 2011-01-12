/*-----------------------------------------------------------------------------
 * $RCSfile: CORBADllCalcService.cpp,v $
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


// This is the object implementation of CORBA

class calc_i : public POA_calc,
	       public PortableServer::RefCountServantBase
{
public:
  inline calc_i() {}
  virtual ~calc_i() {}
  virtual double add(double a, double b);
  virtual double subtract(double a, double b);
  virtual double multiply(double a, double b);
  virtual double divide(double a, double b);
};


CORBA::Double calc_i::add(CORBA::Double a, CORBA::Double b)
{
  return (a+b);
}

CORBA::Double calc_i::subtract(CORBA::Double a, CORBA::Double b)
{
  return (a-b);
}

CORBA::Double calc_i::multiply(CORBA::Double a, CORBA::Double b)
{
  return (a*b);
}

CORBA::Double calc_i::divide(CORBA::Double a, CORBA::Double b)
{
  if(b==0.0){
    return (-0);
  }
  else{
    return (a/b);
  }
}

// The followings act as a client to the object of CORBA

typedef 
int
(*CalcFunctionType)(double a, double b, double *r);

static
int
Add(double a,
    double b,
    double *r) {
    int ret = OPENSOAP_MEM_ERROR;
    cerr << "the begining of Add" << endl;

 try {
   //    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB3");
    int argc_add = 0;
    CORBA::ORB_var orb = CORBA::ORB_init(argc_add, NULL, "omniORB3");

    // Obtain a reference to the root POA.
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);
		
    calc_i* mycalc = new calc_i();
		
    // Activate the object. This tells the POA that this object is 
    // ready to accept requests.
    PortableServer::ObjectId_var myechoid = poa->activate_object(mycalc);
		
    // Obtain a reference to the object.
    calc_var mycalcref = mycalc->_this();
		
    // Decrement the reference count of the object implementation, so 
    // that it will be properly cleaned up when the POA has determined
    // that it is no longer needed.
    mycalc->_remove_ref();
		
    // Obtain a POAManager, and tell the POA to start accepting
    // requests on its objects.
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();
		
    // Do the client-side call.
    if( CORBA::is_nil(mycalcref) ) {
	cerr << "calc: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = mycalcref->add(a,b)" << endl;

    *r = mycalcref->add(a,b);
    cerr << "after *r = mycalcref->add(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a + b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
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
    cerr << "the begining of Add" << endl;

 try {
   //    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB3");
    int argc_add = 0;
    CORBA::ORB_var orb = CORBA::ORB_init(argc_add, NULL, "omniORB3");

    // Obtain a reference to the root POA.
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);
		
    calc_i* mycalc = new calc_i();
		
    // Activate the object. This tells the POA that this object is 
    // ready to accept requests.
    PortableServer::ObjectId_var myechoid = poa->activate_object(mycalc);
		
    // Obtain a reference to the object.
    calc_var mycalcref = mycalc->_this();
		
    // Decrement the reference count of the object implementation, so 
    // that it will be properly cleaned up when the POA has determined
    // that it is no longer needed.
    mycalc->_remove_ref();
		
    // Obtain a POAManager, and tell the POA to start accepting
    // requests on its objects.
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();
		
    // Do the client-side call.
    if( CORBA::is_nil(mycalcref) ) {
	cerr << "calc: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = mycalcref->subtract(a,b)" << endl;

    *r = mycalcref->subtract(a,b);
    cerr << "after *r = mycalcref->subtract(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a - b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  
  return ret;
}

static
int
Multiply(double a,
         double b,
         double *r) {
    int ret = OPENSOAP_PARAMETER_BADVALUE;
    cerr << "the begining of Multiply" << endl;
 
 try {
   //    CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB3");
    int argc_multi = 0;
    CORBA::ORB_var orb = CORBA::ORB_init(argc_multi, NULL, "omniORB3");

    // Obtain a reference to the root POA.
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);
		
    calc_i* mycalc = new calc_i();
		
    // Activate the object. This tells the POA that this object is 
    // ready to accept requests.
    PortableServer::ObjectId_var myechoid = poa->activate_object(mycalc);
		
    // Obtain a reference to the object.
    calc_var mycalcref = mycalc->_this();
		
    // Decrement the reference count of the object implementation, so 
    // that it will be properly cleaned up when the POA has determined
    // that it is no longer needed.
    mycalc->_remove_ref();
		
    // Obtain a POAManager, and tell the POA to start accepting
    // requests on its objects.
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();
		
    // Do the client-side call.
    if( CORBA::is_nil(mycalcref) ) {
	cerr << "calc: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = mycalcref->multiply(a,b)" << endl;

    *r = mycalcref->multiply(a,b);
    cerr << "after *r = mycalcref->multiply(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a * b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
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

    // Obtain a reference to the root POA.
    CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
    PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);
		
    calc_i* mycalc = new calc_i();
		
    // Activate the object. This tells the POA that this object is 
    // ready to accept requests.
    PortableServer::ObjectId_var myechoid = poa->activate_object(mycalc);
		
    // Obtain a reference to the object.
    calc_var mycalcref = mycalc->_this();
		
    // Decrement the reference count of the object implementation, so 
    // that it will be properly cleaned up when the POA has determined
    // that it is no longer needed.
    mycalc->_remove_ref();
		
    // Obtain a POAManager, and tell the POA to start accepting
    // requests on its objects.
    PortableServer::POAManager_var pman = poa->the_POAManager();
    pman->activate();
		
    // Do the client-side call.
    if( CORBA::is_nil(mycalcref) ) {
	cerr << "calc: The object reference is nil! \n" << endl;
        ret = OPENSOAP_MEM_ERROR;
	return ret;
    }
    cerr << "before *r = mycalcref->divide(a,b)" << endl;

    *r = mycalcref->divide(a,b);
    cerr << "after *r = mycalcref->divide(a,b)" << endl;

    cerr << "a = " << a << endl
         << "b = " << b << endl
         << "The Calc object result: a / b = " << *r << endl;

    orb->destroy();
    
    ret = OPENSOAP_NO_ERROR;
  }
  catch(CORBA::COMM_FAILURE& ex) {
    cerr << "Caught system exception COMM_FAILURE -- unable to contact the "
         << "object." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::SystemException& se) {
    cerr << "Caught CORBA::SystemException." << endl;
    cerr << "pd_status = " << se.completed() << endl;
    
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(CORBA::Exception &ex) {
    cerr << "Caught CORBA::Exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(omniORB::fatalException& fe) {
    cerr << "Caught omniORB::fatalException:" << endl;
    cerr << "  file: " << fe.file() << endl;
    cerr << "  line: " << fe.line() << endl;
    cerr << "  mesg: " << fe.errmsg() << endl;
    ret = OPENSOAP_MEM_ERROR;
  }
  catch(...) {
    cerr << "Caught unknown exception." << endl;
    ret = OPENSOAP_MEM_ERROR;
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
		fprintf(stderr, "CORBADllCalcService\n");
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
