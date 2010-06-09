;NSIS Modern User Interface
;Welcome/Finish Page Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

;--------------------------------
;General

  Var GIMP_InstDir
  Var GIMP_NotFound
  Var DirectoryPageText
  Var FileList

  !define P_NAME "Gimp Liquid Rescale Plug-In"
  !define P_VERSION 0.7.1
  !define LIB_VERSION 0.4.1

  !define INPUT_DIR "gimp-lqr-plugin_${P_VERSION}-liblqr_${LIB_VERSION}_win32"
  !define SHARE_DIR "share\gimp-lqr-plugin"
  !define INPUT_SHARE_DIR "${INPUT_DIR}\${SHARE_DIR}"
  !define INPUT_FILE_LIST "file_list.log"
  !define INPUT_ICON "LqR_icon.ico"

  !include "${INPUT_DIR}\lqr_file_list.nsh"

  LangString UninstLogMissing ${LANG_ENGLISH} "${INPUT_FILE_LIST} not found!$\r$\nUninstallation cannot proceed!"


  ;Windows uninstall key
  !define REG_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\GimpLqRPlugIn"

  !define InstFile "gimp-lqr-plugin_${P_VERSION}-liblqr_${LIB_VERSION}_win32_setup.exe"

  ;Name and file
  Name "GIMP LqR Plug-In"
  OutFile "${InstFile}"

  ;Default installation folder
  ;InstallDir "$DESKTOP\LqR-plugin" ; temporary, will be set by .onInit

  ;Installer Icon
  !define MUI_ICON "${INPUT_SHARE_DIR}\${INPUT_ICON}"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

  ;Set compression
  SetCompressor /SOLID lzma

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  ;!define MUI_UNFINISHPAGE_NOAUTOCLOSE

  !define MUI_CUSTOM_DIRECTORYPAGE
  ; NOTE: this required a change in "${NSIS}\Contrib\Language Files\English.nsh"
  ; from this:
  ;
  ; !ifdef MUI_DIRECTORYPAGE
  ;   ${LangFileString} MUI_TEXT_DIRECTORY_TITLE "Choose Install Location"
  ;   ${LangFileString} MUI_TEXT_DIRECTORY_SUBTITLE "Choose the folder in which to install $(^NameDA)."
  ; !endif
  ; 
  ; to this:
  ;
  ; !ifdef MUI_CUSTOM_DIRECTORYPAGE
  ;   ${LangFileString} MUI_TEXT_DIRECTORY_TITLE "${MUI_DIRECTORYPAGE_TITLE}"
  ;   ${LangFileString} MUI_TEXT_DIRECTORY_SUBTITLE "${MUI_DIRECTORYPAGE_SUBTITLE}"
  ; !else
  ;   !ifdef MUI_DIRECTORYPAGE
  ;     ${LangFileString} MUI_TEXT_DIRECTORY_TITLE "Choose Install Location"
  ;     ${LangFileString} MUI_TEXT_DIRECTORY_SUBTITLE "Choose the folder in which to install $(^NameDA)."
  ;   !endif
  ; !endif
  ;

;--------------------------------
;Pages

  !define MUI_WELCOMEPAGE_TEXT \
"This wizard will guide you through the installation of GIMP \
Liquid Rescale Plug-In, version ${P_VERSION}.$\n\
$\n\
You are strongly encouraged to uninstall any previous installation of the plugin before continuing.$\n\
$\n\
The GIMP Liquid Rescale plug-in aims at resizing pictures non uniformly \
while preserving their features, i.e. avoiding distortion of the important parts.\
The plugin supports manual feature selection, and can also be used to remove \
portions of the picture in a consistent way.$\n\
Since version 0.6 it also supports real-time interactive scaling.$\n\
$\n\
Click Next to continue."

  !define MUI_DIRECTORYPAGE_TITLE "Choose GIMP Install Location" ; NOTE: customized, see above
  !define MUI_DIRECTORYPAGE_SUBTITLE "Choose the folder in which GIMP is installed." ; NOTE: customized, see above

  !define MUI_DIRECTORYPAGE_TEXT $DirectoryPageText

  !define MUI_DIRECTORYPAGE_SUBTEXT "Selected GIMP installation Folder"
  !define MUI_DIRECTORYPAGE_BROWSETEXT "Select the GIMP installation folder to install $(^NameDA) in:"

  DirText "${MUI_DIRECTORYPAGE_TEXT}" "${MUI_DIRECTORYPAGE_SUBTEXT}" "" "${MUI_DIRECTORYPAGE_BROWSETEXT}"


  !define MUI_FINISHPAGE_TEXT \
"GIMP Liquid Rescale Plug-In has been installed on your system.$\n\
To use the plug-in, open an image with GIMP and select 'Liquid Rescale...' from the \
'Layer' menu.$\n\
$\n\
You need to restart GIMP if it's already running.$\n\
$\n\
Click Finish to close this wizard."

  !define MUI_FINISHPAGE_LINK "GIMP LqR Plug-In homepage"
  !define MUI_FINISHPAGE_LINK_LOCATION "http:\\liquidrescale.wikidot.com"

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "License.txt"
  ;!insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  ;!insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "Base" Base
