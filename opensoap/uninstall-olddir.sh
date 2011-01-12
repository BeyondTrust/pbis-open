#!/bin/sh

#
################################################################
# OpenSOAP Server Uninstall Script Ver. 1.0
#
# http://opensoap.jp
################################################################
#

#
# Set Command
#
COMMAND='ls -l'
# COMMAND='rm -rf'
# COMMAND='tar -uvf OpenSOAP_Server.tar'

#
# Have A Command ?
#
if test "${COMMAND}" = ""
then
  echo No Command Specified.
  echo Exiting
  exit
fi


#
# Set Resource Locations
#
OPENSOAP_BIN=/usr/local/bin
OPENSOAP_LIB=/usr/local/lib
OPENSOAP_SERVICES=/usr/local/sbin
OPENSOAP_INC=/usr/local/include/OpenSOAP
OPENSOAP_CGI=/home/httpd/cgi-bin
OPENSOAP_CONF=/var/tmp/OpenSOAP


#
# Set The Verb
#
if test "${COMMAND}" = "ls -l"
then
  VERB="List"
fi

if test "${COMMAND}" = "rm -rf"
then
  VERB="Uninstall"
fi

if test "${COMMAND}" = "tar -uvf OpenSOAP_Server.tar"
then
  VERB="Tarball"
fi


echo
echo ${VERB} OpenSOAP Server and Components
echo

echo "Continue To" ${VERB} "OpenSOAP Server And Components? (y/n)"
read primary_response

if test "$primary_response" = y
then

  echo
  if "${VERB}" = "Uninstall"
  then
    echo Stopping The OpenSOAP Server
    ${OPENSOAP_BIN}/opensoap-server-ctl stop
  else
    ${COMMAND} ${OPENSOAP_BIN}/opensoap-server-ctl
  fi
  
  echo
  echo ${VERB} Server Components
  for i in ssmlAttrMgr srvConfAttrMgr reloadSrvConf fwderCreator \
           idManager queueManager msgDrvCreator spoolManager \
           srvDrvCreator ttlManager srvDrvCreator_async \
           queueManager_fwd Soaping opensoap-server-ctl
  do
    if test -f "${OPENSOAP_BIN}/$i"
    then
      echo ${OPENSOAP_BIN}/$i:
      ${COMMAND} ${OPENSOAP_BIN}/$i
    fi
  done
  
  echo
  echo ${VERB} OpenSOAP Libraries
  ${COMMAND} ${OPENSOAP_LIB}/libOpenSOAP*
  ${COMMAND} ${OPENSOAP_LIB}/libconnection.*
  ${COMMAND} ${OPENSOAP_LIB}/libSOAPMessage.*
  ${COMMAND} ${OPENSOAP_LIB}/libHTTPMessage.*

  echo
  echo ${VERB} Default OpenSOAP Services
  for i in SoapingService TransactionService
  do
    if test -f "${OPENSOAP_SERVICES}/$i"
    then
      echo ${OPENSOAP_SERVICES}/$i:
      ${COMMAND} ${OPENSOAP_SERVICES}/$i
    fi
  done

  echo
  echo
  echo ${VERB} "The OpenSOAP Sample Services? (y/n)"
  read uninstall_opensoap_samples
  if test "$uninstall_opensoap_samples" = y
  then
    echo
    echo ${VERB} OpenSOAP Sample Services
    for i in CalcService CalcAsyncService HelloService HelloSecService GetCertService \
             ShoppingService ShoppingServiceSec SimpleCalcService TransactionABankService
    do
      if test -f "${OPENSOAP_SERVICES}/$i"
      then
        echo ${OPENSOAP_SERVICES}/$i:
        ${COMMAND} ${OPENSOAP_SERVICES}/$i
      fi
    done
  fi

  echo
  echo ${VERB} OpenSOAP Header Files
  ${COMMAND} ${OPENSOAP_INC}

  echo
  echo ${VERB} OpenSOAP CGI Services
  for i in SoapingService.cgi TransactionService.cgi soapInterface.cgi
  do
    if test -f "${OPENSOAP_CGI}/$i"
    then
      echo ${OPENSOAP_CGI}/$i:
      ${COMMAND} ${OPENSOAP_CGI}/$i
    fi
  done

  echo
  echo
  echo ${VERB} "The OpenSOAP Sample CGI Services? (y/n)"
  read uninstall_opensoap_cgi_samples
  if test "$uninstall_opensoap_cgi_samples" = y
  then
    echo
    echo ${VERB} OpenSOAP CGI Services
    for i in CalcService.cgi CalcAsyncService.cgi HelloService.cgi HelloSecService.cgi GetCertService.cgi \
             ShoppingService.cgi ShoppingServiceSec.cgi SimpleCalcService.cgi TransactionABankService.cgi
    do
      if test -f "${OPENSOAP_CGI}/$i"
      then
        echo ${OPENSOAP_CGI}/$i:
        ${COMMAND} ${OPENSOAP_CGI}/$i
      fi
    done
  fi

#  echo ${VERB} OpenSOAP Server Configuration Files
#  for i in Data SOCKET conf log run
#  do
#    if test -d "${OPENSOAP_CONF}/$i"
#    then
#      echo ${OPENSOAP_CONF}/$i:
#      ${COMMAND} ${OPENSOAP_CONF}/$i
#    fi
#  done
#  if test -d "${OPENSOAP_CONF}"
#  then
#    echo ${OPENSOAP_CONF}:
#    ${COMMAND} ${OPENSOAP_CONF}
#  fi
  echo
  echo OpenSOAP Server Configuration, Log and Data Files can be found at
  echo  - ${OPENSOAP_CONF}/Data
  echo  - ${OPENSOAP_CONF}/SOCKET
  echo  - ${OPENSOAP_CONF}/conf
  echo  - ${OPENSOAP_CONF}/log
  echo  - ${OPENSOAP_CONF}/run
  echo If these are not needed they can be removed manually

fi

echo
echo Finished
echo
