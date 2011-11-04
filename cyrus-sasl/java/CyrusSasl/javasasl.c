/* javasasl.c--Java SASL JNI implementation
 * Tim Martin
 */
/***********************************************************
        Copyright 1998 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Carnegie Mellon
University not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
******************************************************************/

#include <config.h>
#include <stdio.h>
#include <sasl.h>
#include <saslutil.h>
#include <saslplug.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>

#include "javasasl.h"

#define VL(x) /* printf x */

static JNIEnv *globalenv;
static jobject globalobj;

static int setcomplete(JNIEnv *env, jobject obj);

static void throwexception(JNIEnv *env, int error)
{
  jclass newExcCls;

  VL (("Throwing exception!\n"));

  newExcCls = (*env)->FindClass(env, "CyrusSasl/SaslException");

  if (newExcCls == 0) { 
    return;
  }

  (*env)->ThrowNew(env, newExcCls, sasl_errstring(error,NULL,NULL));
}

/* server init */

JNIEXPORT jint JNICALL Java_CyrusSasl_ServerFactory_jni_1sasl_1server_1init
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jstring jstr)
{
  /* Obtain a C-copy of the Java string */
  const char *str = (*env)->GetStringUTFChars(env, jstr, 0);
  int result;

  result=sasl_server_init(NULL,str);
  if (result!=SASL_OK)
    throwexception(env,result);

  /* Now we are done with str */
  (*env)->ReleaseStringUTFChars(env, jstr, str);

  return result;
}

static int
log(void *context __attribute__((unused)),
    int priority,
    const char *message) 
{
  const char *label;
  jstring jlabel, jmessage;
  jclass cls;
  jmethodID mid;

  if (! message)
    return SASL_BADPARAM;

  switch (priority) {
  case SASL_LOG_ERR:
    label = "Error";
    break;
  case SASL_LOG_WARN:
    label = "Warning";
    break;
  case SASL_LOG_NOTE:
    label = "Note";
    break;
  case SASL_LOG_FAIL:
    label = "Fail";
    break;
  case SASL_LOG_PASS:
    label = "Pass";
    break;
  case SASL_LOG_TRACE:
    label = "Trace";
    break;
  case SASL_LOG_DEBUG:
    label = "Debug";
    break;
  default:
    return SASL_BADPARAM;
  }

  VL(("I have message %s\n",message));
  VL(("Trying to call log callback\n"));
  cls = (*globalenv)->GetObjectClass(globalenv, globalobj);
  mid = (*globalenv)->GetMethodID(globalenv, cls, "callback_log",
				  "(Ljava/lang/String;Ljava/lang/String;)V");
  if (mid == 0) {
    return SASL_FAIL;
  }

  /* make label into a java string */
  jlabel= (*globalenv)->NewStringUTF(globalenv,label);

  /* make message into a java string */
  jmessage= (*globalenv)->NewStringUTF(globalenv,message);

  /* call java */
  (*globalenv)->CallVoidMethod(globalenv, globalobj, mid, 
			       jlabel, jmessage);

  /* Now we are done with jlabel */
  (*globalenv)->ReleaseStringUTFChars(globalenv, jlabel, label);

  /* Now we are done with jmessage */
  (*globalenv)->ReleaseStringUTFChars(globalenv, jmessage, message);

  VL(("done with log callback"));

  return SASL_OK;
}

static sasl_callback_t callbacks[] = {
  {
    SASL_CB_LOG,      &log, NULL
  }, {
    SASL_CB_PASS,     NULL, NULL
  }, {
    SASL_CB_USER,     NULL, NULL /* we'll handle these ourselves */
  }, {
    SASL_CB_AUTHNAME, NULL, NULL /* we'll handle these ourselves */
  }, {
      /* TODO
	 SASL_CB_ECHOPROMPT, &prompt, NULL
  }, {
    SASL_CB_NOECHOPROMPT, &prompt, NULL
    }, { */
    SASL_CB_LIST_END, NULL, NULL
  }
};

