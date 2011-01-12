SSML Sample Analysis(Sample.ssml)


<?xml version='1.0' encoding='UTF-8' ?>
<SSML xmlns="x-schema:ssmlSchema.xml">


In the SSML(Soap Service Markup Language) file, the topmost element is <SSML>,
and includes a single <service> element that defines the service name. The SSML
file name is conveniently the same name as the service. 


<service name='Sample' nsuri='http://services.opensoap.jp/samples/Sample/'>


The <service> node contains at least 1 <connection> element, and optionally
multiple <operation> elements, 1 <fault> element and a maximum of 1
<MaxProcessNumber> elements.

Within the server, the operation name may be duplicated, and if necessary the
service name space is set in the <service> node nsuri attribute.


<connection name='Sample1'>
	<Socket hostname='localhost' port='8765'/>
	<asynchronizedTTL>8000</asynchronizedTTL>
	<synchronizedTTL count="second">20</synchronizedTTL>
	<MaxProccessNumber>5</MaxProccessNumber>
</connection>

<connection name='Sample2'>
        <StdIO>
                <exec prog='/usr/local/sbin/SampleService' option='-s -u'/>
	</StdIO>
	<asynchronizedTTL>8000</asynchronizedTTL>
	<synchronizedTTL count="second">20</synchronizedTTL>
	<MaxProccessNumber>5</MaxProccessNumber>
</connection>

<connection name='Sample3'>
	<HTTP>
	  <url>http://services.opensoap.jp/cgi-bin/TargetService.cgi</url>
	</HTTP>
	<asynchronizedTTL>8000</asynchronizedTTL>
	<synchronizedTTL count="second">20</synchronizedTTL>
	<MaxProccessNumber>5</MaxProccessNumber>
</connection>


How to connect to the service is described in the <connection> element. The
connection types
 - socket:
   <Socket hostname='hostname of service program' port='port number'/>
     (Registration required in inetd or xinetd. Refer to RegistService.txt)

 - Standard Input/Output:
   <StdIO> sub-element <exec prog='full path of standard input/output
    compatible program' option='command line parameters passed to the program'

 - HTTP:
   It is possible to specify a <url> in the <connection> element.
   The <url> entry contains the endpoint of the service being called.

 - Named pipe:<FIFO> (Not yet implemented)
 - IPC:<IPC> (Not yet implemented)
 - COM:<COM> (Not yet implemented)
 - Other connection modules:<Module> (Not yet implemented)
can be specified.


Furthermore, the asynchronous process timeout(=incomplete process queue, the
time after which spool data is cancelled)(in seconds)<asychronizedTTL>,
and the synchronous process timeout <synchronizedTTL> (count parameter in
seconds 'second', or message hop count 'hoptimes') can be set.
These can be specified simultaneously, the appropriate value is selected
depending on whether the request message is specified as synchronous or
asynchronous.

Regarding the hop count specified in 'hoptimes', if the number of transfers
needed to reach the appropriate server (received_path steps check) is greater
than the specified hoptimes, the service is not called and a Fault message is
returned.


 ** Similarly, these settings can also be defined in server.conf as illustrated
    below. The values in the SSML file, if they are defined, take precedence
    over the values in server.conf (irrespective of size). (Values should be
    greater than 0. For hoptimes, a value of 0 means no transfer).

 -- in server.conf
  <limit>
    <ttl>
      <asynchronousTTL>3600</asynchronousTTL>
      <synchronousTTL>600</synchronousTTL>
      <hoptimes>4</hoptimes>
    </ttl>
  </limit>
--


Use the <MaxProcessNumber> to specify the maximum number of connections using
this connection method.


<operation type ='Sample1'>add</operation>
<operation type ='Sample2'>sub</operation>


In the <operation> element, the name attribute of <connection> describes the
operation name of the specified connection type.

It makes no difference if several operations using different connection styles
use a single service.


<fault signature='1' />


The signature attribute('0' or '1') of the <fault> element is used to specify
whether the server signature is appended by the sever to returned Fault
message or not.

<MaxProccessNumber>15</MaxProccessNumber>


Use the <MaxProccessNumber> to limit the total number of connections to the
service.


------

LastModified: Aug, 31, 2003



