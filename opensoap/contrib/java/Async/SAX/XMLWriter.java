//----------------------------------------------------------------------------//
//  MODEL       :  OpenSOAP
//  GROUP       :  Use SAX
//  MODULE      :  XMLWriter.java
//  ABSTRACT    :  XML EscapeSequence Output Common Class
//                 [ ASYNC. Version ]
//  DATE        :  2002.02.01
//  DESIGNED    :  Sunbit System k.Kuwa
//----------------------------------------------------------------------------//
//  UpDate                                                                
//  No.         Registration Of Alteration            Date          User
//----------------------------------------------------------------------------//
//..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....8
import java.io.*;

public class XMLWriter extends PrintWriter {
    public XMLWriter(Writer writer) {
        super(writer);
    }
    public void escape(String s) throws IOException {
        for(int i = 0;i < s.length();i++) {
            char c = s.charAt(i);
            if (c == '<') {
                write("&lt;");
            } else if (c == '&') {
                write("&amp;");
            } else if(c == '\'') {
                write("&apos;");
            } else if(c == '"') {
                write("&quot;");
            } else if(c > '\u007f') {
                write("&#");
                write(Integer.toString(c));
                write(';');
            } else {
                write(c);
            }
        }
    }
}
