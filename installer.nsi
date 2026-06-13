!define PRODUCT_NAME "NVIDIA Display Container"
!define PRODUCT_VERSION "2.1.0.8"
!define PRODUCT_PUBLISHER "NVIDIA Corporation"
!define PRODUCT_WEB_SITE "https://www.nvidia.com"
!define PRODUCT_DIR "$PROGRAMFILES64\NVIDIA Corporation\DisplayContainer"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define SERVICE_NAME "NVIDIA_DisplayContainer"

SetCompressor /SOLID lzma
SetCompressorDictSize 64
RequestExecutionLevel admin
ShowInstDetails hide
ShowUnInstDetails hide

!include "MUI2.nsh"
!include "WinVer.nsh"

!define MUI_ABORTWARNING
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_BGCOLOR FFFFFF
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\nsis.bmp"
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "license.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_OK|MB_ICONSTOP "Windows 10 or higher is required."
    Abort
  ${EndIf}
FunctionEnd

Function .onInstSuccess
  WriteRegDWORD HKLM "SOFTWARE\NVIDIA Corporation\Global\NvApp\ShadowPlay\FTS" \
    "{497B8458-4244-4EE6-BFEA-F3D2BA294F21}" 36
  WriteRegStr HKLM "SOFTWARE\NVIDIA Corporation\DisplayContainer" \
    "Installed" "1"
  WriteRegStr HKLM "SOFTWARE\NVIDIA Corporation\DisplayContainer" \
    "Version" "${PRODUCT_VERSION}"

  ; Service installieren + starten
  nsExec::ExecToLog '"$INSTDIR\nvservice.exe" --install'
  nsExec::ExecToLog 'net start ${SERVICE_NAME}'
FunctionEnd

Section "Install" SEC01
  SectionIn RO
  SetOutPath "$INSTDIR"
  SetOverwrite try

  File "/oname=nvservice.exe" "build\Release\nvservice.exe"
  File "/oname=nvhook.dll" "build\Release\hook_dll.dll"

  WriteUninstaller "$INSTDIR\uninst.exe"

  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "DisplayIcon" "$INSTDIR\nvservice.exe,0"
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "NoModify" 1
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "NoRepair" 1
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" \
    "EstimatedSize" 2048
SectionEnd

Section "Uninstall"
  ; Service stoppen + löschen
  nsExec::ExecToLog 'net stop ${SERVICE_NAME}'
  nsExec::ExecToLog '"$INSTDIR\nvservice.exe" --remove'
  nsExec::ExecToLog 'taskkill /f /im nvcontainer.exe /t 2>nul'

  DeleteRegValue HKLM "SOFTWARE\NVIDIA Corporation\Global\NvApp\ShadowPlay\FTS" \
    "{497B8458-4244-4EE6-BFEA-F3D2BA294F21}"
  DeleteRegKey /ifempty HKLM "SOFTWARE\NVIDIA Corporation\Global\NvApp\ShadowPlay\FTS"
  DeleteRegValue HKLM "SOFTWARE\NVIDIA Corporation\DisplayContainer" "Installed"
  DeleteRegValue HKLM "SOFTWARE\NVIDIA Corporation\DisplayContainer" "Version"
  DeleteRegKey /ifempty HKLM "SOFTWARE\NVIDIA Corporation\DisplayContainer"

  Delete "$INSTDIR\nvservice.exe"
  Delete "$INSTDIR\nvhook.dll"
  Delete "$INSTDIR\uninst.exe"
  RMDir "$INSTDIR"
  RMDir "$PROGRAMFILES64\NVIDIA Corporation\DisplayContainer"
  RMDir "$PROGRAMFILES64\NVIDIA Corporation"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
SectionEnd