/* client init */
JNIEXPORT jint JNICALL Java_CyrusSasl_ClientFactory_jni_1sasl_1client_1init
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jstring jstr)
{
  /* Obtain a C-copy of the Java string */
  const char *str = (*env)->GetStringUTFChars(env, jstr, 0);
  int result;

  VL(("client initing\n"));

  result=sasl_client_init(callbacks);
  if (result!=SASL_OK)
    throwexception(env,result);

  /* Now we are done with str */
  (*env)->ReleaseStringUTFChars(env, jstr, str);

  return result;
}

/* server new */

JNIEXPORT jint JNICALL Java_CyrusSasl_ServerFactory_jni_1sasl_1server_1new
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jstring jservice, 
   jstring jlocal, 
   jint jsecflags)
{
  sasl_conn_t *conn;

  const char *service = (*env)->GetStringUTFChars(env, jservice, 0);
  const char *local_domain = (*env)->GetStringUTFChars(env, jlocal, 0);
  const char *user_domain = NULL;  
  int result;

  if (local_domain) {
      VL(("local domain = %s\n",local_domain));
  }
  if (user_domain) {
      VL(("user domain = %s\n",user_domain));
  }

  result=sasl_server_new(service, local_domain, user_domain, 
			 NULL, NULL, NULL, jsecflags, &conn);
  if (result!=SASL_OK)
    throwexception(env,result);

  /* Now we are done with str */
  (*env)->ReleaseStringUTFChars(env, jservice, service);  
  (*env)->ReleaseStringUTFChars(env, jlocal, local_domain);  

  return (jint) conn;
}


JNIEXPORT jint JNICALL JNICALL Java_CyrusSasl_ClientFactory_jni_1sasl_1client_1new
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jstring jservice, jstring jserver, jint jsecflags, jboolean successdata)
{
  sasl_conn_t *conn;

  const char *service = (*env)->GetStringUTFChars(env, jservice, 0);
  const char *serverFQDN = (*env)->GetStringUTFChars(env, jserver, 0);
  int result;

  result=sasl_client_new(service, serverFQDN, NULL, NULL, NULL,
			 jsecflags | (successdata ? SASL_SUCCESS_DATA : 0), 
                         &conn);

  if (result!=SASL_OK)
    throwexception(env,result);

  /* Now we are done with str */
  (*env)->ReleaseStringUTFChars(env, jservice, service);  
  (*env)->ReleaseStringUTFChars(env, jserver, serverFQDN);  

  return (jint) conn;
}

/* server start */

JNIEXPORT jbyteArray JNICALL Java_CyrusSasl_GenericServer_jni_1sasl_1server_1start
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jstring jstr, jbyteArray jarr, jint jlen)
{
  sasl_conn_t *conn;
  const char *mech = (*env)->GetStringUTFChars(env, jstr, 0);
  const char *out;
  unsigned int outlen;
   int result;
  jbyteArray arr;
  char *tmp;
  char *in=NULL;

  VL(("in server start\n"));

  if (jarr!=NULL)
    in = (*env)->GetByteArrayElements(env, jarr, 0);

  conn=(sasl_conn_t *) ptr;

  result=sasl_server_start(conn, mech,
			   (const char *) in, jlen,
			   &out, &outlen);

  if ((result!=SASL_OK) && (result!=SASL_CONTINUE))
  {

    throwexception(env,result);
    return NULL;
  }

  /* Because SASLv2 does not allow for persistance, we'll copy
   * it here */
  tmp = malloc(outlen);
  if(!tmp) {
      throwexception(env, SASL_NOMEM);
      return NULL;
  }

  memcpy(tmp, out, outlen);
  
  arr=(*env)->NewByteArray(env,outlen);
  (*env)->SetByteArrayRegion(env,arr, 0, outlen, (char *)tmp);

  return arr;
}


static int getvalue(JNIEnv *env, jobject obj, char *funcname, char **result, int *len)
{
    jclass cls;
    jmethodID mid;
    const char *str;
    jstring jstr;
    
    /* set up for java callback */
    cls = (*env)->GetObjectClass(env, obj);
    mid = (*env)->GetMethodID(env, cls, funcname,
				  "(I)Ljava/lang/String;");
    if (mid == 0) {
	VL(("Can't find %s callback!!!\n",funcname));
	return SASL_FAIL;
    }

    VL(("do the callback\n"));
    jstr = (jstring) (*env)->CallObjectMethod(env, obj, mid);

    if (jstr) {
        VL(("convert the result string into a char *\n"));
        str = (*env)->GetStringUTFChars(env, jstr, 0);

        /* copy password into the result */    
        *result=(char *) malloc( strlen(str));
        strcpy(*result, str);
        *len=strlen(str);

        /* Now we are done with str */
        (*env)->ReleaseStringUTFChars(env, jstr, str);
    } else {
        *result = NULL;
        *len = 0;
    }

    return SASL_OK;
}

