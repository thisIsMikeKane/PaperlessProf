#NoEnv ; Recommended for performance and compatibility with future AutoHotkey releases.
#SingleInstance force

#Include MDMF.ahk
#Include TaskbarMove.ahk
#Include NewChromeWin.ahk
#Include Lib\Webapp.ahk



__Webapp_AppStart:

; Move the task bar to the top
;	https://github.com/gerdami/AutoHotkey/blob/master/TaskbarMove.ahk`
WinExist("ahk_class Shell_TrayWnd") ; Bring task bar (on primary monitor) into focus
WinGetPos, TBx, TBy, TBw, TBh ; Get the position of the task bar
SysGet, pMon, Monitor ; Get the position of the screen
; Compare the position of the screen to the position of the taskbar
if (pMonBottom = TBy+TBh) {
	; The task bar will not be done moving when the WorkingArea positions are determined below,
	;		so shift everything down to account for the WorkingArea moving down by the height of the task bar
	TBShift := TBh
	
	; Move the task bar
	TaskbarMove("top")
} else { 
	;TODO what if the task bar isn't on the top or bottom?
	TBShift := 0
}

; Identify which monitor is the primary monitor. 
;		Assume only two monitors are connected, 
;			and the non-primary monitor is the public monitor.
;		Save the monitor info in the appropriate variable, with values
;			.Num .Name. Primary .Left .Top .Right .Bottom .Width .Height .AR
;			.WALeft .WATop .WARight .WABottom .WAWidth .WAHeight .WAAR
Monitors := MDMF_Enum()
PvtMonNum = 0;
For HMON, M in Monitors {
	if ( M.Primary ) {
		global PvtMon := M
		
	} else if(M.AR > 1) { ; The secondary display should be in landscape mode
		global PubMon := M
	}
}

; Get border information
;		https://msdn.microsoft.com/en-us/library/windows/desktop/bb688195%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
global bdrThkX
global bdrThkY
global capThk
SysGet, bdrThkX, 32 ;SM_CXFRAME 
SysGet, bdrThkY, 33 ;SM_CYFRAME
SysGet, capThk, 4 ;SM_CYCAPTION

; AutoHotkey Settings
SetTitleMatchMode, 2

; Determine the PUBLIC SPACE
; --------------------------
;		e.g. the space at the bottom of the primary screen that will get projected to the class
;		Full width of the private monitor, with the aspect ratio of the public monitor, 
;			bottom aligned with the private monitor
global PubSpaceWidth		:= PvtMon.Width
global PubSpaceHeight	:= Round(PvtMon.Width / PubMon.AR) 
global PubSpaceTop			:= (PvtMon.WABottom - PubSpaceHeight) + TBShift
global PubSpaceBottom	:= PvtMon.WABottom
global PubSpaceLeft		:= PvtMon.Left
global PubSpaceRight		:= PvtMon.Right
; Remove border thickness
global PubSpaceHeight	+= capThk + bdrThkY ;Hide the window caption from the public
global PubSpaceTop			-= capThk + bdrThkY

; Determine the PRIVATE SPACE 
; ---------------------------
;		e.g. the space at the top of the primary screen that only the teacher sees
;		The entire space of the private monitor, except the space occupied by the public space
global PvtSpaceWidth		:= PvtMon.Width
global PvtSpaceHeight	:= PvtMon.WAHeight - PubSpaceHeight
global PvtSpaceTop			:= PvtMon.WATop + TBShift
global PvtSpaceBottom	:= PvtMon.WATop + PvtSpaceHeight
global PvtSpaceLeft		:= PvtMon.Left
global PvtSpaceRight		:= PvtMon.Right
; Remove border thickness
global PvtSpaceHeight	+= bdrThkY
global PvtSpaceBottom	+= bdrThkY
; Not all windows need to remove the x-border thickness, so keep these separate
global PvtSpaceLeftWBdr  := PvtSpaceLeft-bdrThkX
global PvtSpaceWidthWBdr := PvtSpaceWidth+2*bdrThkX

