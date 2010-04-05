About BvtTestHarness Tool
=========================
BvtTestHarness is an automation tool to execute and verify bvt test cases
for likewise product in testing distros. 

The Tool uses ssh utility to connect to various test machines and execute the 
BVT test cases. The tool validates a test case by comparing the actual result 
with the preconfigured expected result to determine a test case pass/fail. 
The result is then saved in a 'LWTestReport.xls' file. It can also mail the 
result file to set of email addresses passed in the command line argument.

REQUIREMENT:
Environment consists of Test machines rnning various linux distros to be tested 
with Domain Controllers, a Host machine

1. Host machine
Ensure that the host machine has SSH utility and mail service installed/enabled
Install xlwt package in the host machine.

Note: xlwt package is used to generate test report in excel sheet.

2. Test machine
The bvt test cases are executed in test machine by logging in as root user to
the test machine. 

3. Domain controller


CONFIGURATION:

Copy BvtTestHarness package to host machine. 

1.Update Test Machines in TestMachines.csv file
BvtTestHarness tool executes bvt test cases in multiple test machines 
sequencially. The machine details are specified in 'TestMachines.csv' file as 
comma separated values.

The format of each entry of test machine is 
#IP, Root Login, Password
    IP - the IP of the test machine
    Login - Administrator login name
    Password - Administrator login password
Update the IP, Login, password details of the machine to be tested in the 
TestMachines.csv file. The test machine detail in the file looks like
10.10.60.95,root,rootpassword

2.  Update bvt test cases in TestCases.csv
The BvtTestHarness tool uses bvt test cases as the input data for testing and 
as well comparing the test results. The bvt test cases are specified in csv 
file named  for example as 'TestCases.csv'. 

Test case format is #ID, Description, Platforms, Command, Expected Result
    ID - test case ID. Test case ID is used to differenciate each test case
    and the same id is reflected in test result file.
    
    Description - Describes the test case.
    
    Platforms - on which platform/s the test case has to be executed. The bvt
    test cases are run on multiple distros. The file location or some execution
    commands vary between distros. Platforms field is used to differenciate 
    the distros. For example, the likewise daemon path are different for aix and 
    hp-ux machines. In aix, the path is '/etc/rc.d/init.d/' wherein hp-ux it is
    '/sbin/init.d/'.

    Simple 'uname -a' in test machine gives the test machine platform, like 
    'aix' for AIX machine, 'Linux' for linux machines. 
 
    The platform option 'all', executes the test cases in all test distros. 
    Other options are 'linux' for linux distros,'hp-ux' for hp distros,'aix'
    for aix distros,'sunos' for solaris machines and 'Darwin' for MAC machines.
    
    Command - the test case command to be executed in test machine. The tool
    supports linux commands and also tool defines few api's. The tool logs in
    to the test machine using testuser credential to execute the command. So 
    its necessary to execute the commands using sudo command. All linux 
    commands which executed in testuser console follow the format 
    sudo(commiand). For example, sudo(/opt/likewise/bin/domainjoin-cli query).
    
    Following are the API's defined by the  tool 
    1. install()
    install api downloads the likewise build from shared location to test 
    machine and installs the likewise product in test machine. 
    The parameter for api are
    i. command to access shared location, specify username password if require
    ii. Path for the likewise build exe
    iii. Likewise build exe name
    iv. Path to save the likewise build

    The parameters are seperated by delimiter ||. 
    Example: install("smbclient \\\\10.10.60.5\\share -U administrator password
    ||Release_builds\7878\LWISO||
    LikewiseIdentityServiceOpen-5.4.0.7878-linux-i386-rpm.sh||/tmp/7878")

    2. domainuserlogin()
    The test machine is logged in with testuser credentials by default. But 
    couple of bvt testcase requires to login the test machine with domain user 
    credentials which is accomplished by the api domainuserlogin(). The api 
    logsout the testuser console and logs in as domain user using credentials. 
    The credentials i.e. domain username nad password are passed as api 
    parameters. For example, 
    domainuserlogin(PARENT\\bvtuser||password)

    After executing all commands in the testcase, the domain user is logged
    out and test machine is logged in as testuser again.

    Expected Result - Value to compare and verify the command execution result 

One of the file inputs is Expected Result. The Expected Result value for each 
test case has to be built up manually for the first time. The output of the 
command in successful scenario for each test case is copied to the Expected 
Result field.

Steps to update Expected Result are
    1. Ssh to one of test machine using root login. 
    2. Get the test case from the TestCases.csv, run the test case in 'Command'
       field value in console. 
    3. Collect the result and update the value in Expected Result field
     
The bvt test case supports multiple Command/Expected Result combination.
For example, the test case 
#10,Verify domain user login,Linux,id,id_value,groups,groups_value,pwd,pwd_value
    
The tool executes 'id' command and comapares the result with id_value. Then it 
executes 'groups' command and compares the result with groups_value
        
Multiple test cases are added to TestCases.csv file each test case 
differentiated with test case id

3.  Test Result:
Test results are saved in 'LWTestReport.xls' file. The file gives testing distro 
information, test cases ran on the distro with its result. If bvt test case 
fails it stores the actual test result of the machine.
 

TEST EXECUTION: 

1.  Update the test machine's detail in 'TestMachines.csv' and bvt test cases 
    along with the Expected Result in 'TestCases.csv' files

2.  Run the BvtTestHarness tool, for example, as
    ./BvtTestHarness.py -c TestCases.csv -m TestMachines.csv -n TestResults.txt -e EMAIL

    Where,
        -c <test case file name>, --cases=<test case file name>
        -m <test machines file name>, --machines=<test machines file name>
        -n <Test result file name>, --name=<Test result file name>
        -e <Email ID>, --mail=<Email ID>

3.  Check test results 
    Open the 'LWTestReport.xls' file and search for failed test cases. Compare the 
    actual result and expected result and verify the test case. Check the mail 
    if mailing option is selected.

Note: The BvtTestHarness tool is tested in Ubuntu 8.10 version and later. 
