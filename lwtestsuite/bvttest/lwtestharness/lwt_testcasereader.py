#!/usr/bin/python
import sys
import csv
    
class TestCaseReader:
    def __init__(self, csvfile):
        self.__csvfile = open(csvfile, "rb")
        self.__reader = csv.reader(self.__csvfile)
        self.__rowcnt = 0
        self.__colcnt = 0
        self.CountRows()
        self.__fields = [] 
        self.__value = []
        
        
        
    def CountRows(self):
        rwcnt = 0
        clcnt = 0   
 
        for row in self.__reader:
            rwcnt += 1

        for col in range(0, len(row)):
            clcnt += 1

        self.__rowcnt = rwcnt
        self.__colcnt = clcnt

    
    def CopyField(self):
        self.__csvfile.seek(0)
        self.__reader = csv.reader(self.__csvfile)
        
        for row in self.__reader:
            self.__value.append(row)


    def CopyHeader(self):
        self.__csvfile.seek(0)
        self.__reader = csv.reader(self.__csvfile)
    
        for row in self.__reader:
            self.__fields = row
            break
            
    def PrintHeader(self):
        i = 0;
        for i in range(0, len(self.__fields)):
            sys.stdout.write(self.__fields[i])
            sys.stdout.write("\t")

    
    def PrintValue(self):
        for i in range(1, self.__rowcnt):
            for j in range(0, self.__colcnt):
                print self.__value[i][j]
                sys.stdout.write("\t");
            sys.stdout.write("\n")

    def GetTestCaseLine(self, index):
        return self.__value[index]
    
    def Close(self):
        close(csvfile)
        return
    

    def GetRowCount(self):
        return self.__rowcnt

    def GetColCount(self):
        return self.__colcnt


    
if __name__ == '__main__':
    from optparse import OptionParser

    parser = OptionParser()
    parser.add_option("-c", "--cases",
                      dest="testcases_file",
                      default="None",
                     )
    parser.add_option("-m", "--machines",
                      dest="testmachines_file",
                      default="None",
                     )
    parser.add_option("-n", "--name",
                      dest="testreport_file",
                      default="None",
                     )
    parser.add_option("-e", "--mail",
                      dest="email",
                      default="None",
                      type="string",
                     )
    options, remainder = parser.parse_args()
    
    if options.testcases_file == "None":
        sys.stdout.write("No test case file set. Use --help option for more info\n")
        sys.exit(1)

    if options.testmachines_file == "None":
        sys.stdout.write("No test machines file set. Use --help option for more info.\n")
        isys.exit(1)

    if options.testreport_file == "None":
        options.testreport_file = "LWTestReport"
        sys.stdout.write("No test result file set. Selecting default test result file LWTestReport.xls\n")


    TestHarness(options.testcases_file, 
                options.testmachines_file, 
                options.email, 
                options.testreport_file
                )
    sys.stdout.write("\n")
