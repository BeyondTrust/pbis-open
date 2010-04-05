' Make these command line arguments, someday
' Some code in this file comes from examples found on the web which
' generally had no licensing information attached.

strFQDN = "EXAMPLE.com"
WriteUsers "users.csv", strFQDN
WriteGroups "groups.csv", strFQDN
WScript.Quit(0)




Sub WriteUsers(strFilename, strFQDN)
    Set objRootDSE = Nothing
    Set objUserContainer = Nothing
    
    Set FileSystem = Nothing
    Set OutputFile = Nothing


    'Initialize variables
    Set objRootDSE = GetObject("LDAP://rootDSE")
    Set objUserContainer = GetObject("LDAP://" & objRootDSE.Get("defaultNamingContext") )
    set objOU = GetObject("LDAP://" & objRootDSE.Get("defaultNamingContext"))
    objOU.Filter = Array("organizationalUnit")
    
    Set FileSystem = WScript.CreateObject("Scripting.FileSystemObject")
    
    'enumerate in default ou
    Set OutputFile = FileSystem.CreateTextFile("computers." + strFileName, True)
    WriteUsersHeader OutputFile
    EnumerateUsers objRootDSE, strFQDN,"", objUserContainer, OutputFile 
    Set OutputFile = Nothing

    For Each objChild In objOU
	strFile = objChild.ou + "." + strFilename
        Set OutputFile = FileSystem.CreateTextFile(strFile, True)
        'Enumerate Container
        WriteUsersHeader OutputFile
        EnumerateUsers objRootDSE, strFQDN, objChild.ou, objUserContainer, OutputFile 
        'Clean up
        OutputFile.Close
        Set OutputFile = Nothing
        Set strFile = Nothing
    Next

    Set FileSystem = Nothing
    Set objUserContainer = Nothing
    Set objRootDSE = Nothing
End Sub


'-------------------------------------------------------------------


Sub WriteUsersHeader(OutputFile)

  OutputFile.Write "NTName,"
  OutputFile.Write "SamAccountName,"
  OutputFile.Write "Sid,"
  OutputFile.Write "UserPrincipalName,"
  OutputFile.Write "FQDN,"
  OutputFile.Write "NetBiosName,"
  OutputFile.Write "DistinguishedName,"


  OutputFile.Write "name,"
  OutputFile.Write "st,"
  OutputFile.Write "givenName,"          ' given name, first name, personal name
  OutputFile.Write "sn,"                 ' last name, surname, family name
  OutputFile.Write "initials,"           '
  OutputFile.Write "description,"        '


  OutputFile.Write "UnixUid,"
  OutputFile.Write "UnixGid,"
  OutputFile.Write "UnixGecos,"
  OutputFile.Write "UnixLoginShell,"
  OutputFile.Write "UnixHomeDirectory,"


  OutputFile.Write "MarkerField1,"


  OutputFile.Write "Password,"                   ' Set the account password here. There is not field in AD named this.
  OutputFile.Write "bCanChangePassword,"         ' Controls whether the user can change there password. Use dsmod user <UserDN> -canchpwd (no|yes)
  OutputFile.Write "PasswordLastSetDate,"        ' This is in AD, but may have issue. See AD Cookbook. Set 0 to force change next logon.
  OutputFile.Write "bPasswordNeverExpires,"      ' Use dsmod user "<UserDN" -pwdneverexpires (no|yes) 
  OutputFile.Write "PasswordExpirationDate,"     

  OutputFile.Write "UserEnabled,"
  OutputFile.Write "UserLockedOut,"
  OutputFile.Write "AccountExpired,"
  OutputFile.Write "PasswordExpired,"

  OutputFile.Write "lastField"          'Last field in our Cvs file, just makes some things easier.

  OutputFile.WriteLine

End Sub


'-------------------------------------------------------------------

