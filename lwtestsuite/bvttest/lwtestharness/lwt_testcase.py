#!/usr/bin/python
import os
import sys
import time
import thread
import select

from lwt_config import *

USERADD_OPTION=" -m -s /bin/bash "

class TestCase:
    def __init__(self, host):
        self.__debuglevel = DEBUG_LEVEL
        self.__TestPass = False
        self.__configured = False
        self.__host = host.hostname
        self.__ipaddress = host.ipaddress
        self.__testplatform = host.platform
        self.__loggedintxt = ""
        self.__tcoutput = []
        self.__output = ""
        self.__summarysep = "\n"
        self.__stepslog = ""
        self.__testcaselog = []
        self.__sshconn = host.rootconn
        self.__context = host
        self.__domainlogin = False
        self.__configured = True
        self.__TestCaseResult = "NOT RUN"
        self.__builddirectory = ""
        self.__lwbuild = ""

    
    def getid(self):
        return self.__id
    id = property(getid)
    
    def gethost(self):
        return self.__context
    host = property(gethost)

    def getsshconn(self):
        return self.__sshconn
    sshconn = property(getsshconn)

    def gethostname(self):
        return self.__host
    hostname = property(gethostname)
    
    def getipaddress(self):
        return self.__ipaddress
    ipaddress = property(getipaddress)
    
    def getdescription(self):
        return self.__description
    description = property(getdescription)
    
    def getusername(self):
        return self.__username
    username = property(getusername)
    
    def getpassword(self):
        return self.__password
    password = property(getpassword)

    def gettestcaselog(self):
        return self.__testcaselog
    testcaselog = property(gettestcaselog)
    
    def getplatformlist(self, platform):
        platformlist = []
        platforms = platform.replace("\"","").split(",")
        for key in platformdict.keys():
            for testplatform in platforms:
                testplatform = str(testplatform)
                testplatform = testplatform.lstrip(" ")
                if testplatform.lower().replace("\"","").strip() == "all":
                    for key in platformdict.keys():
                        if key != "unknown":
                            platformlist.append(key)
                    return platformlist
                if key.lower().strip() == testplatform.lower().strip():
                    platformlist.append(key)
        #for platform in platforms:
        #    platform = platform.lstrip()
        #    platformlist.append(platformdict[platform.lower()])
        #sys.stdout.write(str(platformlist))
        return platformlist
    
    def gettestresult(self):
        return self.__TestCaseResult
    result = property(gettestresult)
    
    def getstepslog(self):
        return self.__stepslog
    def setstepslog(self,nothing):
        return
    stepslog = property(getstepslog)
    
    def checkplatform(self):
        self.__configured = False
        if self.__platforms.lower().find("all") > -1:
            self.__configured = True
            return
        
        for platform in platformdict.keys():
            if self.__platforms.lower().find(platform.lower().strip()) > -1:
                self.__configured = True
                return
        #self.__sshconn.close()
            
    def login(self,username, password):
        self.__domainlogin = True
        loggedin = False 
        if self.__configured:
            loggedin = self.host.domainlogin(username, password)
            if loggedin == True:
                self.__sshconn = self.host.domainssh
            else:
                self.__TestCaseResult = "FAIL"
        else:
            self.__TestCaseResult = "FAIL"
    
        return loggedin

    def executetest(self, testcaseline):
        platforms    = testcaseline[2]
        tcsteps          = []
        platformlist= []
        matchplatform = False
        
        platformlist = self.getplatformlist(platforms)
        for testplatform in platformlist:
            if (self.__testplatform.lower() == testplatform.lower().strip() or testplatform.lower().strip() == "all"):
                matchplatform = True
                break
        
        if not matchplatform:
            self.__TestCaseResult = "NOT RUN"
            return

        if self.__debuglevel > 0:
            sys.stdout.write("\nTest platform: " + self.__testplatform + "\n")
            sys.stdout.write("Targeted platforms: " + str(platforms) + "\n")

        expectedattr = len(testcaseline) - 3
        for i in range(3, len(testcaseline), 2):
            if testcaseline[i] is None or testcaseline[i] == "" :
                continue
            else:
                steppair = []
                steppair.append(testcaseline[i])
                if expectedattr == 1:
                    steppair.append("")
                else:           
                    steppair.append(testcaseline[i+1])
                tcsteps.append(steppair)
                expectedattr -= 2

        if self.__sshconn.connected:
            for command, expectedoutput in tcsteps:
                if self.__debuglevel:
                    print "\ntestharness::executetest(): " + command
                
                if command.lower().strip().startswith("install"):
                    actualoutput = self.install(command)    
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("uninstall("):
                    actualoutput = self.uninstall(command)    
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)
 
                elif command.lower().strip().startswith("sudo("):
                    actualoutput = self.runsudocmd(command[5:len(command) -1]).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("reboot"):
                    actualoutput = self.runrebootcmd(command).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("ping"):
                    actualoutput = self.runpingcmd(command).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("regshellstart("):
                    actualcmd = command.replace("regshellstart(", "").replace(")", "")
                    actualoutput = self.runcmd(actualcmd).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("regshellstop("):
                    actualcmd = "exit" 
                    actualoutput = self.runcmd(actualcmd).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("userlogin("):
                    actualoutput = self.userlogin(command).replace("\n", " | ").replace("\r","")
                    # The code is work around to fix the bug - user login fails some time 
                    expectedoutput = "SUCCESS"
                    if actualoutput.lower().find(expectedoutput.lower()) == -1:
                        actualoutput = self.userlogin(command).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("scp("):
                    actualoutput = self.scp(command).replace("\n", " | ").replace("\r","")
                    # The code is work around to fix the bug - user login fails some time 
                    expectedoutput = "SUCCESS"
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("systemlogin("):
                    timeout = command.replace("systemlogin(","").replace(")","")
                    if timeout == '' or "": 
                        timeout = '10'
                    print "Test case executing for machine:" + self.host.inputhostname
                    print "Do you want login to physical machine[yes/no]:"
                    Input = ''
                    ExpectedIn = 'yes'
                    if len(select.select([sys.stdin.fileno()], [], [], float(timeout))[0])>0:
                        Input += os.read(sys.stdin.fileno(), 4096)
                        if Input.lower().strip().startswith("yes"):
                            raw_input("After testing enter any key to continue:")
                    step = [command, expectedoutput, ""]

                elif command.lower().strip().startswith("domainjoin("):
                    actualoutput = self.domainjoin(command).replace("\n", " | ").replace("\r","")
                    # The code is work around to fix the bug - domain join fails first time and 
                    # subsequent domain join suceeds
                    lsasserror = "Error"
                    if actualoutput.lower().find(lsasserror.lower()) > -1:
                        actualoutput = self.domainjoin(command).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)
                    time.sleep(20)
                    
                elif command.lower().strip().startswith("domainleave("):
                    actualoutput = self.domainjoin(command).replace("\n", " | ").replace("\r","")
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)
                    
                elif command.lower().strip().startswith("startgpagent"):
                    actualoutput = self.startgpagent(command, expectedoutput)   
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().find("domainjoin-cli") > -1 and command.lower().strip().find("query") > -1:
                    actualoutput = self.runcmd_readall(command)   
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

                elif command.lower().strip().startswith("useradd("):
                    print "Received command is: " + command
                    newcmd = command.replace("useradd(","").replace(")","")
                    attributes = newcmd.split("||")
                    if len(attributes) < 2:
                        sys.stderr.write("Invalid useradd command: ")
                        errormsg = "Invalid parameters for userlogin. Valid parameters are username||password"
                        step = [command, expectedoutput, errormsg]
                        self.__testcaselog.append(step)
                        break

                    useraddcmd = "useradd" + USERADD_OPTION + attributes[0]
                    if useraddcmd[-1] != '\n':
                        useraddcmd += '\n'
        
                    userexists = "user " + attributes[0] + " exists"
                    actualoutput = self.__sshconn.sendcmd(useraddcmd)
                    if actualoutput.lower().find(userexists.lower()) != -1:
                        self.__TestPass   = False
                        self.__TestCaseResult = "FAIL"
                        step = [command, expectedoutput, actualoutput]
                        self.__testcaselog.append(step)
                    else:
                        userpasscmd = "passwd " + attributes[0]
                        if userpasscmd[-1] != '\n':
                            userpasscmd += '\n'

                        actualoutput = self.__sshconn.sendcmd(userpasscmd)

                        newpass = "password:"
                        if actualoutput.lower().find(newpass.lower()) != -1:
                            actualoutput = self.__sshconn.sendcmd(attributes[1])
                            retype = "Retype"
                            reenter = "Reenter"
                            reenterpass = "password:"
                            if actualoutput.lower().find(reenterpass.lower()) != -1 and \
                                (actualoutput.lower().find(retype.lower()) != -1 or \
                                actualoutput.lower().find(reenter.lower()) != -1):

                                actualoutput = self.__sshconn.sendcmd(attributes[1])
                                success1 = "successfully"
                                success2 = "Password changed"
                                if actualoutput.lower().find(success1.lower()) > -1 or \
                                    actualoutput.lower().find(success2.lower()) > -1:
                                    step = [command, expectedoutput, actualoutput]
                                    self.__testcaselog.append(step)
                                else:
                                    self.__sshconn.sendcmd(CTRL_C)
                                    self.__TestPass   = False
                                    self.__TestCaseResult = "FAIL"
                                    step = [command, expectedoutput, actualoutput]
                                    self.__testcaselog.append(step)
                            else:
                                self.__sshconn.sendcmd(CTRL_C)
                                self.__TestPass   = False
                                self.__TestCaseResult = "FAIL"
                                step = [command, expectedoutput, actualoutput]
                                self.__testcaselog.append(step)
                        else:
                            self.__TestPass   = False
                            self.__TestCaseResult = "FAIL"
                            step = [command, expectedoutput, actualoutput]
                            self.__testcaselog.append(step)
    
                elif command != "" and expectedoutput != "\n":
                    actualoutput = self.__loggedintxt + self.runcmd(command).replace("\n", " | ").replace("\r","",3)
                    step = [command, expectedoutput, actualoutput]
                    self.__testcaselog.append(step)

            self.verifytest()

            if self.__domainlogin == True:
                self.logout()
                if not self.host.rootlogin():
                    print "Root log in Error:" + self.host.loginerrmsg
                self.host.userloginname = self.host.rootloginname 
                self.host.userpass = self.host.rootpass
                self.__sshconn = self.host.rootconn

        else:
            for command, expectedoutput in tcsteps:
                if command != "" and expectedoutput != "\n":
                    step = [command, expectedoutput, "ERROR: Connection failure for the test machine. Aborting testing"]
                    self.__testcaselog.append(step)    
                    break
            self.verifytest()


    def startgpagent(self, command, expectedoutput):
        if command.lower().strip().startswith("startgpagent("):
            actualcommand = command.replace("startgpagent(","").replace(")","")
            
        #stop gpagent service
        newcmd = "/opt/likewise/bin/lwsm restart gpagent\n"
        ExpectedResult = "Starting service: gpagent"
        output = self.runcmd(newcmd).replace("\n", " | ").replace("\r","")

        #if output.lower().find(ExpectedResult.lower()) == -1:
        #   return output


        # Send the gpagent start command 
        actualoutput = self.runcmd(actualcommand).replace("\n", " | ").replace("\r","")
        success = "Completed refreshing computer group policies" 
        error   = "No such file or directory"
        condition = True
        timeout = 0
        while (timeout < MAX_TIMEOUT and condition):
            if actualoutput.lower().find(success.lower()) > -1 or actualoutput.lower().find(error.lower()) > -1:
                condition = False
            else:
                time.sleep(1)
                timeout += 1
                actualoutput += self.__sshconn.read_very_lazy()
                
        #Send control C command
        output = self.runcmd(CTRL_C)
        return actualoutput
    
    def userlogin(self, command):
        newcmd = command.replace("userlogin(","").replace(")","")
        attributes = newcmd.split("||")
        if len(attributes) < 2:
            sys.stderr.write("Invalid login command: ")
            errormsg = "Invalid commands for userlogin. Valid commands are username||password"
            return errormsg
                    
        self.host.userloginname = str(attributes[0])
        self.host.userpass = str(attributes[1])
        self.host.rootlogout()
        if not self.login(attributes[0], attributes[1]):
            return self.host.loginerrmsg
        else:
            return "SUCCESS"

    
    def scp(self, command):
        newcmd = command.replace("scp(","").replace(")","")
        attributes = newcmd.split("||")
        if len(attributes) < 2:
            sys.stderr.write("Invalid arguement for file copy command: ")
            errormsg = "Invalid command for scp. Valid command scp source:file dest:file||password"
            return errormsg
                    
        if not self.filecp(attributes[0].strip(), attributes[1].strip()):
            return "FAILURE" 
        else:
            return "SUCCESS"

    def filecp(self, command, password):
        """copies files from source to destination
        """
        searchkey='password'
        banner = self.runcmd(command.strip(), 60)

        if (banner.lower().find(searchkey.lower()) >= 0):
            response, abort = self.prompt(banner, "password", password)

            if abort:
                return banner

            if response != "":
                response = response + '\n'
                banner = self.runcmd(response)

        return banner
    
    def prompt(self, message, key, password):
        foundprompt = (message.lower().find(key) >= 0)
        abort = 0
        try:
            if foundprompt:
                response = password #getpass.getpass(key)
            elif (message.lower().find('sa key fingerprint') >= 0):
                if DEBUG_LEVEL:
                    print "\nFound DSA/RSA key fingerprint request"
                """Found DSA/RSA key fingerprint request"""
                response = "yes"
            elif (message.lower().find('denied') >=0):
                if DEBUG_LEVEL:
                    print "\nPermission denied from host"
                response = ''
                abort = 1
            else:
                response = ""
        except KeyboardInterrupt:
            response = ''
            abort = 1
        return response, abort

    def domainjoin(self, command):
        if command.lower().strip().startswith("domainjoin("):
            newcmd = command.replace("domainjoin(","").replace(")","")
        if command.lower().strip().startswith("domainleave("):
            newcmd = command.replace("domainleave(","").replace(")","")

        actualoutput = "Error"

        attributes = newcmd.split("||")
                
        if len(attributes) == 1:
            actualoutput = self.runcmd(attributes[0]).replace("\n", " | ").replace("\r","")
            success = "SUCCESS"
            error     = "Error"
            condition = True
            timeout = 0

            while (timeout < MAX_TIMEOUT and condition):
                if actualoutput.lower().find(success.lower()) > -1 or actualoutput.lower().find(error.lower()) > -1:
                    condition = False
                else:
                    time.sleep(1)
                    timeout += 1
                    actualoutput += self.__sshconn.read_very_lazy()

        elif len(attributes) == 2:
            actualoutput = self.runcmd(attributes[0]).replace("\n", " | ").replace("\r","")

            success = "password:"
            error     = "Error"
            condition = True
            timeout = 0

            while (timeout < MAX_TIMEOUT and condition):
                if actualoutput.lower().find(success.lower()) != -1:
                    if success == "password:":
                        actualoutput = self.__sshconn.sendcmd(attributes[1], 5).strip() 
                        success = "SUCCESS"
                        timeout = 0
                    elif success == "SUCCESS":
                        condition = False

                elif actualoutput.lower().find(error.lower()) != -1:
                    condition = False
                else:
                    time.sleep(1)
                    timeout += 1
                    actualoutput += self.__sshconn.read_very_lazy()
        else:    
            actualoutput = "Error: Invalid input parameter" 

        return actualoutput


    def uninstall(self, command):
        host = self.__context
        command = command.replace("uninstall(","").replace(")","").strip()
        if host.distribution.find("Darwin") > -1:
            uninstallcmd = "/opt/likewise/bin/macuninstall.sh"
        else:
            if command != "":
                uninstallcmd = "sh " + command + " uninstall"
            elif self.__builddirectory != "" and self.__lwbuild != "":
                build = self.__builddirectory + "/" + self.__lwbuild
                uninstallcmd = "sh " + build + " uninstall"
            else:
                return "ERROR: Failed to locate likewise build. Please provide the complete path to uninstall"
            
        success = "Success"
        error   = "Failed to uninstall packages"
        invalidpath = "No such file or directory"
        condition = True
        timeout = 0
        self.__output = self.runcmd(uninstallcmd).replace("\n", " | ").replace("\r","")

        if self.__output.lower().strip().find(invalidpath.lower().strip()) > -1:
            return self.__output

        while (timeout < MAX_TIMEOUT and condition):
            if self.__output.lower().find(success.lower()) > -1 or \
                self.__output.lower().find(error.lower()) != -1:
                condition = False    
            else:                               
                time.sleep(1)
                timeout += 1                             
                output = self.__sshconn.read_very_lazy()                                
                self.__output += output
        
        if condition:
            self.__output += "\nERROR:REQUEST_TIMEDOUT"

        return self.__output
 
    def install(self, command):
        host = self.__context
        newcmd = command.replace("install(","").replace(")","")
        attributes = newcmd.split("||")
        lwbuild = " "
        
        if len(attributes) == 0 and self.__builddirectory == "":
            sys.stderr.write("Invalid install command: ")
            return "Invalid install command in test case file"
        elif len(attributes) > 1:
            #TODO: The root path varies for solaris
            if host.distribution.find("Darwin") > -1:
                path = "/var/root/Desktop"
            else:
                path = "/root/Desktop"

            if attributes[0].lower().find("smbclient") == -1:
                installoutput = "Tool is supporting only smbclient command to download installables"
                return installoutput
            elif len(attributes) == 3:
                if os.path.isdir(attributes[2]):
                    path = attributes[2]
                else:
                    sys.stderr.write("Specified directory '" + attributes[2] + "' doesn't exist. Selecting default path " + path)
                 
            command = "cd " + path
            self.__builddirectory = path                                    
            output = self.__sshconn.sendcmd(command)
            if output.find("No such file or directory") > -1 or output.find("command not found") > -1:
                return output

        if len(attributes) > 1:
            sys.stdout.write("\nSending input:" + attributes[0]) 
            output = self.__sshconn.sendcmd(attributes[0],2)

            smbloginerr = "failed"
            smbsuccess = "smb: \>"
            condition = True
            timeout = 0

            while (timeout < MAX_TIMEOUT and condition):
                if output.lower().find(smbsuccess.lower()) > -1:
                    condition = False
                elif output.lower().find(smbloginerr.lower()) != -1:
                    return output
                else:
                    time.sleep(1)
                    timeout += 1
                    out = self.__sshconn.read_very_lazy()
                    output += out

            if condition:
                output += "\nERROR:REQUEST_TIMEDOUT"
                return output
            else:
                smbpatherror = "NT_STATUS_OBJECT_PATH_NOT_FOUND"
                smbnameerror = "NT_STATUS_OBJECT_NAME_NOT_FOUND"
                sys.stdout.write("\nSending input:" + attributes[1]) 
                command = "cd " + attributes[1]
                output = self.__sshconn.sendcmd(command)
                if not output.lower().find(attributes[1].replace("\"","").lower().strip()) != -1 or \
                    output.lower().find(smbpatherror.lower().strip()) > -1 or \
                    output.lower().find(smbnameerror.lower().strip()) > -1:
                    self.__sshconn.sendcmd("q")
                    return output
                else:
                    smbfileerr = "NT_STATUS_NO_SUCH_FILE"
                    buildlist = self.__sshconn.sendcmd("ls")
                    resultcmd = self.getlwbuild(buildlist).strip()
                    if resultcmd == "":
                        self.__sshconn.sendcmd("q")
                        return "Failed to locate the likewise build.Please provide the complete path to install"
                    newcmd = resultcmd.split(" ")
                    lwbuild = newcmd[0]
                    command = "mget " + lwbuild 
                    output = self.__sshconn.sendcmd(command)
                    expected = lwbuild + "?"
                    if output.lower().find(expected.lower()) == -1 or output.lower().find(smbfileerr.lower()) > -1:
                        self.__sshconn.sendcmd("q")
                        return output
                    else:
                        output = self.__sshconn.sendcmd("y",5)
                        if not output.lower().find(attributes[1].replace("\"","").lower().strip()) != -1:
                            self.__sshconn.sendcmd("q")
                            return output
                        else:
                            output = self.__sshconn.sendcmd("q")
                            print "Success"


            self.__lwbuild = lwbuild 
        else:
            command = "ls "
            if attributes[0].strip() == "":
                command = command + self.__builddirectory
            else:
                command = command + attributes[0].strip()
                self.__builddirectory = attributes[0].strip()
                
            buildlist = self.__sshconn.sendcmd(command)
            newcmd = self.getlwbuild(buildlist).strip()
            if newcmd == "":
                return "Failed to locate the likewise build. Please provide the complete path to install"
            print "\n\nBuild received:" + newcmd
            resultcmd = ""
            filelist = newcmd.split(" ")
            for file in filelist:
                if 'LikewiseIdentityService' in file:
                    resultcmd = file 

            if 'LikewiseIdentityServiceOpen' in resultcmd:
                if host.distribution.find("Darwin") == -1:
                    self.__lwbuild = "LikewiseIdentityServiceOpen*.sh"  
                else:
                    self.__lwbuild = "LikewiseIdentityServiceOpen*.dmg"  
            elif 'LikewiseIdentityServiceEnterprise' in resultcmd:
                if host.distribution.find("Darwin") == -1:
                    self.__lwbuild = "LikewiseIdentityServiceEnterprise*.sh"  
                else:
                    self.__lwbuild = "LikewiseIdentityServiceEnterprise*.dmg"  
            else:
                return "Failed to locate the likewise build. Please provide the complete path to install"

            dirlen = len(self.__builddirectory)
            buildlen = len(self.__lwbuild)

            lwbuild = self.__builddirectory + '/' + self.__lwbuild
            expectedtext = ""
           
        print "\n\nInstall host.distribution:" + host.distribution
        if host.distribution.find("Darwin") > -1:
            unpackdmg = "hdiutil attach " + lwbuild
            self.__output = self.__sshconn.sendcmd(unpackdmg)
            folder = self.__lwbuild
            folder = folder.replace(".dmg","")
            intovolume = "cd /Volumes/" + folder
            self.__output = self.__sshconn.sendcmd(intovolume)
            folder = self.__lwbuild
            installer = folder.replace("dmg","mpkg/")
            installcmd = "installer -pkg " + installer + " -target /"
            expectedtext = "The install was successful"
        else:
            installcmd = "sh " + lwbuild + " install"
            expectedtext = "Install complete"

        licenceaccpt = "Do you accept the terms of these licenses? (yes/no)"
        installstmnt = "Would you like to install now? (yes/no)"
        compatibilitystmt = "Would you like to install 32-bit compatibility libraries? (auto/yes/no)"
        errortext = "Failed to install packages"
        invalidpath = "No such file or directory"
        condition = True
        timeout = 0

        self.__output = self.__sshconn.sendcmd(installcmd)
        if self.__output.lower().strip().find(invalidpath.lower().strip()) > -1:
            return self.__output


        while (timeout < MAX_TIMEOUT + 30 and condition):
            if self.__output.lower().strip().find(expectedtext.lower().strip()) > -1 or \
                self.__output.lower().strip().find(errortext.lower().strip() ) > -1:
                condition = False
            else:
                if True == self.islicensestmt(self.__output):
                    self.__output = self.__sshconn.sendcmd('q')
                    print self.__output
            
                if self.__output.lower().find(licenceaccpt.replace("\"","").lower().strip() ) != -1:
                    self.__output = self.__sshconn.sendcmd("yes", 2)
                    print self.__output
                  
                    output = self.__output.split("\n") 
                    length = len(output)
                    stractual = output[length - 1]
                
                    if(installstmnt.lower().strip() == stractual.lower().strip()):
                        self.__output = self.__sshconn.sendcmd("yes", 2)
                        print self.__output

                        output = self.__output.split("\n") 
                        length = len(output)
                        stractual = output[length - 1]

                        if(compatibilitystmt.lower().strip() == stractual.lower().strip()):
                            self.__output = self.__sshconn.sendcmd("auto", 2)
                            print self.__output
                        continue

                time.sleep(1)
                timeout += 1
                self.__output += self.__sshconn.read_very_lazy()

        if (self.__output.find(expectedtext) != -1):
            return expectedtext 
        else:
            return self.__output


    def getlwbuild(self, buildlist):
        host = self.__context
        build = buildlist.split("\n")
       
        print "\n\nhost.distribution:"  + host.distribution
        print "\n\nhost.bitcompat:" + host.bitcompat
        for buildvalue in build:
            print "\n\nBuild:" + buildvalue
            if host.distribution.find("Ubuntu") > -1 or host.distribution.find("Debian") > -1:
                if buildvalue.lower().find("deb") > -1:
                    if host.bitcompat.find("i686") > -1 or host.bitcompat.find("i386") > -1:
                        if buildvalue.lower().find("i386-deb.sh") > -1:
                            return buildvalue
                    elif host.bitcompat.find("x86_64") > -1:
                        if buildvalue.lower().find("amd64-deb.sh") > -1:
                            return buildvalue
            elif host.distribution.find("CentOS") > -1 or host.distribution.find("SUSE") > -1 or \
            host.distribution.find("RedHat") > -1 or host.distribution.find("Fedora") > -1 or \
            host.distribution.find("ESX") > -1:
                if buildvalue.lower().find("rpm") > -1:
                    if host.bitcompat.find("i686") > -1 or host.bitcompat.find("i386") > -1:
                        if buildvalue.lower().find("i386-rpm.sh") > -1:
                            return buildvalue
                    elif host.bitcompat.find("x86_64") > -1:
                        if buildvalue.lower().find("x86_64-rpm.sh") > -1:
                            return buildvalue
            elif host.distribution.find("Darwin") > -1:
                if buildvalue.lower().find("dmg") > -1:
                    if host.bitcompat.find("Power Macintosh") > -1 or host.bitcompat.find("x86_64") > -1 or host.bitcompat.find("i386") > -1:
                        if host.bitcompat.find("Power Macintosh") > -1:
                            if buildvalue.lower().find('powerpc.dmg') > -1:
                                return buildvalue
                        elif host.bitcompat.find("x86_64") > -1:
                                if buildvalue.lower().find('universal.dmg') > -1:
                                    return buildvalue
                        elif host.bitcompat.find("i386") > -1:
                                if buildvalue.lower().find('i386.dmg') > -1:
                                    return buildvalue
            elif host.distribution.find("Solaris") > -1:
               print "\n\nbuild value:" +  buildvalue
               if buildvalue.lower().find("solaris") > -1:
                   if host.bitcompat.find("i86pc") > -1 or host.bitcompat.find("i386") > -1:
                       if buildvalue.lower().find("i386-pkg.sh") > -1:
                           return buildvalue
                   elif host.bitcompat.find("x86_64") > -1:
                       if buildvalue.lower().find("386-pkg.sh") > -1:
                           return buildvalue

        return ""

    
    def islicensestmt(self, output):
        hplicense = "Standard input"
        suselicense = "--More--"
        opensuselicense = "(END)"
        opensuseenterprise = "lines"
        
        if output.lower().find(hplicense.replace("\"","").lower().strip() ) != -1:
            return True
        elif output.lower().find(suselicense.replace("\"","").lower().strip() ) != -1:
            return True
        elif output.lower().find(opensuselicense.replace("\"","").lower().strip() ) != -1:
            return True
        elif output.lower().find(opensuseenterprise.replace("\"","").lower().strip() ) != -1:
            return True
            
        return False


    def runsudocmd(self, command):
        if command == "":
            return ""
        
        if self.__sshconn.connected:
            self.__output = self.__sshconn.sudocmd(command).strip()
            self.__tcoutput.append(self.__output)
            sys.stdout.write(self.__output)
        return self.__output

    
    def runrebootcmd(self, command):
        if command == "":
            return ""
        MaxTimeOut = 600
        if self.__sshconn.connected:
            if command.lower().find("reboot(") > -1:
                tempcmd = command
                MaxTimeOut = tempcmd.replace("reboot(","").replace(")","")
                command = "reboot"
            self.__output = self.runcmd(command).strip()
            self.host.rootlogout(True)
            print 'Thread ID:', thread.get_ident()
            time.sleep(45)
            condition = True
            timeout = 0
            while (timeout < MaxTimeOut and condition):
                if self.host.rootlogin():
                    self.host.userloginname = self.host.rootloginname 
                    self.host.userpass = self.host.rootpass
                    self.__sshconn = self.host.rootconn
                    condition = False
                time.sleep(2)
                timeout += 2

            if condition == True:
                self.__output = "System didnot start after rebooting"

        return self.__output
    

    def runpingcmd(self, command):
        if command == "":
            return ""
    
        if self.__sshconn.connected:
            if command.lower().find(" -c ") == -1:
                command = command.replace("ping","ping -c 1")

        success = "ping statistics"
        error   = "unknown host"
        condition = True
        timeout = 0

        self.__output = self.runcmd(command).strip()

        while (timeout < MAX_TIMEOUT and condition):
            if self.__output.lower().find(success.lower()) != -1 or self.__output.lower().find(error.lower()) != -1:
                condition = False
            else:
                time.sleep(2)
                timeout += 5
                output = self.__sshconn.read_very_lazy()
                self.__output += output

        if condition:
            self.__output = self.runcmd(CTRL_C)
            self.__output = "Ping command timeout"
        
        return self.__output

    def runcmd(self, command, timeout=0):
        if command == "":
            return
        
        if self.__sshconn.connected:    
            if not command.lower().strip().startswith("passwd("):
                self.__output = self.__sshconn.sendcmd(command, timeout).strip()
                self.__tcoutput.append(self.__output)
                sys.stdout.write(self.__output)
            else:
                newcmd = command.replace("passwd(","").replace(")","")
                attributes = newcmd.split("||")
                if len(attributes) < 2:
                    sys.stderr.write("Invalid passwd command: ")
