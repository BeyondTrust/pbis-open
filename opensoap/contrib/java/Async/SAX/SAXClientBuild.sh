#!/bin/sh
#
XERCES_PATH=../../lib/xerces.ja
#
javac -classpath $XERCES_PATH:. OpenSoapConstants.java
javac -classpath $XERCES_PATH:. XMLWriter.java
javac -classpath $XERCES_PATH:. OpenSoapException.java
javac -classpath $XERCES_PATH:. OpenSoapEnvelope.java
javac -classpath $XERCES_PATH:. OpenSoapRequest.java
javac -classpath $XERCES_PATH:. CalcClientRequest.java
javac -classpath $XERCES_PATH:. CalcClientResult.java
javac -classpath $XERCES_PATH:. CalcClientPanel.java
javac -classpath $XERCES_PATH:. CalcClient.java