static int callall_callbacks(JNIEnv *env, jobject obj, 
			     int calluid,int callaid,
			     int callpass,int callrealm)
{
    jclass cls;
    jmethodID mid;
    
    /* set up for java callback */
    cls = (*env)->GetObjectClass(env, obj);
    mid = (*env)->GetMethodID(env, cls, "do_callbacks", "(IIII)V");
    if (mid == 0) {
	VL(("Can't find do_callbacks callback!!!\n"));
	return SASL_FAIL;
    }

    /* do the callback */
    (*env)->CallVoidMethod(env, obj, mid,calluid,callaid,callpass,callrealm);

    VL(("callall_callbacks worked\n"));
    return SASL_OK;
}

/*
 * Fills in all the prompts by doing callbacks to java
 * returns SASL_INTERACT on sucess
 */

static int fillin_interactions(JNIEnv *env, jobject obj, 
				sasl_interact_t *tlist)
{
  sasl_interact_t *ptr=tlist;
  sasl_interact_t *uid=NULL; int is_uid = 0;
  sasl_interact_t *aid=NULL; int is_aid = 0;
  sasl_interact_t *pass=NULL;int is_pass = 0;
  sasl_interact_t *realm=NULL; int is_realm = 0;

  /* First go through the prompt list to see what we have */
  while (ptr->id!=SASL_CB_LIST_END)
  {
    if (ptr->id==SASL_CB_PASS)
	{  pass=ptr; is_pass = 1; }
    if (ptr->id==SASL_CB_AUTHNAME)
	{ aid=ptr; is_aid = 1; }
    if (ptr->id==SASL_CB_USER)
	{ uid=ptr; is_uid = 1; }
    if (ptr->id==SASL_CB_GETREALM)
	{ realm = ptr; is_realm = 1; }
    ptr->result=NULL;
    
    /* increment to next sasl_interact_t */
    ptr++;
  }

  callall_callbacks(env,obj,is_uid,is_aid,is_pass,is_realm);

  if (is_pass) {
      VL(("in is_pass\n"));

      getvalue(env,obj,"get_password",(char **) &(pass->result),(int *) &(pass->len));
  }
  if (is_aid) {
      VL(("in is_aid\n"));

      getvalue(env,obj,"get_authid",(char **) &(aid->result),(int *) &(aid->len));
  }
  if (is_uid) {
      VL(("in is_uid\n"));

      getvalue(env,obj,"get_userid",(char **) &(uid->result),(int *) &(uid->len)); 
  }
  if (is_realm) {
      VL(("in is_realm\n"));

      getvalue(env,obj,"get_realm",(char **) &(realm->result),(int *) &(realm->len));
  }

  /* everything should now be filled in (i think) */
  VL(("everything should now be filled in (i think)\n"));

  return SASL_INTERACT;
}

/* client start */