Sub EnumerateUsers(objRootDSE, strFQDN, strOU, objUserContainer, OutputFile)
  Set objUser = Nothing

  strNetBiosName = Get_NetBiosName(objRootDSE, strFQDN)

  For Each objUser In objUserContainer

    Select Case LCase(objUser.Class)
      Case "user"
        if (Get_UserInOU(objUser.sAMAccountName, strOU)) then

          WriteIt OutputFile, Get_NTName(strNetBiosName, objUser.sAMAccountName)
          WriteIt OutputFile, objUser.sAMAccountName
          WriteIt OutputFile, ObjSidToStrSid(objUser.objectSid)
          WriteIt OutputFile, objUser.UserPrincipalName
          WriteIt OutputFile, strFQDN
          WriteIt OutputFile, strNetBiosName
          WriteIt OutputFile, objUser.distinguishedName

          WriteIt OutputFile, objUser.name
          WriteIt OutputFile, objUser.st
          WriteIt OutputFile, objUser.givenName
          WriteIt OutputFile, objUser.sn
          WriteIt OutputFile, objUser.initials
          WriteIt OutputFile, objUser.description
   
          WriteIt OutputFile, Get_UID(objUser.sAMAccountName, strOU)
          WriteIt OutputFile, Get_GID(objUser.sAMAccountName, strOU)
          WriteIt OutputFile, Get_Gecos(objUser.sAMAccountName, strOU)
          WriteIt OutputFile, Get_LoginShell(objUser.sAMAccountName, strOU)
          WriteIt OutputFile, Get_UnixHomeDir(objUser.sAMAccountName, strOU)

          WriteIt OutputFile, "MarkerField1"    ' lastADField

          WriteIt OutputFile, "Password"
          WriteIt OutputFile, Get_CanChangePassword(objUser)
          WriteIt OutputFile, Get_PasswordLastSet(objUser)
          WriteIt OutputFile, Get_PasswordNeverExpires(objUser) ' "pwdNeverExpires"
          WriteIt OutputFile, Get_PasswordExpirationDate(objUser)

          WriteIt OutputFile, Get_AccountEnabled(objUser)
          WriteIt OutputFile, Get_AccountLocked(objUser)
          WriteIt OutputFile, Get_AccountExpired(objUser)
          WriteIt OutputFile, Get_PasswordExpired(objUser)

          ' lastField doesn't need to be printed, previous field printed comma
          OutPutFile.WriteLine 
        End if

      Case "organizationalunit" , "container"
        EnumerateUsers objRootDSE, strFQDN, strOU, objUser, OutputFile
    End Select
  Next

End Sub

'-------------------------------------------------------------------
Sub WriteGroups(strFilename, strFQDN)
    Set objRootDSE = Nothing
    Set objGroupContainer = Nothing

    Set FileSystem = Nothing
    Set OutputFile = Nothing

    Set objRootDSE = GetObject("LDAP://rootDSE")
    Set objGroupContainer = GetObject("LDAP://" & objRootDSE.Get("defaultNamingContext") )
    set objOU = GetObject("LDAP://" & objRootDSE.Get("defaultNamingContext"))
    objOU.Filter = Array("organizationalUnit")
    
    Set FileSystem = WScript.CreateObject("Scripting.FileSystemObject")
    
    'enumerate in default ou
    Set OutputFile = FileSystem.CreateTextFile("default." + strFileName, True)
    WriteGroupsHeader OutputFile
    EnumerateGroups objRootDSE, strFQDN, "", objGroupContainer, OutputFile 

    Set OutputFile = Nothing
   
    For Each objChild In objOU
	strFile = objChild.ou + "." + strFilename
        Set OutputFile = FileSystem.CreateTextFile(strFile, True)
        'Enumerate Container
        WriteGroupsHeader OutputFile
        EnumerateGroups objRootDSE, strFQDN, objChild.ou, objGroupContainer, OutputFile 
        'Clean up
        OutputFile.Close
        Set OutputFile = Nothing
        Set strFile = Nothing
    Next

    Set FileSystem = Nothing
    Set objGroupContainer = Nothing
    Set objRootDSE = Nothing

End Sub
'-------------------------------------------------------------------