#                    self.__configured = False
                    self.__TestPass   = False    
                    self.__TestCaseResult = "FAIL"
                    return "Invalid arguments for passwd command"
                self.__oldpassword = attributes[0].strip()
                self.__newpassword = attributes[1].strip()
                
                self.__output = self.__sshconn.changepass(self.__oldpassword, self.__newpassword)
                self.__tcoutput.append(self.__output)
                sys.stdout.write(self.__output)
        return self.__output

    def runcmd_readall(self, command):
        if self.__sshconn.connected:    
            if not command.lower().strip().startswith("passwd("):
                return self.__sshconn.runcmd_readall(command)
            else:
                newcmd = command.replace("passwd(","").replace(")","")
                attributes = newcmd.split("||")
                if len(attributes) < 2:
                    sys.stderr.write("Invalid passwd command: ")
                    self.__TestPass   = False   
                    self.__TestCaseResult = "FAIL"
                    return "Invalid arguments for passwd command"
                self.__oldpassword = attributes[0].strip()
                self.__newpassword = attributes[1].strip()
                self.__output = self.__sshconn.changepass(self.__oldpassword, self.__newpassword)
                self.__tcoutput.append(self.__output)
                sys.stdout.write(self.__output)
        return self.__output

    def verifytest(self):
        results = []
        
        self.__stepslog=""        
        for command, expected, actual in self.__testcaselog:
            if expected.lower().find("|") != -1:
                ResultList = expected.split("|")
                ResultCount = len(ResultList)
                CompareResult = False;
                for i in range(0, ResultCount):
                    expected = ResultList[i]
                    if expected.strip() == "*":
                        CompareResult = True;
                        break
                    elif expected.lower().strip().find( "not(" ) == 0:
                        if (actual.lower().strip().find( expected[4:len(expected) - 1].lower().strip() ) == -1) and \
                            (expected[4:len(expected) - 1].lower().strip() not in actual.lower().strip()):
                            CompareResult = True;
                            break
                    elif (actual.lower().strip().find( expected.replace("\"","").lower().strip() ) != -1) or \
                        (expected.lower().strip() in actual.lower().strip()):
                        CompareResult = True;
                        break

                results.append(CompareResult)
            elif expected.lower().find("&") != -1:
                ResultList = expected.split("&")
                ResultCount = len(ResultList)
                FinalResult = True
                for i in range(0, ResultCount):
                    CompareResult = False;
                    expected = ResultList[i]
                    if expected.strip() == "*":
                        CompareResult = True;
                    elif expected.lower().strip().find( "not(" ) == 0:
                        if (actual.lower().strip().find( expected[4:len(expected) - 1].lower().strip() ) == -1) and \
                            (expected[4:len(expected) - 1].lower().strip() not in actual.lower().strip()):
                            CompareResult = True;
                    elif (actual.lower().strip().find(expected.replace("\"","").lower().strip() ) > -1) or \
                        (expected.lower().strip() in actual.lower().strip()):
                        CompareResult = True;
            
                    FinalResult = FinalResult and CompareResult

                results.append(FinalResult)

            else:
                if expected.strip() == "*":
                    results.append(True)
                elif expected.lower().strip().find( "not(" ) == 0:
                    if (actual.lower().strip().find( expected[4:len(expected) - 1].lower().strip() ) == -1) and \
                        (expected[4:len(expected) - 1].lower().strip() not in actual.lower().strip()):
                        results.append(True)
                    else:
                        results.append(False)
                elif (actual.lower().strip().find( expected.replace("\"","").lower().strip() ) != -1) or \
                    (expected.lower().strip() in actual.lower().strip()):
                    results.append(True)
                else:
                    results.append(False)

        self.__TestPass = True
        for result in results:
            self.__TestPass = self.__TestPass and result

        if self.__TestPass:
            self.__TestCaseResult = "PASS"
        else:
            self.__TestCaseResult = "FAIL"

        return
    
    def betaverifytest(self, expected, actual):
        if expected.startswith("and("):
            i = 1
        return
    
    def logout(self):
        self.__domainlogin = False
        if self.host.connected:
            self.host.domainlogout()

 
    def ToString(self):
        return self.__testsummary + "\n"
        