JNIEXPORT jbyteArray JNICALL Java_CyrusSasl_GenericClient_jni_1sasl_1client_1start(JNIEnv *env, jobject obj, jint ptr, jstring jstr)
{    
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  const char *mechlist = (*env)->GetStringUTFChars(env, jstr, 0);
  const char *out;
  unsigned int outlen=0;
  const char *mechusing;
  int result;
  sasl_interact_t *client_interact=NULL;
  char *tmp;
  jbyteArray arr;
  jstring jmechusing;
  jclass cls;
  jmethodID mid;

  VL(("sasl_start"));
 
  do {

      result=sasl_client_start(conn, mechlist,
			       &client_interact,
			       &out, 
                               &outlen,
			       &mechusing);

      if (result==SASL_INTERACT) {
	  int res2 = fillin_interactions(env,obj,client_interact);
      }

  } while (result==SASL_INTERACT);

  /* ok release mechlist */
  (*env)->ReleaseStringUTFChars(env, jstr, mechlist);  

  if ((result!=SASL_OK) && (result!=SASL_CONTINUE))
  {
    throwexception(env,result);
    return NULL;
  }

  /* tell the java layer what mechanism we're using */

  /* set up for java callback */
  cls = (*env)->GetObjectClass(env, obj);
  mid = (*env)->GetMethodID(env, cls, "callback_setmechanism",
			    "(Ljava/lang/String;I)V");
  if (mid == 0) {
      throwexception(env,SASL_FAIL);
    return NULL;
  }

  VL(("mechusing=%s\n",mechusing));

  /* make into mech */
  jmechusing= (*env)->NewStringUTF(env,mechusing);

  /* do the callback */
  (*env)->CallVoidMethod(env, obj, mid,jmechusing);

  /* Because SASLv2 does not allow for persistance, we'll copy
   * it here */
  tmp = malloc(outlen);
  if(!tmp) {
      throwexception(env, SASL_NOMEM);
      return NULL;
  }
  
  memcpy(tmp, out, outlen);
  
  arr=(*env)->NewByteArray(env,outlen);
  (*env)->SetByteArrayRegion(env,arr, 0, outlen, (char *)tmp);

  return arr;
}

/* server step */

JNIEXPORT jbyteArray JNICALL Java_CyrusSasl_GenericServer_jni_1sasl_1server_1step

  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jbyteArray jarr, jint jlen)
{
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  int result;
  const char *out;
  unsigned int outlen;
  jbyteArray arr;
  char *in = NULL;
  char *tmp;
  
  if (jlen > 0)
      in = (*env)->GetByteArrayElements(env, jarr, 0);

  result=sasl_server_step(conn, (const char *) in, jlen,
			  &out, &outlen);

  if ((result!=SASL_OK) && (result!=SASL_CONTINUE))
  {
      VL (("Throwing exception! %d\n",result));
      /* throw exception */
      throwexception(env,result);
      return NULL;
  }

  if (result == SASL_OK) {
      setcomplete(env,obj);
  }

  if (jlen > 0)
      (*env)->ReleaseByteArrayElements(env, jarr,in ,0);

  /* Because SASLv2 does not allow for persistance, we'll copy
   * it here */
  tmp = malloc(outlen);
  if(!tmp) {
      throwexception(env, SASL_NOMEM);
      return NULL;
  }

  memcpy(tmp, out, outlen);
  
  arr=(*env)->NewByteArray(env,outlen);
  (*env)->SetByteArrayRegion(env,arr, 0, outlen, (char *)tmp);

  return arr;
}


/* 
 * Tell client we're done
 */
static int setcomplete(JNIEnv *env, jobject obj)
{
    jclass cls;
    jmethodID mid;

    VL (("Complete!\n"));
    
    /* set up for java callback */
    cls = (*env)->GetObjectClass(env, obj);
    mid = (*env)->GetMethodID(env, cls, "setcomplete",
				  "(I)V");
    if (mid == 0) {
	VL(("Can't find do_callbacks callback!!!\n"));
	return SASL_FAIL;
    }

    /* do the callback */
    (*env)->CallVoidMethod(env, obj, mid, 5);

    return SASL_OK;
}

/* client step */