Sub WriteGroupsHeader(OutputFile)

  OutputFile.Write "DistinguishedName,"
  OutputFile.Write "userPrincipalName,"
  OutputFile.Write "NetBiosName,"
  OutputFile.Write "SamAccountName,"
  OutputFile.Write "NTName,"
  OutputFile.Write "FQDN,"
  OutputFile.Write "Name,"
  OutputFile.Write "givenName,"          ' given name, first name, personal name
  OutputFile.Write "description,"        '
  OutputFile.Write "Sid,"          ' Will need SId
  OutputFile.Write "Gid,"          ' Will need SId
  OutputFile.Write "objectGUID,"          ' 
  OutputFile.Write "Members,"

  OutputFile.Write "Password,"          'Set the account password here. There is not field in AD named this.
  'OutputFile.Write "canChangePassword," 'Controls whether the user can change there password. Use dsmod user <UserDN> -canchpwd (no|yes)
  'OutputFile.Write "pwdLastSet,"        ' This is in AD, but may have issue. See AD Cookbook. Set 0 to force change next logon.
  'OutputFile.Write "pwdNeverExpires,"   ' Use dsmod user "<UserDN" -pwdneverexpires (no|yes) 
  OutputFile.Write "lastField"          'Last field in our Cvs file, just makes some things easier.

  OutputFile.WriteLine

End Sub


'-------------------------------------------------------------------

Sub EnumerateGroups(objRootDSE, strFQDN, strOU, objGroupContainer, OutputFile)
  Dim oGroup
  Dim Members

  strNetBiosName = Get_NetBiosName(objRootDSE, strFQDN)

  For Each oGroup In objGroupContainer
    Select Case LCase(oGroup.Class)
      Case "group"
        if (Get_GroupInOU(oGroup.sAMAccountName, strOU)) then
          WriteIt OutputFile, oGroup.distinguishedName
          WriteIt OutputFile, oGroup.userPrincipalName
          WriteIt OutputFile, strNetBiosName
          WriteIt OutputFile, oGroup.sAMAccountName
          WriteIt OutputFile, Get_NTName(strNetBiosName, oGroup.sAMAccountName)
          WriteIt OutputFile, strFQDN
          WriteIt OutputFile, oGroup.name
          WriteIt OutputFile, oGroup.givenName
          WriteIt OutputFile, oGroup.description
          WriteIt OutputFile, ObjSidToStrSid(oGroup.objectSid)
          'WriteIt OutputFile, "1111111111"
          WriteIt OutputFile, Get_GGID(oGroup.sAMAccountName, strOU)
          WriteIt OutputFile, ObjSidToStrSid(oGroup.objectGUID)
          Members = Empty
          For Each objMember in oGroup.Members
      	    if IsEmpty(Members) then
	      Members = objMember.Name
	    else
	      Members = Members + "," + objMember.Name
	    end if        
          Next
          WriteIt OutputFile, Members

          WriteIt OutputFile, "password"
      '    WriteIt OutputFile, "canChangePassword"
      '    WriteIt OutputFile, "pwdLastSet"
      '    WriteIt OutputFile, "pwdNeverExpires"

          ' lastField doesn't need to be printed, previous field printed comma
          OutPutFile.WriteLine 
        End if
  
      Case "builtindomain" , "organizationalunit" , "container"  
        EnumerateGroups objRootDSE, strFQDN, strOU, oGroup , OutputFile
    End Select
  Next

End Sub


'-------------------------------------------------------------------


Sub WriteIt(OutputFile, str)
 if not IsEmpty(str) then
   OutputFile.Write """" & str & """" & ","
 else
   OutputFile.Write ","
 end if
End Sub





