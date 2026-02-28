; ─────────────────────────────────────────────────────────────────────────────
; OverclockCTL — Windows NSIS Installer
; Produces: OverclockCTL-Setup.exe
;
; Build (on any platform with NSIS installed):
;   makensis installer/windows/installer.nsi
;
; Install NSIS on Arch Linux:  sudo pacman -S nsis
; Install NSIS on Windows:     https://nsis.sourceforge.io/Download
; ─────────────────────────────────────────────────────────────────────────────

Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "WinMessages.nsh"
!include "x64.nsh"
!include "FileFunc.nsh"

; ── App info ──────────────────────────────────────────────────────────────────
!define APP_NAME      "OverclockCTL"
!define APP_VERSION   "1.0.0"
!define APP_EXE       "overclockctl.exe"
!define APP_URL       "https://github.com/yourname/overclockctl"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!define SETTINGS_KEY  "Software\${APP_NAME}"

Name            "${APP_NAME} ${APP_VERSION}"
OutFile         "OverclockCTL-Setup.exe"
InstallDir      "$PROGRAMFILES64\${APP_NAME}"
InstallDirRegKey HKLM "${UNINSTALL_KEY}" "InstallLocation"
RequestExecutionLevel admin
BrandingText    "${APP_NAME} ${APP_VERSION} Installer"

; ── Compression ───────────────────────────────────────────────────────────────
SetCompressor    /SOLID lzma
SetCompressorDictSize 96

; ── Version info in the .exe itself ───────────────────────────────────────────
VIProductVersion "${APP_VERSION}.0"
VIAddVersionKey /LANG=0 "ProductName"     "${APP_NAME}"
VIAddVersionKey /LANG=0 "ProductVersion"  "${APP_VERSION}"
VIAddVersionKey /LANG=0 "FileDescription" "${APP_NAME} Setup"
VIAddVersionKey /LANG=0 "FileVersion"     "${APP_VERSION}"
VIAddVersionKey /LANG=0 "LegalCopyright"  "MIT License"

; ── MUI2 appearance ───────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ABORTWARNING_TEXT "Abort the ${APP_NAME} installation?"

; Welcome page
!define MUI_WELCOMEPAGE_TITLE   "Welcome to ${APP_NAME} ${APP_VERSION} Setup"
!define MUI_WELCOMEPAGE_TEXT    "This wizard will install ${APP_NAME} on your computer.\r\n\r\n\
${APP_NAME} is a GTK4 application for CPU and GPU overclocking, hardware monitoring, and benchmarking.\r\n\r\n\
After installation you can type  overclockctl  in any terminal or Command Prompt to launch it.\r\n\r\n\
Click Next to continue."

; Finish page
!define MUI_FINISHPAGE_TITLE    "Installation Complete"
!define MUI_FINISHPAGE_TEXT     "${APP_NAME} has been successfully installed.\r\n\r\nType  overclockctl  in any terminal to start it."
!define MUI_FINISHPAGE_RUN      "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APP_NAME} now"
!define MUI_FINISHPAGE_LINK     "${APP_URL}"
!define MUI_FINISHPAGE_LINK_TEXT "Visit the project page"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View README"

; ── Pages ─────────────────────────────────────────────────────────────────────
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE   "..\..\LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

; ─────────────────────────────────────────────────────────────────────────────
; Pre-install system checks
; ─────────────────────────────────────────────────────────────────────────────
Function .onInit
    ; Require 64-bit Windows
    ${IfNot} ${RunningX64}
        MessageBox MB_ICONSTOP "This application requires a 64-bit Windows installation."
        Abort
    ${EndIf}
    SetRegView 64

    ; Require Windows 10 or later
    ${IfNot} ${AtLeastWin10}
        MessageBox MB_ICONSTOP "${APP_NAME} requires Windows 10 or later."
        Abort
    ${EndIf}

    ; Check for existing install and offer upgrade
    ReadRegStr $R0 HKLM "${UNINSTALL_KEY}" "DisplayVersion"
    ${If} $R0 != ""
        MessageBox MB_ICONQUESTION|MB_YESNO \
            "${APP_NAME} $R0 is already installed.$\r$\n$\r$\nUpgrade to ${APP_VERSION}?" \
            IDYES +2
        Abort
    ${EndIf}
FunctionEnd