; Move all current re-sizable windows to the 'private' space 
; ----------------------------------------------------------
WinGet, Window, List ; Loop through all windows
Loop %Window% {

	; Only move windows that can be resized
	Id:=Window%A_Index%
	WinGet, style, Style, % "ahk_id " Id
	if (style & 0x40000) { ; style & WS_SIZEBOX
	
		; Restore minimized windows
		WinRestore, % "ahk_id " Id 
		; Move first, then let DPI scaling take effect (automatic) if different monitors have different DPIs
		WinMove, % "ahk_id " Id,, PvtSpaceLeft-bdrThkX, PvtSpaceTop, 
		; Then resize once rescaled
		WinMove, % "ahk_id " Id,, PvtSpaceLeftbdrThkX, , PvtSpaceWidth+2*bdrThkX, PvtSpaceHeight 
	}
}

; Open public windows to display to the class
; -------------------------------------------
; * OneNote
Run OneNote.exe,,, pubOneNoteID
WinWait, OneNote ahk_pid %pubOneNoteID%,, 5
if ErrorLevel {
	MsgBox, OneNote failed to load quickly enough and was not moved
}
Sleep 300 ; Wait a little longer for good measure
WinRestore, ahk_pid %pubOneNoteID% ; Restore in case it was created maximized
WinMove, ahk_pid %pubOneNoteID%,, %PubSpaceLeft%, %PubSpaceTop%
WinMove, ahk_pid %pubOneNoteID%,, , , %PubSpaceWidth%, %PubSpaceHeight%

; * Chrome
;			Chrome behaves weird, so use NewChromeWin() to handle creation
; TODO this still doesn't always work right
pubChromeID := NewChromeWin(PubSpaceLeft - bdrThkX, PubSpaceTop, PubSpaceWidth + 2*bdrThkX, PubSpaceHeight)

; * DrawBoardPDF
WinMove, Drawboard PDF,, %PubSpaceLeft%, %PubSpaceTop%,
WinMove, Drawboard PDF,, , , %PubSpaceWidth%, %PubSpaceHeight%
;TODO figure out how to open drawboard
 
; Create the mirror on the public screen
; --------------------------------------
PubSpaceTopLocal := Round(PubSpaceTop - PvtMon.Top) + TBShift
PubMonNum := PubMon.Num - 1 ; VLC zero-indexes monitor numbers

global mirrorWin
Run C:\Program Files (x86)\VideoLAN\VLC\vlc.exe --fullscreen --qt-fullscreen-screennumber=%PubMonNum% --no-qt-fs-controller --video-filter=croppadd --croppadd-croptop=%PubSpaceTopLocal% --croppadd-cropbottom=0 --croppadd-cropleft=0 --croppadd-cropright=0 screen://,,, mirrorWin
 
; The GUI
; =============================================================================================
; Place GUI (only the top left corner)
WinGetPos, guiX, guiY, guiW, guiH, ahk_id %__Webapp_GuiHwnd%
newGuiY := PubSpaceTop - guiH - capThk
WinMove, ahk_id %__Webapp_GuiHwnd%,, %PubSpaceLeft%, %newGuiY%

;Get our HTML DOM object
iWebCtrl := getDOM()

;Change App name on run-time
setAppName("Display Controls")

; Our custom protocol's url event handler
app_call(args) {
	if InStr(args,"close") {
	
		WinClose, ahk_pid %mirrorWin%
		TaskbarMove("bottom")
		
	} else {
		MsgBox, app_call:%args%
	}
}

; function to run when page is loaded
app_page(NewURL) {
}

disp_info() {
}

; Functions to be called from the html/js source
; ----------------------------------------------

; Move the next active window into the public space
MyButton1() {
	; Assuming the GUI is the active window since the button was clicked, wait until it is not active
	guiWinID := WinExist("A") ;TODO use actual known GUI id. For some reason __Webapp_GuiHwnd doesnt work
	WinWaitNotActive, ahk_id %guiWinID%,, 5
		
	; Move the active window to the public space
	curWinID := WinExist("A")
	WinMove, % "ahk_id" curWinID,, PubSpaceLeft - bdrThkX, PubSpaceTop, PubSpaceWidth + 2*bdrThkX, PubSpaceHeight + bdrThkY
}

; Move the next active window into the private space
MyButton2() {
	; Assuming the GUI is the active window since the button was clicked, wait until it is not active
	guiWinID := WinExist("A") ;TODO use actual known GUI id. For some reason __Webapp_GuiHwnd doesnt work
	WinWaitNotActive, ahk_id %guiWinID%,, 5
		
	; Move the active window to the private space
	curWinID := WinExist("A")
	WinMove, % "ahk_id " curWinID,, PvtSpaceLeft-bdrThkX, PvtSpaceTop, PvtSpaceWidth+2*bdrThkX, PvtSpaceHeight 

}