'-------------------------------------------------------------------
' Rem http://msdn.microsoft.com/en-us/library/aa772300(VS.85).aspx
  Const ADS_UF_SCRIPT                                   = &H1
  Const ADS_UF_ACCOUNTDISABLE                           = &H2
  Const ADS_UF_HOMEDIR_REQUIRED                         = &H8
  Const ADS_UF_LOCKOUT                                  = &H10
  Const ADS_UF_PASSWD_NOTREQD                           = &H20
  Const ADS_UF_PASSWD_CANT_CHANGE                       = &H40
  Const ADS_UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED          = &H80
  Const ADS_UF_TEMP_DUPLICATE_ACCOUNT                   = &H00
  Const ADS_UF_NORMAL_ACCOUNT                           = &H200
  Const ADS_UF_INTERDOMAIN_TRUST_ACCOUNT                = &H800
  Const ADS_UF_WORKSTATION_TRUST_ACCOUNT                = &H1000
  Const ADS_UF_SERVER_TRUST_ACCOUNT                     = &H2000
  Const ADS_UF_DONT_EXPIRE_PASSWD                       = &H10000
  Const ADS_UF_MNS_LOGON_ACCOUNT                        = &H20000
  Const ADS_UF_SMARTCARD_REQUIRED                       = &H40000
  Const ADS_UF_TRUSTED_FOR_DELEGATION                   = &H80000
  Const ADS_UF_NOT_DELEGATED                            = &H100000
  Const ADS_UF_USE_DES_KEY_ONLY                         = &H200000
  Const ADS_UF_DONT_REQUIRE_PREAUTH                     = &H400000
  Const ADS_UF_PASSWORD_EXPIRED                         = &H800000
  Const ADS_UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION   = &H1000000



'----------------------------------------------------------------------

Function Get_AccountLocked(byval objUser)
	on error resume next
	set objLockout = objUser.get("lockouttime")

	if err.number = -2147463155 then
		Get_AccountLocked = 0
		exit Function
	end if
	on error goto 0
	
	if objLockout.lowpart = 0 And objLockout.highpart = 0 Then
		Get_AccountLocked = 0
	Else
		Get_AccountLocked = 1
	End If
End Function

'----------------------------------------------------------------------

Function Get_PasswordExpired(objUser)

  Const E_ADS_PROPERTY_NOT_FOUND = &h8000500D
  Const ONE_HUNDRED_NANOSECOND = .000000100
  Const SECONDS_IN_DAY = 86400

    bPasswordExpired = 0

    Set objRootDSE = GetObject("LDAP://rootDSE")
    Set objDomain = GetObject("LDAP://" & objRootDse.Get("defaultNamingContext") )

    Set objMaxPwdAge = objDomain.Get("maxPwdAge")
    
    flag = objUser.Get("userAccountControl")

    If Get_PasswordNeverExpires(objUser) Then
        bPasswordExpired = 0   
    ElseIf ((objUser.pwdLastSet).HighPart = 0) And ((objUser.pwdLastSet).LowPart = 0) Then  'password set to change on next logon
        bPasswordExpired = 1
    Else
        dtmValue = objUser.PasswordLastChanged

        If Err.Number = E_ADS_PROPERTY_NOT_FOUND Then
            bPasswordExpired = 0                                 'password was not set
        Else
            iTimeInterval = Int(Now - dtmValue)
    
            If objMaxPwdAge.LowPart = 0 Then
                bPasswordExpired = 0                                'password does not expire
            Else
                dblMaxPwdNano = Abs(objMaxPwdAge.HighPart * 2^32 + objMaxPwdAge.LowPart)                
                dblMaxPwdSecs = dblMaxPwdNano * ONE_HUNDRED_NANOSECOND
                dblMaxPwdDays = Int(dblMaxPwdSecs / SECONDS_IN_DAY)

                If iTimeInterval >= dblMaxPwdDays Then
                    bPasswordExpired = 1
                End If
            End If
        End If
    End If
    
    Get_PasswordExpired =  bPasswordExpired
End Function

'----------------------------------------------------------------------

