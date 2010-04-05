#!/usr/bin/python
import sys
   
from lwtestharness import *

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
