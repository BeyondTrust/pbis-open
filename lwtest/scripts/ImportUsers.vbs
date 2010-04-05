'Global variables
 Dim oContainer
 Dim OutPutFile
 Dim FileSystem

 'Initialize global variables
 Set FileSystem = WScript.CreateObject("Scripting.FileSystemObject")
 Set InputFile = FileSystem.OpenTextFile("AllUsers.txt", 1)

 ' Connect to Active Directory, Users container
 Set objRootLDAP = GetObject("LDAP://rootDSE")
 Set objContainer = GetObject("LDAP://cn=Users," & objRootLDAP.Get("defaultNamingContext"))


  headers = InputFile.ReadLine
  headerArray = CSVParse(headers)

  nameIndex = FindIndex(headerArray, "name")
  lastADFieldIndex = FindIndex(headerArray, "lastADField")
  passwordIndex = FindIndex(headerArray, "password")

  if ( nameIndex = -1 ) then
    Wscript.quit(1)
  end if

  Do Until InputFile.AtEndOfStream
    line = InputFile.ReadLine

    dataArray = CSVParse(line)


    Set objNewUser = objContainer.Create("User", dataArray(nameIndex))

    For i = 0 to lastADFieldIndex - 1
      If ( i <> nameIndex AND Not IsEmpty(dataArray(i)) ) then
        WScript.StdOut.WriteLine headerArray(i) & " : " & dataArray(i)
        objNewUser.Put headerArray(i), dataArray(i)
      End If
    Next

    objNewUser.SetInfo ' Write it to AD

    If ( passwordIndex <> -1 AND Not IsEmpty(dataArray(passwordIndex)) ) Then
      objNewUser.SetPassword(dataArray(passwordIndex))
    End If 
    
  Loop


Function FindIndex(ByRef headerArray, ByVal str)
  FindIndex = -1
  For i = 0 to Ubound(headerArray)
    If ( headerArray(i) = str ) then
      FindIndex = i
    end if
  Next
End Function

Function CSVParse(ByVal strLine)
    ' Function to parse comma delimited line and return array
    ' of field values.

    Dim arrFields
    Dim blnIgnore
    Dim intFieldCount
    Dim intCursor
    Dim intStart
    Dim strChar
    Dim strValue

    Const QUOTE = """"
    Const QUOTE2 = """"""

    ' Check for empty string and return empty array.
    If (Len(Trim(strLine)) = 0) then
        CSVParse = Array()
        Exit Function
    End If

    ' Initialize.
    blnIgnore = False
    intFieldCount = 0
    intStart = 1
    arrFields = Array()

    ' Add "," to delimit the last field.
    strLine = strLine & ","

    ' Walk the string.
    For intCursor = 1 To Len(strLine)
        ' Get a character.
        strChar = Mid(strLine, intCursor, 1)
        Select Case strChar
            Case QUOTE
                ' Toggle the ignore flag.
                blnIgnore = Not blnIgnore
            Case ","
                If Not blnIgnore Then
                    ' Add element to the array.
                    ReDim Preserve arrFields(intFieldCount)
                    ' Makes sure the "field" has a non-zero length.
                    If (intCursor - intStart > 0) Then
                        ' Extract the field value.
                        strValue = Mid(strLine, intStart, _
                            intCursor - intStart)
                        ' If it's a quoted string, use Mid to
                        ' remove outer quotes and replace inner
                        ' doubled quotes with single.
                        If (Left(strValue, 1) = QUOTE) Then
                            arrFields(intFieldCount) = _
                                Replace(Mid(strValue, 2, _
                                Len(strValue) - 2), QUOTE2, QUOTE)
                        Else
                            arrFields(intFieldCount) = strValue
                        End If
                    Else
                        ' An empty field is an empty array element.
                        arrFields(intFieldCount) = Empty
                    End If
                    ' increment for next field.
                    intFieldCount = intFieldCount + 1
                    intStart = intCursor + 1
                End If
        End Select
    Next
    ' Return the array.
    CSVParse = arrFields
End Function
