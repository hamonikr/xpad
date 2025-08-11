; NSIS installer script for Xpad (Windows)

!include "MUI2.nsh"

; The following are provided via makensis -DPRODUCT_NAME=... etc.
; !define PRODUCT_NAME      "Xpad"
; !define PRODUCT_VERSION   "0.0.0"
; !define COMPANY_NAME      "Xpad Project"
; !define INSTALL_SRC       "C:\\path\\to\\staged"
; !define OUT_DIR           "C:\\path\\to\\out"

!define MUI_ABORTWARNING

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${OUT_DIR}\\${PRODUCT_NAME}-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$PROGRAMFILES\\${PRODUCT_NAME}"
RequestExecutionLevel user

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${INSTALL_SRC}\\*.*"

  ; Add Start Menu shortcut
  CreateDirectory "$SMPROGRAMS\\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\\${PRODUCT_NAME}\\${PRODUCT_NAME}.lnk" "$INSTDIR\\xpad.exe"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\\${PRODUCT_NAME}\\${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\\${PRODUCT_NAME}"
  RMDir /r "$INSTDIR"
SectionEnd