Function Get_AccountExpired(objUser)
    lngBias = GetTimeZoneBias()
    set objDate = objUser.accountExpires
    lngDate = Integer8Date(objDate, lngBias)
    tDiff = DateDiff("s", #1/1/1601#, lngDate)
    if(tDiff = 0 ) then
        bAccountExpired = 0
    else
        if  DateDiff("s", Now, lngDate) > 0 then               'expiry time is greater than current time
            bAccountExpired = 0
        else
            bAccountExpired = 1
        end if
    end if
    Get_AccountExpired = bAccountExpired
End Function
'----------------------------------------------------------------------

Function GetTimeZoneBias()
    ' Obtain local Time Zone bias from machine registry.
    Set objShell = CreateObject("Wscript.Shell")
    lngBiasKey = objShell.RegRead("HKLM\System\CurrentControlSet\Control\" _
        & "TimeZoneInformation\ActiveTimeBias")
    If (UCase(TypeName(lngBiasKey)) = "LONG") Then
        lngBias = lngBiasKey
    ElseIf (UCase(TypeName(lngBiasKey)) = "VARIANT()") Then
        lngBias = 0
        For k = 0 To UBound(lngBiasKey)
            lngBias = lngBias + (lngBiasKey(k) * 256^k)
        Next
    End If
End Function

'----------------------------------------------------------------------

' From Richard Mueller 
Function Integer8Date(ByVal objDate, ByVal lngBias)
    ' Function to convert Integer8 (64-bit) value to a date, adjusted for
    ' local time zone bias.
    Dim lngAdjust, lngDate, lngHigh, lngLow
    lngAdjust = lngBias
    lngHigh = objDate.HighPart
    lngLow = objdate.LowPart
    ' Account for error in IADsLargeInteger property methods.
    If (lngLow < 0) Then
        lngHigh = lngHigh + 1
    End If
    If (lngHigh = 0) And (lngLow = 0) Then
        lngAdjust = 0
    End If
    lngDate = #1/1/1601# + (((lngHigh * (2 ^ 32)) _
        + lngLow) / 600000000 - lngAdjust) / 1440
    ' Trap error if lngDate is ridiculously huge.
    On Error Resume Next
    Integer8Date = CDate(lngDate)
    If (Err.Number <> 0) Then
        On Error GoTo 0
        Integer8Date = #1/1/1601#
    End If
    On Error GoTo 0
End Function

'----------------------------------------------------------------------


Function Get_AccountEnabled(objUser)
    Get_AccountEnabled = 1

    flag = objUser.userAccountControl

    If ( flag And ADS_UF_ACCOUNTDISABLE ) then
      Get_AccountEnabled = 0
    End If

End Function

Function Get_CanChangePassword(oUser)

    Get_CanChangePassword = 1

    flag = oUser.userAccountControl

    If (flag And ADS_UF_PASSWD_CANT_CHANGE) then
      Get_CanChangePassword = 0
    End If

End Function


Function Get_PasswordNeverExpires(oUser)

    Get_PasswordNeverExpires = 0

    flag = oUser.userAccountControl

    If (flag And ADS_UF_DONT_EXPIRE_PASSWD) then
      Get_PasswordNeverExpires = 1
    End If

End Function

Sub Set_PasswordNeverExpires(oUser, newValue)

    flag = oUser.Get("userAccountControl")
    if ( newValue ) then
	newFlag = flag Or ADS_UF_DONT_EXPIRE_PASSWD
    else
        newFlag = flag And Not(ADS_UF_DONT_EXPIRE_PASSWD)
    end if
    oUser.Put "userAccountControl", newFlag

End Sub



Function Get_PasswordExpirationDate(oUser)

  set objDate = oUser.accountExpires

  Get_PasswordExpirationDate = Get_UInt8ToHexString(objDate)

End Function


Function Get_PasswordLastSet(oUser)

    Set objDate = oUser.Get("pwdLastSet")

    Get_PasswordLastSet = Get_UInt8ToHexString(objDate)

End Function

'----------------------------------------------------------------------
Function Get_UInt8ToHexString(objInt8)

  Get_UInt8ToHexString = Get_UInt4ToHexString(objInt8.HighPart) & Get_UInt4TohexString(objInt8.LowPart)

End Function

'----------------------------------------------------------------------
Function Get_UInt4ToHexString(Int4)

  answer = ""

  if ( Int4 < 0 ) Then
    Value = 1
  else
    Value = 0
  End If

  for i = 30 to 0 step - 1

    If ( i = 27 Or i = 23 Or i = 19 Or i = 15 Or i = 11 Or i = 7 Or i = 3  ) then
      answer = answer & Hex(value)
      value = 0
    End If

    Value = 2 * Value

    If ( Not((Int4 And (2^i)) = 0) ) then
      Value = Value + 1
    End If
  next

  answer = answer & Hex(value)

  Get_UInt4ToHexString = answer

End Function




'----------------------------------------------------------------------

