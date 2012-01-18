#!/usr/bin/python
import sys

from lwtestssh import Ssh
from lwtestssh import mysshError

from lwt_config import *

class TestHost:
    def __init__(self, hostname, root, password):
        self.__debuglevel   = DEBUG_LEVEL
        self.__inputhostname= hostname
        self.__hostname         = hostname
        self.__ipaddress    = ""
        self.__platform         = ""
        self.__platformid   = ""
        self.__distribution = ""
        self.__distroversion= ""
        self.__bitcompat    = ""
        self.__joineddomain = ""
        self.__joinedou         = ""
        self.__rootlogin    = root
        self.__rootpass         = password
        self.__rootconn         = Ssh(self.__rootlogin, self.__hostname)
        self.__userlogin    = root 
        self.__userpass         = password
        self.__connected    = False
        self.__domainssh    = Ssh("", self.__hostname)
        self.__loginerror   = False 
        self.__loginerrmsg  = ""
        
    def gethostname(self):
        return self.__hostname

    def sethostname(self, name):
        self.__hostname = name
    
    hostname = property(gethostname, sethostname)

    def getrootuser(self):
        return self.__rootlogin
    rootloginname = property(getrootuser)
    
    def getrootpass(self):
        return self.__rootpass
    rootpass = property(getrootpass)

    def getipaddress(self):
        return self.__ipaddress
    ipaddress = property(getipaddress)

    def getloginerror(self):
        return self.__loginerror
    loginerror = property(getloginerror)

    def getloginerrmsg(self):
        return self.__loginerrmsg
    loginerrmsg = property(getloginerrmsg)


    def getinputhostname(self):
        return self.__inputhostname
    inputhostname = property(getinputhostname)
  
    def getuserlogin(self):
        return self.__userlogin
    userloginname = property(getuserlogin)

    def getuserpass(self):
        return self.__userpass
    userpass = property(getuserpass)
 
    def getdomainssh(self):
        return self.__domainssh
    domainssh = property(getdomainssh)
 
    def getconnected(self):
        return self.__connected
    connected = property(getconnected)

    def getplatform(self):
        return self.__platform
    def setplatform(self, platform):
        self.__platform = platform
    platform = property(getplatform, setplatform)
    
    def getplatformid(self):
        return self.__platformid
    platformid = property(getplatformid)
    
    def getdistribution(self):
        return self.__distribution
    distribution = property(getdistribution)

    def getbitcompatibility(self):
        return self.__bitcompat
    bitcompat = property(getbitcompatibility)

    def getdistroversion(self):
        return self.__distroversion
    distroversion = property(getdistroversion)
    
    def getrootconn(self):
        return self.__rootconn
    rootconn = property(getrootconn)

    def rootlogin(self):
        self.__loginerror   = False 
        self.__loginerrmsg  = ""
        sys.stdout.write("Attempting to log into:" + self.__inputhostname + " as root\n")
        loggedintxt = self.__rootconn.login(self.__rootpass)
        if self.__rootconn.connected == True:
            self.__connected  = True
            #self.prompt = self.__rootconn.hostprompt
            if self.__debuglevel > 0:
                print "testharness::login is connected"
                print loggedintxt
                #print "Prompty received:" + self.prompt
            sys.stdout.write("Logged successfully to:" + self.__inputhostname + "\n")
        else:
            self.__rootconn.close()
            self.__connected  = False
            sys.stdout.write("Root login error for host:" + self.__hostname + "\nError:" + loggedintxt + "\n")
            self.__loginerrmsg = loggedintxt
        return self.__connected

    def domainlogin(self, username, password):
        self.__loginerror   = False 
        self.__loginerrmsg  = ""
        self.__domainssh.setparameter(username)
        sys.stdout.write("Attempting to log into:" + self.__inputhostname + " as " + username + "\n")
        loggedintxt = self.__domainssh.login(password)

        if self.__domainssh.connected:
            self.__connected  = True
            #self.prompt = self.__domainssh.hostprompt
            sys.stdout.write(loggedintxt + "\n")
            sys.stdout.write("Logged successfully to:" + self.__inputhostname + "\n")
            #print "Prompty received:" + self.prompt
        else:
            self.__domainssh.close()
            self.__connected  = False
            self.__TestCaseResult = "FAIL"
            self.__loginerrmsg = loggedintxt
            sys.stdout.write("Failed to login:" + self.__inputhostname + "\n")
        return self.__connected

    def findplatform(self):
        if self.__rootconn.connected:
            unameoutput = self.__rootconn.sendcmd("uname -s")
            #for each key in the platform dictionary, check if the key exists 
            #in the uname -a output. If it does, then assign that key as our
            #platform id
            for platform in platformdict.keys():
                if unameoutput.lower().find(platform.lower()) > -1:
                    self.__platformid = platformdict[platform]
                    self.__platform = platform
                    if self.__debuglevel > 0:
                        sys.stdout.write(unameoutput)
                    break
        else:
            if self.__debuglevel > 0:
                print "testharness::findplatform failed due to no ssh connection"

    def findbitversion(self):
        unameoutput = self.__rootconn.sendcmd("uname -m")
        if len(unameoutput.split("\n")) > 1:
            self.__bitcompat = unameoutput.split("\n")[1].strip()

    def finddistribution(self):
        if self.__rootconn.connected:
            if self.__platformid == platformdict["linux"]:
                etcreleasefiles = self.__rootconn.sendcmd("ls -l /etc/*release")
                for file in etcreleasefiles.split("\n"):
                    if file.find("lsb-release") != -1:
                        releaseoutput = self.__rootconn.sendcmd("cat /etc/*release | grep DISTRIB_ID")
                        if len(releaseoutput.split("\n")[1].split("=")) > 1:
                            self.__distribution = releaseoutput.split("\n")[1].split("=")[1].strip()
                        releaseoutput = self.__rootconn.sendcmd("cat /etc/*release | grep DISTRIB_RELEASE")
                        if len(releaseoutput.split("\n")[1].split("=")) > 1:
                            self.__distroversion = releaseoutput.split("\n")[1].split("=")[1].strip()
            
                if len(self.__distribution) == 0 or self.__distribution == "":
                    releaseoutput = self.__rootconn.sendcmd("lsb_release -a")
                    output = releaseoutput.split("\n")
                    DistroID = "Distributor ID"
                    Version = "Release" 

                    for value in output:
                        if value.lower().find(DistroID.lower()) > -1:
                            if len(value.split(":")) > 1:
                                self.__distribution = value.split(":")[1].strip()
                        elif value.lower().find(Version.lower()) > -1:
                            if len(value.split(":")) > 1:
                                self.__distroversion = value.split(":")[1].strip()
                if len(self.__distribution) == 0 or self.__distribution == "":
                    releaseoutput = self.__rootconn.sendcmd("vmware -v")
                    if len(releaseoutput.split("\n")) > 1:
                        systeminfo = releaseoutput.split("\n")[1].split(" ")
                        if len(systeminfo) > 1:
                            self.__distribution = systeminfo[1] 
                        if len(systeminfo) > 3:
                            self.__distroversion = systeminfo[3]

            elif self.__platformid == platformdict["sunos"]:
                releaseoutput = self.__rootconn.sendcmd("cat /etc/*release")
                if len(releaseoutput.split("\n")) > 1:
                    self.__distribution = releaseoutput.split("\n")[1].strip().split(" ")[0]
                    self.__distroversion = releaseoutput.split("\n")[1].strip().split(" ")[1]
                    if self.__debuglevel > 0:
                        print releaseoutput

            elif self.__platformid == platformdict["Darwin"]:
                unameoutput = self.__rootconn.sendcmd("uname -a")
                if len(unameoutput.split("\n")) > 1:
                    self.__distribution = unameoutput.split("\n")[1].strip().split(" ")[0]
                    self.__distroversion = unameoutput.split("\n")[1].strip().split(" ")[2]
                
            elif self.__platformid == platformdict["hp-ux"]:
                unameoutput = self.__rootconn.sendcmd("uname -a")
                if len(unameoutput.split("\n")) > 1:
                    self.__distribution = unameoutput.split("\n")[1].strip().split(" ")[0]
                    self.__distroversion = unameoutput.split("\n")[1].strip().split(" ")[2].replace("B.","")
            
            elif self.__platformid == platformdict["aix"]:
                osleveloutput = self.__rootconn.sendcmd("oslevel -r")
                self.__distribution = "aix"
                if len(osleveloutput.split("\n")) > 1:
                    self.__distroversion = osleveloutput.split("\n")[1].strip()
            
            if self.__debuglevel > 0:
                sys.stdout.write("DISTRIBUTION: " + self.__distribution + " " + self.__distroversion + "\n")
        else:
            if self.__debuglevel > 0:
                print "testharness::finddistribution failed due to no ssh connection"
            
    def findipaddress(self):
        if self.__rootconn.connected:
            if self.__platform.lower() == "linux":
                interfacenames = []
                interfacesoutput = self.__rootconn.sendcmd("ifconfig -s")
                interfaces = interfacesoutput.split("\n")
                for i in range(2,len(interfaces) -1):
                    while interfaces[i].find("  ") > -1:
                        interfaces[i] = interfaces[i].replace("  ", " ")
                    interfacenames.append(interfaces[i].split(" ")[0])
                
                for interface in interfacenames:
                    bfound = False
                    interfaceoutput = self.__rootconn.sendcmd("ifconfig " + interface + " | grep \"inet addr\"")
                    interface = interfaceoutput.split("\n")
                    for iface in interface:
                        if iface.lower().find("inet addr:") > -1:
                            self.__ipaddress = iface.split(":")[1].split(" ")[0]
                            if self.__ipaddress.find("127.0.0.1") == -1 and self.__ipaddress.strip() != "":
                                bfound = True
                                break
                    if bfound:
                        break

        
            elif self.__platform.lower() == "sunos" or self.__platform == "solaris" or \
                 self.__platform.lower() == "aix":
                interfacesoutput = self.__rootconn.sendcmd("ifconfig -a")
                interfaces = interfacesoutput.split("\n")
                for line in interfaces:
                    if line.find("inet") != -1:
                        items = line.split(" ")
                        for i in range(0, len(items)):
                            if items[i].lower().strip() == "inet" and \
                               items[i+1].find("127.0.0.1") == -1:
                                self.__ipaddress = items[i+1].strip()
                                break
                if self.__debuglevel > 0:
                    print interfacesoutput
                sys.stdout.write("UNIX IP Address: " + self.__ipaddress + "\n")
            elif self.__platform.lower() == "hpux" or self.__platform == "hp-ux":
                self.__ipaddress = self.__inputhostname
        else:
            if self.__debuglevel > 0:
                print "testharness::findipaddress failed due to no ssh connection"
            
    def findhostname(self):
        if self.__rootconn.connected:
            command = "hostname"
            hostnameoutput = " "
            while hostnameoutput.lower().strip().find(command.lower().strip()) == -1:
                hostnameoutput = self.__rootconn.sendcmd(command)           
                                                                    
            if len(hostnameoutput.split("\n")) > 1:
                self.__hostname = hostnameoutput.split("\n")[1].strip()
            else:
                self.__hostname = "Unknown"
        else:
            if self.__debuglevel > 0:
                print "testharness::findhostname failed due to no ssh connection"
            
    def finddomainjoinedto(self):
        if self.__rootconn.connected:
            domainjoinoutput = self.__rootconn.sudocmd("/opt/likewise/bin/domainjoin-cli query")
            print domainjoinoutput
            if domainjoinoutput.replace("\r","").find("\n") != -1 and domainjoinoutput.find("= ") != -1:
                if len(domainjoinoutput.split("\n")) > 4:
                    self.__joineddomain = domainjoinoutput.split("\n")[2].split("= ")[1].strip()
                    distinguishedname   = domainjoinoutput.split("\n")[3].split("= ")[1].strip()
                    elements = distinguishedname.split(",")
                    for element in elements:
                        if element.lower().startswith("ou="):
                            self.__joinedou = self.__joinedou + element.replace("OU=","/")
                    print "JOINED TO: " + self.__joineddomain
                    print "OU IS:     " + self.__joinedou
        else:
            if self.__debuglevel > 0:
                print "testharness::finddomainjoinedto failed due to no ssh connection"

    def zipwinbindlogs(self):
        if not self.__rootconn.connected:
            self.login
        if self.__rootconn.connected:
            tarlogs = self.__rootconn.sendcmd(" tar -cvf /tmp/" + self.__hostname + ".tar" + "/var/log/lwidentity/*")
            sys.stdout.write(tarlogs + "\n")
            self.logout
        else:
            if self.__debuglevel > 0:
                print "testharness::zipwinbindlogs failed due to no ssh connection"
            
    def rootlogout(self, isreboot = False):
        if self.__rootconn.connected:
            self.__connected  = False
            try:
                if isreboot == False:
                    self.__rootconn.sendcmd("exit")

                self.__rootconn.close()
            except mysshError:
                print "Attempted to write to a closed connection"
                sys.exit()

    def domainlogout(self):
        if self.__domainssh.connected:
            self.__connected  = False
            sys.stdout.write(self.__domainssh.sendcmd("exit"))
            sys.stdout.write(self.__domainssh.close())
 