JNIEXPORT jbyteArray JNICALL Java_CyrusSasl_GenericClient_jni_1sasl_1client_1step
    (JNIEnv *env, jobject obj, jint ptr, jbyteArray jarr, jint jlen)
{    
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  /*  const char *in = (*env)->GetStringUTFChars(env, jstr, 0);*/
  int result;
  sasl_interact_t *client_interact=NULL;
  const char *out;
  unsigned int outlen;
  jbyteArray arr;
  char *in;
  char *tmp;
  
  VL(("in client step\n"));

  if (jarr) {
      in = (*env)->GetByteArrayElements(env, jarr, 0);
      in[jlen]=0;
  } else {
      assert(jlen == 0);
       in = NULL;
  }

  VL(("in client step 2\n"));

  globalenv=env;
  globalobj=obj;

  do {
      result=sasl_client_step(conn, (const char *) in, jlen,
			      &client_interact,
			      &out, &outlen);

      VL(("in client step 3\n"));

      if (result==SASL_INTERACT) {
	  result = fillin_interactions(env,obj,client_interact);
      }
  } while (result==SASL_INTERACT);

  if ((result!=SASL_OK) && (result!=SASL_CONTINUE)) {
      /* throw exception */
      VL (("Throwing exception %d\n",result));
      throwexception(env,result);
      return NULL;
  }

  if (result == SASL_OK) {
      VL (("Setting complete\n"));
      setcomplete(env,obj);
  }

  if (jarr) {
      VL(("about to releasebytearrayelements\n"));
      (*env)->ReleaseByteArrayElements(env, jarr,in ,0);
  }
      
  /* Because SASLv2 does not allow for persistance, we'll copy
   * it here */
  tmp = malloc(outlen);
  if(!tmp) {
      throwexception(env, SASL_NOMEM);
      return NULL;
  }

  VL(("in client step 4\n"));

  memcpy(tmp, out, outlen);
  
  arr=(*env)->NewByteArray(env,outlen);
  (*env)->SetByteArrayRegion(env,arr, 0, outlen, (char *)tmp);

  VL(("returning arr\n"));
  return arr;
}


JNIEXPORT void JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1set_1prop_1string
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jint propnum, jstring val)
{
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  const char *value = (*env)->GetStringUTFChars(env, val, 0);

  int result=sasl_setprop(conn, propnum, value);

  if (result!=SASL_OK)
    throwexception(env,result);
}


JNIEXPORT void JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1set_1prop_1int
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jint propnum, jint jval)
{

  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  int value=jval;
  int result;

  VL(("sasl conn = %d\n",conn));
  VL (("propnum = %d\n",propnum));

  result=sasl_setprop(conn, propnum, &value);  

  VL (("setprop returned %d\n",result));

  if (result!=SASL_OK)
    throwexception(env,result);
}
JNIEXPORT void JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1set_1prop_1bytes
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jint propnum, jbyteArray jarr)
{
  char *value = (*env)->GetByteArrayElements(env, jarr, 0);
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  int result;

  result=sasl_setprop(conn, propnum, value);
  if (result!=SASL_OK)
    throwexception(env,result);

}

/* encode */
JNIEXPORT jbyteArray JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1encode
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, 
   jbyteArray jarr, jint jlen)
{
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  char *in = (*env)->GetByteArrayElements(env, jarr, 0);
  const char *out;
  unsigned int outlen;
  char *tmp;
  int result;
  jbyteArray arr;

  result=sasl_encode(conn,(const char *) in, jlen, &out, &outlen);
  if (result!=SASL_OK)
    throwexception(env,result);

  /* Because SASLv2 does not allow for persistance, we'll copy
   * it here */
  tmp = malloc(outlen);
  if(!tmp) {
      throwexception(env, SASL_NOMEM);
      return NULL;
  }

  memcpy(tmp, out, outlen);
  
  arr=(*env)->NewByteArray(env,outlen);
  (*env)->SetByteArrayRegion(env,arr, 0, outlen, (char *)tmp);

  return arr;
}

/* decode */
JNIEXPORT jbyteArray JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1decode
  (JNIEnv *env,
   jobject obj __attribute__((unused)), 
   jint ptr, jbyteArray jarr, jint jlen)
{

  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  char *in = (*env)->GetByteArrayElements(env, jarr, 0);
  const char *out;
  unsigned int outlen=9;
  char *tmp;
  int inlen=jlen;
  int result;
  jbyteArray arr;

  result=sasl_decode(conn, (const char *) in, inlen, &out, &outlen);
  if (result!=SASL_OK)
    throwexception(env,result);


  /* Because SASLv2 does not allow for persistance, we'll copy
   * it here */
  tmp = malloc(outlen);
  if(!tmp) {
      throwexception(env, SASL_NOMEM);
      return NULL;
  }

  memcpy(tmp, out, outlen);
  
  arr=(*env)->NewByteArray(env,outlen);
  (*env)->SetByteArrayRegion(env,arr, 0, outlen, (char *)tmp);

  (*env)->ReleaseByteArrayElements(env, jarr, in,0);

  return arr;

}