Function Get_NetBIOSName(objRootDSE, strFQDN)
 ' Taken from cookbook

  strADsPath = "<LDAP://" & strFQDN & "/cn=Partitions," & objRootDSE.Get("configurationNamingContext") & ">;"
  strFilter = "(&(objectcategory=Crossref)" & "(dnsRoot=" & strFQDN & ")(netBIOSName=*));"
  strAttrs = "netbiosname;"
  strScope = "Onelevel"


  set objConn = CreateObject("ADODB.Connection")
  objConn.Provider = "ADsDSOObject"
  objConn.Open "Active Directory Provider"
  set objRS = objConn.Execute(strADsPath & strFilter & strAttrs & strScope)
  objRS.MoveFirst
  set objConn = Nothing


  Get_NetBIOSName = objRS.Fields(0).Value

End Function

 
'-------------------------------------------------------------------

Function Get_NTName(strDomain, strName)

  strNtName = strDomain & "\" & strName

  Get_NTName = strNtName
End Function
'-------------------------------------------------------------------

Function Get_UserInOU(strName, strOU)
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell,OU=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  On Error Resume Next
  Set objUser = GetObject(strADsPath)

  if (isobject(objUser)) then
    Get_UserInOU = 1
  else
    Get_UserInOU = 0
  end if

End Function
'-------------------------------------------------------------------

Function Get_GroupInOU(strName, strOU)
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=groups,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=groups,CN=$LikewiseIdentityCell,OU=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  On Error Resume Next
  Set objUser = GetObject(strADsPath)

  if (isobject(objUser)) then
    Get_GroupInOU = 1
  else
    Get_GroupInOU = 0
  end if

End Function
'-------------------------------------------------------------------

Function Get_UID(strName, strOU)
 
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell,ou=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  Set objDomain = GetObject(strADsPath)
  objKeywordContainer = objDomain.Get("keywords")

  For Each objKeyword In objKeyWordContainer
    uid = InStr(1,objKeyword,"uidNumber",1)
    if (uid <> 0) Then
      Get_UID = Mid(objKeyword, uid+10)	  
      Exit For
    End if
  Next

End Function
    
'-------------------------------------------------------------------
Function Get_GGID(strName, strOU)
 
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=Groups,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=Groups,CN=$LikewiseIdentityCell,ou=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  Set objDomain = GetObject(strADsPath)
  objKeywordContainer = objDomain.Get("keywords")

  For Each objKeyword In objKeyWordContainer
    gid = InStr(1,objKeyword,"gidNumber",1)
    if (gid <> 0) Then
      Get_GGID = Mid(objKeyword, gid+10)	  
      Exit For
    End if
  Next
    
End Function
'-------------------------------------------------------------------

Function Get_GID(strName, strOU)
 
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell,ou=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  Set objDomain = GetObject(strADsPath)
  objKeywordContainer = objDomain.Get("keywords")

  For Each objKeyword In objKeyWordContainer
    gid = InStr(1,objKeyword,"gidNumber",1)
    if (gid <> 0) Then
      Get_GID = Mid(objKeyword, gid+10)	  
      Exit For
    End if
  Next
    
End Function
'-------------------------------------------------------------------


Function Get_Gecos(strName, strOU)
 
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell,ou=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  Set objDomain = GetObject(strADsPath)
  objKeywordContainer = objDomain.Get("keywords")

  For Each objKeyword In objKeyWordContainer
    gecos = InStr(1,objKeyword,"gecos",1)
    if (gecos <> 0) Then
      Get_Gecos = Mid(objKeyword, gecos+6)	  
    Exit For
    End if
  Next

End Function
'-------------------------------------------------------------------
    
Function Get_LoginShell(strName, strOU)
 
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell,ou=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  Set objDomain = GetObject(strADsPath)
  objKeywordContainer = objDomain.Get("keywords")

  For Each objKeyword In objKeyWordContainer
    loginshell = InStr(1,objKeyword,"loginShell",1)
    if (loginshell <> 0) Then
      Get_LoginShell = Mid(objKeyword, loginshell+11)	  
      Exit For
    End if
  Next

End Function
'-------------------------------------------------------------------
    