SectionIn RO
  ;This section is required. It can't be removed.

  CreateDirectory "$INSTDIR"

  !insertmacro InstallFiles

  SetOutPath "$INSTDIR\${SHARE_DIR}"
  File "${INPUT_SHARE_DIR}\${INPUT_FILE_LIST}"

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\${SHARE_DIR}\Uninstall.exe"

  ;Writing uninstall info to registry:
  WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayName" "GIMP LqR Plug-In"
  WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayIcon" "$INSTDIR\${SHARE_DIR}\${INPUT_ICON}"
  WriteRegStr HKLM "${REG_UNINSTALL}" "DisplayVersion" "PlugIn: ${P_VERSION} - Lib: ${LIB_VERSION}"
  WriteRegStr HKLM "${REG_UNINSTALL}" "Publisher" "Carlo Baldassi"
  WriteRegStr HKLM "${REG_UNINSTALL}" "URLInfoAbout" "http://liquidrescale.wikidot.com"
  WriteRegStr HKLM "${REG_UNINSTALL}" "InstallLocation" "$INSTDIR"
 
  WriteRegDWord HKLM "${REG_UNINSTALL}" "NoModify" 1
  WriteRegDWord HKLM "${REG_UNINSTALL}" "NoRepair" 1
  WriteRegStr HKLM "${REG_UNINSTALL}" "UninstallString" '"$INSTDIR\${SHARE_DIR}\Uninstall.exe"'
  WriteRegStr HKLM "${REG_UNINSTALL}" "QuietUninstallString" '"$INSTDIR\${SHARE_DIR}\Uninstall.exe" /S'

SectionEnd

Function .onInit
  StrCpy $GIMP_NotFound "false"
  ReadRegStr $GIMP_InstDir HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinGimp-2.0_is1" "InstallLocation"

  ; Check for GIMP installation existence
  StrCmp $GIMP_InstDir "" 0 GIMPDirFound

    ReadRegStr $GIMP_InstDir HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinGimp-2.0_is1" "InstallLocation"

    StrCmp $GIMP_InstDir "" 0 GIMPDirFound

      StrCpy $GIMP_NotFound "true"
      StrCpy $GIMP_InstDir "$PROGRAMFILES\GIMP-2.0"
      MessageBox MB_ICONEXCLAMATION \
"GIMP installation was not found. \
You must install GIMP before runing this installer. \
If you have GIMP installed, you may proceed, but you need to \
specify the GIMP installtion folder manually."
      ; MessageBox MB_ICONEXCLAMATION "GIMP installation was not found. Please install GIMP before runing this installer."
      ; Abort ; abort if APP installation is not found
  GIMPDirFound:

  StrCpy $INSTDIR "$GIMP_InstDir"

  Call SetDirPageText

FunctionEnd

Function .onVerifyInstDir
  IfFileExists $INSTDIR\bin\gimp-*.exe PathGood
    Abort ; if $INSTDIR is not a GIMP directory, don't let us install there
  PathGood:
FunctionEnd

;Section -Post
  ;Showing the results
  ;ExecShell "open" "$INSTDIR"
;SectionEnd


;--------------------------------
;Uninstaller Section


Section -closelogfile
 SetFileAttributes "$INSTDIR\${SHARE_DIR}\${INPUT_FILE_LIST}" READONLY|SYSTEM|HIDDEN
SectionEnd

Section "Uninstall"

  DetailPrint "*** Opening uninstall file..."

  ReadRegStr $INSTDIR HKLM "${REG_UNINSTALL}" "InstallLocation" 

  ; Can't uninstall if uninstall log is missing!
  IfFileExists "$INSTDIR\${SHARE_DIR}\${INPUT_FILE_LIST}" +3
   MessageBox MB_OK|MB_ICONSTOP "$(UninstLogMissing)"
    Abort
  
  Push $R0
  Push $R1
  SetFileAttributes "$INSTDIR\${SHARE_DIR}\${INPUT_FILE_LIST}" NORMAL
  FileOpen $FileList "$INSTDIR\${SHARE_DIR}\${INPUT_FILE_LIST}" r
  
  DetailPrint "*** Perform uninstall..."
  StrCpy $R1 -1
  
  GetLineCount:
   ClearErrors
   FileRead $FileList $R0
   IntOp $R1 $R1 + 1
   StrCpy $R0 $R0 -2
   Push $R0   
   IfErrors 0 GetLineCount
  
  Pop $R0
  
  LoopRead:
   StrCmp $R1 0 LoopDone
   Pop $R0
  
   IfFileExists "$INSTDIR\$R0\*.*" 0 +3
    RMDir "$INSTDIR\$R0"  #is dir
   Goto +3
   IfFileExists "$INSTDIR\$R0" 0 +2
    Delete "$INSTDIR\$R0" #is file
  
   IntOp $R1 $R1 - 1
   Goto LoopRead

  LoopDone:

  FileClose $FileList
  Pop $R1
  Pop $R0

  DetailPrint "*** Delete uninstaller..."
  Delete "$INSTDIR\${SHARE_DIR}\${INPUT_FILE_LIST}"
  Delete "$INSTDIR\${SHARE_DIR}\Uninstall.exe"

  DetailPrint "*** Removing installation directory..."
  RMDir "$INSTDIR\${SHARE_DIR}"

  DetailPrint "*** Cleaning up Windows register..."
  DeleteRegKey HKLM "${REG_UNINSTALL}"


SectionEnd

Function SetDirPageText

  StrCmp $GIMP_NotFound "true" LabelGIMPNotFound LabelGIMPFound

    LabelGIMPNotFound:
    StrCpy $DirectoryPageText \
"Setup could not find a GIMP installation. \
If you have GIMP installed on your system, click Browse and select \
its installation folder, otherwise press Cancel, install GIMP and start \
this installer again."
    Return

    LabelGIMPFound:
    StrCpy $DirectoryPageText \
"Setup found a GIMP installation at the specified folder. \
If you want to choose a different GIMP installation folder \
under which to install $(^NameDA), click Browse and select another folder."
    Return
  
FunctionEnd