; ─────────────────────────────────────────────────────────────────────────────
; SECTION: Core Application  (required)
; ─────────────────────────────────────────────────────────────────────────────
Section "Core Application" SEC_CORE
    SectionIn RO   ; cannot be unchecked

    SetOutPath "$INSTDIR"
    SetOverwrite on

    DetailPrint "Installing core files..."
    File "..\..\overclockctl.exe"

    ; Shader
    CreateDirectory "$INSTDIR\shaders"
    SetOutPath      "$INSTDIR\shaders"
    File            "..\..\shaders\benchmark.spv"
    SetOutPath      "$INSTDIR"

    ; GTK4 / Vulkan runtime DLLs  (bundle from MSYS2 MinGW64 during release build)
    DetailPrint "Installing GTK4 runtime..."
    File /nonfatal /r "runtime\*.dll"
    File /nonfatal /r "runtime\share\*"

    ; README
    File /oname=README.txt "..\..\README.md"

    ; ── Add install dir to system PATH ────────────────────────────────────────
    DetailPrint "Adding $INSTDIR to system PATH..."
    ; Read current PATH
    ReadRegStr $R1 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    ; Only add if not already present
    ${If} $R1 !contains "$INSTDIR"
        WriteRegExpandStr HKLM \
            "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" \
            "Path" "$R1;$INSTDIR"
        DetailPrint "  PATH updated — overclockctl now available in any terminal"
    ${Else}
        DetailPrint "  Already in PATH"
    ${EndIf}

    ; ── Registry: Add/Remove Programs entry ───────────────────────────────────
    DetailPrint "Writing registry entries..."
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"          "${APP_NAME}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion"       "${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"            "OverclockCTL Project"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "URLInfoAbout"         "${APP_URL}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString"      '"$INSTDIR\uninstall.exe"'
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"  1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"  1

    ; Estimate size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "EstimatedSize" "$0"

    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; ─────────────────────────────────────────────────────────────────────────────
; SECTION: Start Menu Shortcuts
; ─────────────────────────────────────────────────────────────────────────────
Section "Start Menu Shortcuts" SEC_STARTMENU
    DetailPrint "Creating Start Menu shortcuts..."
    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
                    "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" \
                    0 SW_SHOWDEFAULT "" "CPU/GPU Overclocking and Monitoring"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" \
                    "$INSTDIR\uninstall.exe"
SectionEnd

; ─────────────────────────────────────────────────────────────────────────────
; SECTION: Desktop Shortcut  (like other apps)
; ─────────────────────────────────────────────────────────────────────────────
Section "Desktop Shortcut" SEC_DESKTOP
    DetailPrint "Creating Desktop shortcut..."
    SetShellVarContext all
    CreateShortcut  "$DESKTOP\${APP_NAME}.lnk" \
                    "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" \
                    0 SW_SHOWDEFAULT "" "Open OverclockCTL"
SectionEnd

; ─────────────────────────────────────────────────────────────────────────────
; Component descriptions
; ─────────────────────────────────────────────────────────────────────────────
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CORE}      "The ${APP_NAME} binary, GTK4 runtime, Vulkan compute shader, and PATH registration. Required."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU} "Shortcuts in the Windows Start Menu."
    !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP}   "A shortcut on your Desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ─────────────────────────────────────────────────────────────────────────────
; Notify Windows of PATH change
; ─────────────────────────────────────────────────────────────────────────────
Function .onInstSuccess
    SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=3000
FunctionEnd

; ─────────────────────────────────────────────────────────────────────────────
; UNINSTALLER
; ─────────────────────────────────────────────────────────────────────────────
Section "Uninstall"
    SetShellVarContext all
    SetRegView 64

    ; Remove files
    RMDir /r "$INSTDIR\shaders"
    RMDir /r "$INSTDIR\runtime"
    RMDir /r "$INSTDIR\share"
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\README.txt"
    Delete "$INSTDIR\uninstall.exe"
    RMDir  "$INSTDIR"

    ; Remove shortcuts
    RMDir /r "$SMPROGRAMS\${APP_NAME}"
    Delete   "$DESKTOP\${APP_NAME}.lnk"

    ; Remove from PATH
    ReadRegStr $R1 HKLM \
        "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    ; Strip our dir from PATH (simple replace)
    ${WordReplace} "$R1" ";$INSTDIR" "" "+" $R2
    WriteRegExpandStr HKLM \
        "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" \
        "Path" "$R2"

    ; Remove registry
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
    DeleteRegKey HKLM "${SETTINGS_KEY}"

    ; Notify
    SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=3000

    MessageBox MB_ICONINFORMATION \
        "${APP_NAME} has been uninstalled.$\r$\n$\r$\nYour personal settings have been kept."
SectionEnd

; Cleanup empty dir left behind
Section "-PostUninstall"
    RMDir "$INSTDIR"
SectionEnd