Function Get_UnixHomeDir(strName, strOU)
 
  Set objRootDSE = GetObject("LDAP://rootDSE")
  if (strOU = "") then
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell," & objRootDSE.Get("defaultNamingContext")
  else
    strADsPath = "LDAP://CN=" & strName & ",CN=Users,CN=$LikewiseIdentityCell,ou=" & strOU & "," & objRootDSE.Get("defaultNamingContext")
  end if

  Set objDomain = GetObject(strADsPath)
  objKeywordContainer = objDomain.Get("keywords")

  For Each objKeyword In objKeyWordContainer
    homedir = InStr(1,objKeyword,"unixHomeDirectory",1)
    if (homedir <> 0) Then
      Get_UnixHomedir = Mid(objKeyword, homedir+18)	  
      Exit For
    End if
  Next

End Function
'-------------------------------------------------------------------


















' From Richard Mueller 
Function ObjSidToStrSid(arrSid)
' Function to convert OctetString (byte array) to Decimal string (SDDL) Sid.
Dim strHex, strDec

strHex = OctetStrToHexStr(arrSid)
strDec = HexStrToDecStr(strHex)
ObjSidToStrSid = strDec
End Function 'ObjSidToStrSid

'-------------------------------------------------------------------

Function OctetStrToHexStr(arrbytOctet)
' Function to convert OctetString (byte array) to Hex string.
Dim k

OctetStrToHexStr = ""
For k = 1 To Lenb(arrbytOctet)
 OctetStrToHexStr = OctetStrToHexStr & Right("0" & Hex(Ascb(Midb(arrbytOctet, k, 1))), 2)
Next
End Function ' OctetStrToHexStr

'-------------------------------------------------------------------

Function HexStrToDecStr(strSid)
' Function to convert Hex string Sid to Decimal string (SDDL) Sid.

' SID anatomy:
' Byte Position
' 0 : SID Structure Revision Level (SRL)
' 1 : Number of Subauthority/Relative Identifier
' 2-7 : Identifier Authority Value (IAV) [48 bits]
' 8-x : Variable number of Subauthority or Relative Identifier (RID) [32 bits]
'
' Example: '
' <Domain/Machine>\Administrator
' Pos : 0 | 1 | 2 3 4 5 6 7 | 8 9 10 11 | 12 13 14 15 | 16 17 18 19 | 20 21 22 23 | 24 25 26 27
' Value: 01 | 05 | 00 00 00 00 00 05 | 15 00 00 00 | 06 4E 7D 7F | 11 57 56 7A | 04 11 C5 20 | F4 01 00 00
' str : S- 1 | | -5 | -21 | -2138918406 | -2052478737 | -549785860 | -500

Const BYTES_IN_32BITS = 4
Const SRL_BYTE = 0
Const IAV_START_BYTE = 2
Const IAV_END_BYTE = 7
Const RID_START_BYTE = 8
Const MSB = 3 'Most significant byte
Const LSB = 0 'Least significant byte

Dim arrbytSid, lngTemp, base, offset, i

ReDim arrbytSid(Len(strSid)/2 - 1)

' Convert hex string into integer array
For i = 0 To UBound(arrbytSid)
arrbytSid(i) = CInt("&H" & Mid(strSid, 2 * i + 1, 2))
Next

' Add SRL number
HexStrToDecStr = "S-" & arrbytSid(SRL_BYTE)

' Add Identifier Authority Value
lngTemp = 0
For i = IAV_START_BYTE To IAV_END_BYTE
lngTemp = lngTemp * 256 + arrbytSid(i)
Next
HexStrToDecStr = HexStrToDecStr & "-" & CStr(lngTemp)

' Add a variable number of 32-bit subauthority or
' relative identifier (RID) values.
' Bytes are in reverse significant order.
' i.e. HEX 01 02 03 04 => HEX 04 03 02 01
' = (((0 * 256 + 04) * 256 + 03) * 256 + 02) * 256 + 01
' = DEC 67305985
For base = RID_START_BYTE To UBound(arrbytSid) Step BYTES_IN_32BITS
lngTemp = 0
For offset = MSB to LSB Step -1
lngTemp = lngTemp * 256 + arrbytSid(base + offset)
Next
HexStrToDecStr = HexStrToDecStr & "-" & CStr(lngTemp)
Next
End Function ' HexStrToDecStr


