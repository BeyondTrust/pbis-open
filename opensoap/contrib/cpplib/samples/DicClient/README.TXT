Dictionary Client Application (DicClient)

This is an OpenSOAP implementation of a client program for the
Sanseido Publishing Co.'s sample XML Web Service dictionary service,
which can be found at http://www.btonic.com/ws/

This is a Windows application which can be compiled using Microsoft's
Visual C++ 6.0 and makes use of the newly added class libraries
located under contrib/cpplib.

To build this sample utilizing the OpenSOAP API, ensure that your
development environment is properly configured to correctly refer to
the OpenSOAP components.

1. In the [File] menu of VisualStudio, click on [Open Workspace] and
   select "dicClient.dsw".

2. In the [Project] menu, select [Settings...], and
    (a) In the "C/C++" tab, set the "Category" to Preprocessor, and
    add the appropriate value to the "Include  File Path" item
    (eg. C:\OpenSOAP\include).

    (b) In the "Link" tab, set the "Category" to Input, and add the
    appropriate value to the "Additional Libraries Path" item
    (eg. C:\OpenSOAP\lib\include).

However, at this time, 31st March 2003, the OpenSOAP API is not able
to correctly process the response message received from the above
service, and on a response, an error is generated.

As this sample is still under development, most of the comments and
some of the strings in the source code are written in Japanese.
