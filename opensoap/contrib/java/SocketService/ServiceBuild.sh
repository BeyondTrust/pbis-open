#!/bin/sh
XERCES_PATH=../lib/xerces.jar
javac -classpath $XERCES_PATH:. OpenSoapConstants.java
javac -classpath $XERCES_PATH:. XMLWriter.java
javac -classpath $XERCES_PATH:. OpenSoapException.java
javac -classpath $XERCES_PATH:. CalcServiceResponse.java
javac -classpath $XERCES_PATH:. SocketService.java
