'Script to parse current-version.xml, extract version.xml and add NSIS defines

If WScript.Arguments.Count = 0 Then
    WScript.Echo("Usage:" & vbCrLf & "parseVersions.vbs <platform>")
    WScript.Quit
End If

Set xmlDoc = CreateObject("Msxml2.DOMDocument") 
xmlDoc.async = False
If xmlDoc.load("current-version.xml") Then
    Set Root = xmlDoc.documentElement 
    ' Browse through all <Release> elements
    Set ReleaseNodes = Root.getElementsByTagName("Release")
    For Each ReleaseNode In ReleaseNodes
        ' Check the OS value
        Set Release = ReleaseNode.getElementsByTagName("OS")
        If Release(0).Text = WScript.Arguments(0) Then
            ' Found required release - output version.xml and NSIS file
            ParseReleaseData(ReleaseNode)
            WScript.Quit
        End If
    Next
    
    WScript.Echo("Release " & WScript.Arguments(0) & "not found.")
Else
    WScript.Echo("Error loading current-version.xml")
    WScript.Quit
End If

' Parse Release element
Sub ParseReleaseData(ReleaseNode)
    Set fso = CreateObject("Scripting.FileSystemObject")
    ' Write version.xml
    Set fsoFile = fso.CreateTextFile("version.xml", True)
    fsoFile.WriteLine("<?xml version=""1.0"" encoding=""utf-8""?>")
    fsoFile.WriteLine("<Releases>")
    fsoFile.WriteLine(ReleaseNode.xml)
    fsoFile.WriteLine("</Releases>")
    fsoFile.Close

    ' Write NSIS include file
    Set ReleaseVersion = ReleaseNode.getElementsByTagName("Version")
    Set ReleaseDate = ReleaseNode.getElementsByTagName("Date")
    Set ReleaseNotes = ReleaseNode.getElementsByTagName("Notes")
    Set fsoFile = fso.CreateTextFile("AnalogExif.nsh", True)
    fsoFile.WriteLine("; AnalogExif version information")
    fsoFile.WriteLine("!define PRODUCT_VERSION """ & ReleaseVersion(0).Text & """")
    fsoFile.WriteLine("!define PRODUCT_DATE """ & ReleaseDate(0).Text & """")
    
    Dim RegEx
    Set RegEx = New RegExp
    RegEx.Multiline = True
    RegEx.Global = True
    
    RegEx.Pattern = "\n"
    ReleaseNotesText = RegEx.Replace(ReleaseNotes(0).Text,"$\r$\n")

    RegEx.Pattern = "\s{2,}"    
    ReleaseNotesText = RegEx.Replace(ReleaseNotesText,"")

    fsoFile.WriteLine("!define PRODUCT_NOTES """ & ReleaseNotesText & """")
    fsoFile.Close
    
    ' Modify VERSIONINFO
    Set fsoFile = fso.CreateTextFile("AnalogExif.rc.tmp", True)
    Set sourceRcFile = fso.OpenTextFile("AnalogExif.rc", 1)
   
    Do While Not sourceRcFile.AtEndofStream
        myLine = sourceRcFile.ReadLine
        If InStr(myLine, "VALUE ""FileVersion""") Then
            myLine = "            VALUE ""FileVersion"", """ & ReleaseVersion(0).Text & """"
        ElseIf InStr(myLine, "VALUE ""ProductVersion""") Then
            myLine = "            VALUE ""ProductVersion"", """ & ReleaseVersion(0).Text & """"
        End If

        fsoFile.WriteLine(myLine)
    Loop

    fsoFile.Close
    sourceRcFile.Close
    fso.DeleteFile("AnalogExif.rc")
    fso.MoveFile "AnalogExif.rc.tmp", "AnalogExif.rc"
End Sub