/*JNIEXPORT jbyteArray JNICALL Java_sasl_saslServerConn_jni_1sasl_1server_1decode
  (JNIEnv *env, jobject obj, jint ptr, jbyteArray in, jint inlen)
{
  return Java_sasl_saslClientConn_jni_1sasl_1client_1decode(env,obj,ptr,in,inlen);
  }*/

JNIEXPORT void JNICALL Java_CyrusSasl_CommonConn_jni_1sasl_1dispose
  (JNIEnv *env __attribute__((unused)),
   jobject obj __attribute__((unused)),
   jint ptr)
{
  sasl_conn_t *conn=(sasl_conn_t *) ptr;

  sasl_dispose(&conn);

}

JNIEXPORT jstring JNICALL Java_CyrusSasl_ServerFactory_jni_1sasl_1server_1getlist
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jstring jpre, jstring jsep, jstring jsuf)
{
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  const char *pre = (*env)->GetStringUTFChars(env, jpre, 0);
  const char *sep = (*env)->GetStringUTFChars(env, jsep, 0);
  const char *suf = (*env)->GetStringUTFChars(env, jsuf, 0);
  const char *list;
  unsigned int plen;
  jstring ret;

  int result=sasl_listmech(conn, NULL, pre, sep, suf, &list, &plen, NULL);

  if (result!=SASL_OK)
  {
    throwexception(env,result);  
    return NULL;
  }

  ret= (*env)->NewStringUTF(env,list);
  if (ret==NULL)
    throwexception(env, -1);

  return ret;
}

JNIEXPORT void JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1set_1server
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jbyteArray jarr, jint jport)
{
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  unsigned char *ip = (*env)->GetByteArrayElements(env, jarr, 0);
  char out[52];
  int result;

  sprintf(out, "%d.%d.%d.%d;%d", ip[0], ip[1], ip[2], ip[3], (int)jport);

  result=sasl_setprop(conn, SASL_IPREMOTEPORT, out);  

  VL(("Set IP_REMOTE: %s: %d\n",out, result));

  /* if not set throw an exception */
  if (result!=SASL_OK)
    throwexception(env,result); 
}



JNIEXPORT void JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1set_1client
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jbyteArray jarr, jint jport)
{
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  unsigned char *ip = (*env)->GetByteArrayElements(env, jarr, 0);
  char out[52];
  int result;

  sprintf(out, "%d.%d.%d.%d;%d", ip[0], ip[1], ip[2], ip[3], (int)jport);

  result=sasl_setprop(conn, SASL_IPLOCALPORT, out);

  VL(("Set IP_LOCAL: %s: %d\n",out, result));

  /* if not set throw and exception */
  if (result!=SASL_OK)
    throwexception(env,result);  
}

/* allocate a secprops structure */

static sasl_security_properties_t *make_secprops(int min,int max)
{
  sasl_security_properties_t *ret=(sasl_security_properties_t *)
    malloc(sizeof(sasl_security_properties_t));

  ret->maxbufsize=1024;
  ret->min_ssf=min;
  ret->max_ssf=max;

  ret->security_flags=0;
  ret->property_names=NULL;
  ret->property_values=NULL;

  return ret;
}


JNIEXPORT void JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1setSecurity
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr, jint minssf, jint maxssf)
{
  int result=SASL_FAIL;
  sasl_conn_t *conn=(sasl_conn_t *) ptr;
  sasl_security_properties_t *secprops=NULL;
  
  /* set sec props */
  secprops=make_secprops(minssf,maxssf);

  if (secprops!=NULL)
    result=sasl_setprop(conn, SASL_SEC_PROPS, secprops);  

  /* if not set throw and exception */
  if (result!=SASL_OK)
    throwexception(env,result);
}

JNIEXPORT jint JNICALL Java_CyrusSasl_GenericCommon_jni_1sasl_1getSecurity
  (JNIEnv *env,
   jobject obj __attribute__((unused)),
   jint ptr)
{
    int r = SASL_FAIL;
    sasl_conn_t *conn = (sasl_conn_t *) ptr;
    int *ssfp;

    r = sasl_getprop(conn, SASL_SSF, (const void **) &ssfp);
    if (r != SASL_OK) {
	throwexception(env, r);
    }

    return *ssfp;
}


