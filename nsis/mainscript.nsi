!include "MUI.nsh"

OutFile ../barzer-installer.exe
Name "Barzer"
SetCompressor /SOLID lzma
InstallDir "$PROGRAMFILES\Barzer\Barzer"
!define MUI_ABORTWARNING
!define MUI_ICON app.ico
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall-colorful.ico"

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Barzer\Barzer"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "Barzer"

!define MUI_WELCOMEFINISHPAGE_BITMAP installscreen.bmp
!define MUI_HEADERIMAGE_BITMAP headerimage.bmp
!insertmacro MUI_PAGE_WELCOME
!define MUI_LICENSEPAGE_RADIOBUTTONS
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
Var STARTMENU_FOLDER
!insertmacro MUI_PAGE_STARTMENU Barzer $STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!define MUI_FINISHPAGE_RUN $INSTDIR\barzer.exe
!define MUI_FINISHPAGE_LINK "Web site"
!define MUI_FINISHPAGE_LINK_LOCATION http://barzer.net
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Russian"

SectionGroup "Barzer"
	Section "Barzer" MAINFILES
		SetOutPath $INSTDIR
		File app.ico
		File barzer.exe
		File libexpat.dll
		File libgcc_s_dw2-1.dll
		File libstdc++-6.dll
		File /r data

		WriteRegStr HKCU "Software\Barzer\Barzer" "" $INSTDIR
		WriteUninstaller "$INSTDIR\Uninstall.exe"
	
		!insertmacro MUI_STARTMENU_WRITE_BEGIN Barzer
			CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
			CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Barzer.lnk" "$INSTDIR\barzer.exe" "" "$INSTDIR\app.ico"
			CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
		!insertmacro MUI_STARTMENU_WRITE_END

		SectionIn 1 RO
	SectionEnd
SectionGroupEnd

Var MUI_TEMP

Section "Uninstall"
	RMDir /r "$INSTDIR"
		
	!insertmacro MUI_STARTMENU_GETFOLDER Barzer $MUI_TEMP
	Delete "$SMPROGRAMS\$MUI_TEMP\Barzer.lnk"
	Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
	
	StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
 
	startMenuDeleteLoop:
		ClearErrors
		RMDir $MUI_TEMP
		GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    
		IfErrors startMenuDeleteLoopDone
  
		StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
	startMenuDeleteLoopDone:
	DeleteRegKey /ifempty HKCU "Software\Barzer\Barzer"
SectionEnd

LangString DESC_MAINFILES ${LANG_ENGLISH} "Barzer setup."
LangString DESC_MAINFILES ${LANG_RUSSIAN} "Barzer."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${MAINFILES} $(DESC_MAINFILES)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function .onInit
	!insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd
