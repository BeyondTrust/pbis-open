#!/usr/bin/python
import os
import sys
import time
import xlwt

from lwt_testcasereader import TestCaseReader
from lwt_testhost import TestHost
from lwt_testcase import TestCase
from lwtestssh import mysshError

from lwt_config import *


CONN_RST = "Connection reset by peer"

class TestHarness:
    def __init__(self, testcasesfile, hostsfile, email, suitename="LWTestReport"):
        self.__testfile  = testcasesfile
        self.__hostsfile = hostsfile
        self.__logfile   = suitename + ".xls"
        self.__logsep    = "\n"
        self.__logsepEx    = "\n\n"
        self.__emailaddress = email

        try:
            self.__testcasereader = TestCaseReader(self.__testfile)
            self.__testcasereader.CopyHeader();
            self.__testcasereader.CopyField()

            self.__hostread = open(self.__hostsfile, "r")

            self.__lwtestreport = xlwt.Workbook(encoding="utf-8")
            self.__testsummary  = self.__lwtestreport.add_sheet("Test Summary")
            self.__distroresult = 0
            self.AddTestSummaryHeader()
    
            self.TestMachines()
            self.__lwtestreport.save(self.__logfile)
            self.__hostread.close()

            self.MailResults(self.__emailaddress)
        except:
            self.__lwtestreport.save(self.__logfile)
            self.__hostread.close()
    
    def AddTestSummaryHeader(self):
        self.__testsummary.write(0, 0, "Host name")
        self.__testsummary.write(0, 1, "IP")
        self.__testsummary.write(0, 2, "Platform")
        self.__testsummary.write(0, 3, "Distribution")
        self.__testsummary.write(0, 4, "Processor Arch")
        self.__testsummary.write(0, 5, "Version")
        self.__testsummary.write(0, 6, "Total testcases")
        self.__testsummary.write(0, 7, "Pass")
        self.__testsummary.write(0, 8, "Fail")
        self.__testsummary.write(0, 9, "Skip")

    def TestMachines(self):
        machinecount = 0
        hostinfo = self.__hostread.readline()
        while hostinfo != "":
            try:
                if not hostinfo.startswith("#"):
                    host = hostinfo.split(",")
                    if len(host) >= 3: #ip, root login username, root password
                        machinecount += 1

                        testhost = TestHost(host[0], host[1], host[2])
                                       
                        if testhost.rootlogin():
                            testhost.findplatform()
                            testhost.finddistribution()
                            testhost.findipaddress()
                            testhost.findhostname()
                            testhost.findbitversion()

                            self.__testsummary.write(machinecount, 0, testhost.hostname)
                            self.__testsummary.write(machinecount, 1, testhost.ipaddress)
                            self.__testsummary.write(machinecount, 2, testhost.platform)
                            self.__testsummary.write(machinecount, 3, testhost.distribution)
                            self.__testsummary.write(machinecount, 4, testhost.bitcompat)
                            self.__testsummary.write(machinecount, 5, testhost.distroversion)
                            self.__lwtestreport.save(self.__logfile)

                            try:      
                                self.__distroresult = self.__lwtestreport.add_sheet(testhost.hostname)
                            except:
                                self.__distroresult = self.__lwtestreport.add_sheet(testhost.inputhostname)

                            self.AddTestingHeader()
                            Total, Pass, Fail = self.RunTestCases(testhost)
                            self.__testsummary.write(machinecount, 6, Total)
                            self.__testsummary.write(machinecount, 7, Pass)
                            self.__testsummary.write(machinecount, 8, Fail)
                            self.__testsummary.write(machinecount, 9, (Total - (Pass + Fail)))
                            self.__lwtestreport.save(self.__logfile)

                            testhost.rootlogout()
                        else:
                            testhost.__loginerror = True 
                            self.__testsummary.write(machinecount, 0, host[0])
                            self.__testsummary.write(machinecount, 8, testhost.loginerrmsg)
                        #testhost.zipwinbindlogs()
                hostinfo = self.__hostread.readline()
            except:
                hostinfo = self.__hostread.readline()
                pass
    

    def AddTestingHeader(self):
        self.__distroresult.write(0, 0, "Testcase ID")
        self.__distroresult.write(0, 1, "Description")
        self.__distroresult.write(0, 2, "Test Result")
        self.__distroresult.write(0, 3, "Test case")
        self.__distroresult.write(0, 4, "Expected Result")
        self.__distroresult.write(0, 5, "System Result")
        self.__distroresult.write(0, 6, "Test case")
        self.__distroresult.write(0, 7, "Expected Result")
        self.__distroresult.write(0, 8, "System Result")
        self.__distroresult.write(0, 9, "Test case")
        self.__distroresult.write(0, 10, "Expected Result")
        self.__distroresult.write(0, 11, "System Result")
        self.__distroresult.write(0, 12, "Test case")
        self.__distroresult.write(0, 13, "Expected Result")
        self.__distroresult.write(0, 14, "System Result")

    #Get all test cases from test case and execute them
    def RunTestCases(self,host):
        TotalTestCases = 0
        PassTestCases = 0
        FailTestCases = 0                                
        index = 0
        reportindex = 0
        try:
            maxrow = self.__testcasereader.GetRowCount()

            if index == maxrow:
                return TotalTestCases, PassTestCases, FailTestCases 

            testcase = TestCase(host)

            while index < maxrow:
                testcaseline = self.__testcasereader.GetTestCaseLine(index)

                if not str(testcaseline[0]).startswith("#"):
                    testcase.executetest(testcaseline)

                    if testcase.result != "NOT RUN":
                        TotalTestCases += 1
                        reportindex += 1
                        if str(testcase.result) == "FAIL":
                            FailTestCases += 1
                        elif str(testcase.result) == "PASS":
                            PassTestCases += 1
                        self.__distroresult.write(reportindex, 0, str(testcaseline[0]))
                        self.__distroresult.write(reportindex, 1, testcaseline[1])
                        self.__distroresult.write(reportindex, 2, str(testcase.result))

                        column = 0
                        for command, expected, actual in testcase.testcaselog:
                            if actual.strip().lower().find(CONN_RST.strip().lower()) > -1:
                                if testcase.sshconn.connected:
                                    host.rootlogout(True)
                                condition = True
                                timeout = 0
                                while (timeout < MAX_TIMEOUT and condition):
                                    if host.rootlogin():
                                        host.userloginname = host.rootloginname 
                                        host.userpass = host.rootpass
                                        testcase.sshconn = host.rootconn
                                        condition = False
                                    time.sleep(2)
                                    timeout += 2
        
                                if condition == True:
                                    self.__distroresult.write(reportindex, column, command)
                                    self.__distroresult.write(reportindex, column+1, expected)
                                    self.__distroresult.write(reportindex, 0, actual.replace(" | ","\n").strip())
                                    return 

                            column += 3 
                            self.__distroresult.write(reportindex, column, command)
                            self.__distroresult.write(reportindex, column+1, expected)
                            if str(testcase.result) == "PASS":
                                self.__distroresult.write(reportindex, column+2, expected)
                            else:
                                if actual.lower().find(expected.replace("\"","").lower().strip() ) != -1:
                                    self.__distroresult.write(reportindex, column+2, expected)
                                else:
                                    error = actual
                                    error = "\t" + error.replace(" | ","\n").strip()
                                    self.__distroresult.write(reportindex, column+2, error)

                    del testcase.testcaselog[:]
            
                    installcmd = "install"
                    domainjoincmd = "domainjoin"
                    regshellcmd = "regshellstart"
                    rebootcmd = "reboot"
                    expectedcmd = ""
                    error = ""
                    if str(testcase.result) == "FAIL":
                        if testcaseline[3].lower().strip().startswith(installcmd.lower().strip()):
                            error = "Error: Likewise product installation failed. Skipping all test cases till build uninstalls"
                            expectedcmd = "uninstall"
                        elif testcaseline[3].lower().startswith(regshellcmd.lower().strip()):
                            error = "Error: Failed to load regshell command tool. Skipping all regshell testcases"
                            expectedcmd = "regshellstop"
                        elif testcaseline[3].lower().strip().startswith(domainjoincmd.lower().strip()):
                            error = "Error: Domain join failed. Skipping all domain join test cases"
                            expectedcmd = "domainleave"
                        elif testcaseline[3].lower().strip().startswith(rebootcmd.lower().strip()): 
                            error = "Error: System is not rebooted. Skipping all test cases "
                            expectedcmd = "reboot"                    

                        if expectedcmd != "":
                            index += 1
                            while index < maxrow:
                                testcaseline = self.__testcasereader.GetTestCaseLine(index)
                                if not str(testcaseline[0]).startswith("#"):
                                    TotalTestCases += 1
                                    if testcaseline[3].lower().strip().startswith(expectedcmd.lower().strip()):
                                        break                        
                                index += 1
                            self.__distroresult.write(reportindex + 2, 0, "Error: build verification test fails")
                            self.__distroresult.write(reportindex + 3, 0, error)
                            reportindex += 3
                        
                testcaseline=""
                self.__lwtestreport.save(self.__logfile)
                index += 1

            return TotalTestCases, PassTestCases, FailTestCases 
        except:
            return TotalTestCases, PassTestCases, FailTestCases 

    def MailResults(self, emailaddress):
        self.__logfile
        if emailaddress != "None":
            os.system("uuencode \"" + self.__logfile + "\" \"" + self.__logfile + "\" | mail -s \"Test Result\" " + emailaddress)
    


