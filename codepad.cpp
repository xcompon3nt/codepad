/*--------------------------------------------------------------------------
  Codepad - Lightweight Win32 Syntax Editor
  Copyright (C) 2026 Mincho Georgiev
 
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------------*/
 
#pragma warning(disable: 4005)
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>
#include "crypto.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

#define APPNAME             L"Codepad"
#define VERSION             L"1.0.067"

#define CUSTOM_EDIT_CLASS   L"CustomSyntaxEditor"
#define SETTINGS_DLG_CLASS  L"ThemeSettingsDialog"
#define FIND_DLG_CLASS      L"FindDialogClass"
#define REPLACE_DLG_CLASS   L"ReplaceDialogClass"
#define GOTO_DLG_CLASS      L"GotoDialogClass"
#define ABOUT_DLG_CLASS     L"AboutDialogClass"
#define PASS_DLG_CLASS      L"PasswordDialogClass"
#define GREP_DLG_CLASS      L"FindInFilesDialogClass"
#define OUTPUT_PANE_CLASS   L"GrepOutputPaneClass"
#define SPLITTER_CLASS      L"GrepSplitterClass"

#define TEXT_MARGIN_X 8
#define MAX_UNDO_STEPS 10000
#define IDT_AUTOSCROLL 101
#define IDT_FULLSCREEN 102

#define MIN_ZOOM_POINT_SIZE 6
#define MAX_ZOOM_POINT_SIZE 48
#define MAX_SCROLL_UNITS    30000

// Menu IDs - File
#define IDM_FILE_NEW         1001
#define IDM_FILE_OPEN        1002
#define IDM_FILE_SAVE        1003
#define IDM_FILE_SAVEAS      1004
#define IDM_FILE_EXIT        1005

// Menu IDs - Edit
#define IDM_EDIT_UNDO        1008
#define IDM_EDIT_REDO        1009
#define IDM_EDIT_CUT         1010
#define IDM_EDIT_COPY        1011
#define IDM_EDIT_PASTE       1012
#define IDM_EDIT_DELETE      1013
#define IDM_EDIT_SELECTALL   1014
#define IDM_EDIT_UPPERCASE   1015
#define IDM_EDIT_LOWERCASE   1016

// Menu IDs - Search
#define IDM_SEARCH_FIND         1040
#define IDM_SEARCH_FINDNEXT     1041
#define IDM_SEARCH_FINDPREV     1042
#define IDM_SEARCH_FINDINFILES  1045
#define IDM_SEARCH_REPLACE      1043
#define IDM_SEARCH_GOTO         1044

// Menu IDs - View
#define IDM_VIEW_FULLSCREEN       1020
#define IDM_VIEW_DARK             1021
#define IDM_VIEW_LIGHT            1022
#define IDM_VIEW_FONT             1023
#define IDM_VIEW_THEME_DLG        1024
#define IDM_VIEW_GUIDES           1025
#define IDM_VIEW_LINENUMBERS      1026
#define IDM_VIEW_BRACES           1027
#define IDM_VIEW_STATUSBAR        1028
#define IDM_VIEW_HIGHLIGHT_GUIDES 1029

// Menu IDs - Indentation
#define IDM_INDENT_SIZE_2    1060
#define IDM_INDENT_SIZE_4    1061
#define IDM_INDENT_SIZE_8    1062
#define IDM_INDENT_SPACES    1063

// Menu IDs - Tools
#define IDM_TOOLS_ENCRYPT    1030
#define IDM_TOOLS_DECRYPT    1031

// Menu IDs - Help
#define IDM_HELP_ABOUT       1050

#define IDM_ENC_BASE         1100

// Dialog Controls
#define IDC_DLG_TAB          2000
#define IDC_DLG_ELEMENT_LIST 2001
#define IDC_DLG_CHOOSE_COLOR 2002
#define IDC_DLG_PREVIEW_BOX  2003
#define IDC_DLG_KW_EDIT      2004
#define IDC_DLG_PREPROC_EDIT 2005
#define IDC_DLG_OK           2006
#define IDC_DLG_CANCEL       2007

#define IDC_SAVE_ENC_STATIC  3001
#define IDC_SAVE_ENC_COMBO   3002
#define IDC_STATUSBAR        3003

// Find & Replace Controls
#define IDFINDWHAT           4001
#define IDREPLACEWITH        4002
#define IDWHOLEWORD          4003
#define IDMATCHCASE          4004
#define IDWRAP               4005
#define IDDIRUP              4006
#define IDDIRDOWN            4007
#define IDREPLACE            4008
#define IDREPLACEALL         4009
#define IDREPLACEINSEL       4010
#define IDREPLDONE           4011
#define IDGOTO_LINE          4012

// Find in Files Controls
#define IDFILES              4020
#define IDDIRECTORY          4021
#define IDDOTDOT             4022
#define IDBROWSE             4023
#define IDC_OUTPUT_CLOSE     4024
#define IDC_OUTPUT_LIST      4025

// Password Dialog Controls
#define IDC_CMD_PASS         5001
#define IDC_CMD_OK           5002

// Memory Restrictions:
#define UNDOSTACK_MEMORY     (256 * 1024 * 1024)
#define UNDOSTACK_MAX_LINES  (1000000)
#define CODEBLOCKS_MAX_LINES (1000000)


// Encoding Definitions
typedef struct {
    int codePage;
    const wchar_t* name;
    int bomType;
} EncodingDesc;

static const EncodingDesc g_encodings[] = {
    { CP_ACP,L"ANSI",                           0 },
    { 65001, L"UTF-8",                          0 },
    { 65001, L"UTF-8 with BOM",                 1 },
    { 1200,  L"UTF-16 LE (Unicode)",            2 },
    { 1201,  L"UTF-16 BE (Big Endian)",         3 },
    { 1251,  L"Windows-1251 (Cyrillic)",        0 },
    { 1252,  L"Windows-1252 (Western Latin)",   0 }
};
#define ENCODING_COUNT ((int)(sizeof(g_encodings) / sizeof(g_encodings[0])))

typedef struct {
    COLORREF bg;
    COLORREF text;
    COLORREF keyword;
    COLORREF preprocessor;
    COLORREF comment;
    COLORREF string;
    COLORREF number;
    COLORREF indentGuide;
    COLORREF activeIndentGuide;
    COLORREF braceMatchBorder;
    COLORREF braceMatchBg;
    COLORREF selection;
    COLORREF gutterBg;
    COLORREF gutterText;
    COLORREF gutterLine;
} Theme;

static Theme g_darkTheme = {
    RGB(30, 30, 30),    // bg: #1E1E1E
    RGB(220, 220, 220), // text
    RGB(86, 156, 214),  // keyword
    RGB(155, 155, 155), // preprocessor
    RGB(106, 153, 85),  // comment
    RGB(206, 145, 120), // string
    RGB(181, 206, 168), // number
    RGB(50, 50, 50),    // indentGuide
    RGB(0, 150, 255),   // activeIndentGuide
    RGB(0, 150, 255),   // braceMatchBorder
    RGB(45, 60, 80),    // braceMatchBg
    RGB(38, 79, 120),   // selection
    RGB(37, 37, 38),    // gutterBg
    RGB(133, 133, 133), // gutterText
    RGB(60, 60, 60)     // gutterLine
};

static Theme g_lightTheme = {
    RGB(255, 255, 255), // bg: #FFFFFF
    RGB(0, 0, 0),       // text
    RGB(0, 0, 127),     // keyword
    RGB(127, 127, 0),   // preprocessor
    RGB(0, 127, 0),     // comment
    RGB(127, 0, 127),   // string
    RGB(0, 127, 127),   // number
    RGB(215, 215, 215), // indentGuide
    RGB(0, 0, 127),     // activeIndentGuide
    RGB(0, 0, 255),     // braceMatchBorder
    RGB(215, 235, 255), // braceMatchBg
    RGB(192, 192, 192), // selection
    RGB(224, 224, 224), // gutterBg
    RGB(128, 128, 128), // gutterText
    RGB(192, 192, 192)  // gutterLine
};

static wchar_t g_keywordsBuffer[4096] = 
    L"auto break case char const continue default do double else enum extern float for goto if "
    L"int long register return short signed sizeof static struct switch typedef union unsigned void volatile while "
    L"class public private protected template typename namespace using new delete true false nullptr virtual override "
    L"inline explicit constexpr noexcept decltype static_assert friend mutable";

static wchar_t g_preprocBuffer[2048] = 
    L"include define undef ifdef ifndef endif elif else if pragma error line warning";

typedef struct {
    int64_t col; // 64-bit safe column tracking
    int row;
} TextPos;

typedef struct {
    wchar_t* text;
    size_t length;
    size_t capacity;
    BOOL inComment;
} Line;

typedef struct {
    int openRow;
    int openCol;
    int closeRow;
    int closeCol;
    int guideCol;
} CodeBlock;

typedef struct {
    wchar_t* text;
    TextPos caret;
    TextPos selAnchor;
    BOOL isModified;
    int64_t scrollX;
    int scrollY;
} UndoStep;

typedef struct {
    UndoStep* steps;
    int count;
    int capacity;
} UndoStack;

typedef struct {
    wchar_t findText[256];
    wchar_t replaceText[256];
    BOOL matchWholeWord;
    BOOL matchCase;
    BOOL wrapAround;
    BOOL searchDown;
} SearchConfig;

static SearchConfig g_search = { L"", L"", FALSE, FALSE, TRUE, TRUE };

// Find in Files persistent settings
static wchar_t g_grepPatterns[512] = L"*.c *.cpp *.h *.asm *.inc *.cxx *.pas";
static wchar_t g_grepDirectory[MAX_PATH] = L"";
static int g_splitterHeight = -1; // -1 means default 1/4 of window
static BOOL g_showOutputPane = FALSE;

typedef struct {
    Line* lines;
    int lineCount;
    int lineCapacity;

    TextPos caret;
    TextPos selAnchor;
    int64_t preferredCol;
    BOOL isMouseDown;
    BOOL isGutterSelecting;
    BOOL isPendingDrag;
    BOOL isDraggingText;
    POINT dragStartPt;
    TextPos dragSelAnchor;
    TextPos dragSelCaret;
    TextPos dragDropPos;

    DWORD lastDblClickTime;
    POINT lastDblClickPt;

    BOOL hasBraceMatch;
    TextPos brace1;
    TextPos brace2;

    CodeBlock* blocks;
    int blockCount;
    int blockCapacity;

    UndoStack undoStack;
    UndoStack redoStack;

    HFONT hFont;
    wchar_t fontFace[64];
    int pointSize;
    int zoomLevel;
    BOOL fontBold;
    int fontQuality;
    int charWidth;
    int charHeight;
    int gutterWidth;

    int64_t scrollX;
    int scrollY;
    int64_t maxCols;
    int visibleLines;

    BOOL isDark;
    BOOL showIndentGuides;
    BOOL highlightIndentGuides;
    BOOL showBraceMatch;
    BOOL showLineNumbers;
    BOOL showStatusBar;
    int tabSize;
    BOOL useSpaces;
    int encodingIdx;
    BOOL isModified;
    wchar_t filePath[MAX_PATH];
    wchar_t fileName[MAX_PATH];
    HWND hwndMain;
    HWND hwndStatus;
    
    BOOL isCtrlSolo;
    
} EditorState;

// Global State
static HWND g_hwndEditor = NULL;
static HWND g_hFindDlg = NULL;
static HWND g_hReplaceDlg = NULL;
static HWND g_hGotoDlg = NULL;
static HWND g_hGrepDlg = NULL;
static HWND g_hwndOutputPane = NULL;
static HWND g_hwndOutputList = NULL;
static HWND g_hwndOutputClose = NULL;
static HWND g_hwndSplitter = NULL;
static BOOL g_isSplitting = FALSE;

static BOOL g_isFullscreen = FALSE;
static WINDOWPLACEMENT g_prevPlacement = { sizeof(WINDOWPLACEMENT) };
static DWORD g_prevStyle = 0;
static int g_selectedFontQuality = CLEARTYPE_QUALITY;
static HWND g_hFontQualCombo = NULL;
static int g_selectedDlgEncoding = 0;
static HWND g_hDlgCombo = NULL;
static HCURSOR g_hDragCursor = NULL;

static inline Theme* GetActiveTheme(EditorState* s) {
    return (s && s->isDark) ? &g_darkTheme : &g_lightTheme;
}

static void SetCursorPixel(BYTE* andMask, BYTE* xorMask, int x, int y, int type) {
    if (x < 0 || x >= 32 || y < 0 || y >= 32) return;
    int idx = y * 4 + (x / 8);
    BYTE mask = (BYTE)(0x80 >> (x % 8));
    andMask[idx] &= ~mask;
    if (type == 1) xorMask[idx] |= mask;
    else xorMask[idx] &= ~mask;
}

static HCURSOR CreateDragCursor(void) {
    BYTE andMask[128];
    BYTE xorMask[128];
    memset(andMask, 0xFF, sizeof(andMask));
    memset(xorMask, 0x00, sizeof(xorMask));

    for (int y = 0; y <= 16; y++) SetCursorPixel(andMask, xorMask, 0, y, 0);
    for (int y = 0; y <= 11; y++) {
        SetCursorPixel(andMask, xorMask, y, y, 0);
        for (int x = 1; x < y; x++) SetCursorPixel(andMask, xorMask, x, y, 1);
    }
    SetCursorPixel(andMask, xorMask, 1, 12, 1); SetCursorPixel(andMask, xorMask, 2, 12, 1); SetCursorPixel(andMask, xorMask, 3, 12, 0);
    SetCursorPixel(andMask, xorMask, 1, 13, 1); SetCursorPixel(andMask, xorMask, 2, 13, 0);
    SetCursorPixel(andMask, xorMask, 1, 14, 0);
    
    SetCursorPixel(andMask, xorMask, 4, 11, 0); SetCursorPixel(andMask, xorMask, 5, 11, 1); SetCursorPixel(andMask, xorMask, 6, 11, 1); SetCursorPixel(andMask, xorMask, 7, 11, 0);
    SetCursorPixel(andMask, xorMask, 6, 12, 0); SetCursorPixel(andMask, xorMask, 7, 12, 1); SetCursorPixel(andMask, xorMask, 8, 12, 0);
    SetCursorPixel(andMask, xorMask, 7, 13, 0); SetCursorPixel(andMask, xorMask, 8, 13, 1); SetCursorPixel(andMask, xorMask, 9, 13, 0);
    SetCursorPixel(andMask, xorMask, 8, 14, 0); SetCursorPixel(andMask, xorMask, 9, 14, 1); SetCursorPixel(andMask, xorMask, 10, 14, 0);
    SetCursorPixel(andMask, xorMask, 9, 15, 0); SetCursorPixel(andMask, xorMask, 10, 15, 0);

    for (int x = 13; x <= 23; x++) { SetCursorPixel(andMask, xorMask, x, 13, 0); SetCursorPixel(andMask, xorMask, x, 23, 0); }
    for (int y = 13; y <= 23; y++) { SetCursorPixel(andMask, xorMask, 13, y, 0); SetCursorPixel(andMask, xorMask, 23, y, 0); }
    for (int y = 14; y <= 22; y++) {
        for (int x = 14; x <= 22; x++) SetCursorPixel(andMask, xorMask, x, y, 1);
    }
    for (int x = 15; x <= 21; x++) SetCursorPixel(andMask, xorMask, x, 16, 0);
    for (int x = 15; x <= 21; x++) SetCursorPixel(andMask, xorMask, x, 18, 0);
    for (int x = 15; x <= 19; x++) SetCursorPixel(andMask, xorMask, x, 20, 0);

    ICONINFO ii;
    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = FALSE;
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = CreateBitmap(32, 64, 1, 1, NULL);
    BYTE combined[256];
    memcpy(combined, andMask, 128);
    memcpy(combined + 128, xorMask, 128);
    SetBitmapBits(ii.hbmMask, 256, combined);
    ii.hbmColor = NULL;
    HCURSOR hCur = CreateIconIndirect(&ii);
    DeleteObject(ii.hbmMask);
    return hCur;
}

// Forward Declarations
static void Tool_Encrypt(HWND hwndEditor, EditorState* s);
static void Tool_Decrypt(HWND hwndEditor, EditorState* s);
static BOOL ShowPasswordDialog(HWND hwndParent, const wchar_t* title, wchar_t* outPass, int maxLen);
static void ShowAboutDialog(HWND hwndParent);
static void RegisterCustomEditControl(HINSTANCE hInstance);
static void PushUndo(EditorState* s);
static void Editor_Undo(HWND hwnd, EditorState* s);
static void Editor_Redo(HWND hwnd, EditorState* s);
static void Editor_ClearAll(EditorState* s);
static void UndoStack_Clear(UndoStack* stack);
static void UpdateEncodingMenuCheck(HWND hwndMain, int activeIdx);
static void UpdateScrollbars(HWND hwnd, EditorState* s);
static void UpdateCaret(HWND hwnd, EditorState* s);
static void UpdateStatusBar(EditorState* s);
static void EnsureCaretVisible(HWND hwnd, EditorState* s);
static void EnsureCaretVisibleEx(HWND hwnd, EditorState* s, float marginRatio);
static void FindMatchingBrace(EditorState* s);
static void UpdateTitle(EditorState* s);
static void RecalculateGutter(EditorState* s);
static void RecalculateMaxCols(EditorState* s);
static void SaveSettingsToIni(HWND hwndMain, EditorState* s);
static void LoadSettingsFromIni(EditorState* s);
static void RecreateEditorFont(HWND hwnd, EditorState* s);
static BOOL PromptSaveIfModified(HWND hwndMain, EditorState* s);
static BOOL DoFileSaveAs(HWND hwndMain, EditorState* s);
static BOOL LoadFileContent(HWND hwnd, EditorState* s, const wchar_t* path);
static void Editor_SetText(EditorState* s, const wchar_t* text);
static void Editor_ConvertIndentation(EditorState* s, int oldTabSize, int newTabSize);
static void UpdateCodeBlocks(EditorState* s);
static void PaintIndentGuides(HDC hdc, EditorState* s, int lineIdx, int y, const Theme* theme, int activeBlockIdx);
static void PaintSyntaxLine(HDC hdc, EditorState* s, int lineIdx, int y, RECT* clientRc, const Theme* theme, int activeBlockIdx);
static void ShowContextMenu(HWND hwnd, EditorState* s, int screenX, int screenY);
static void ShowThemeSettingsDialog(HWND hwndParent, EditorState* s);
static void ShowFindDialog(HWND hwndParent, EditorState* s);
static void ShowReplaceDialog(HWND hwndParent, EditorState* s);
static void ShowGotoDialog(HWND hwndParent, EditorState* s);
static void ShowFindInFilesDialog(HWND hwndParent, EditorState* s);
static BOOL FindNextText(EditorState* s, BOOL searchDown);
static void Editor_MakeSelectionCase(EditorState* s, BOOL makeUpper);
static TextPos ScreenToTextPos(EditorState* s, int x, int y);
static BOOL IsCharInQuoteOrComment(EditorState* s, int r, int64_t c);
static void Line_FreeText(Line* line);
static void Line_Init(Line* line);
static void ClearSelection(EditorState* s);
static void RecalculateAllOutputListExtent(void);

static LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK ThemeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK OutputPaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK SplitterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// -------------------------------------------------------------
// Password & About Dialog Implementations
// -------------------------------------------------------------
typedef struct {
    const wchar_t* title;
    wchar_t* outPass;
    int maxLen;
    BOOL confirmed;
    HWND hEdit;
} PassDlgState;

static LRESULT CALLBACK PasswordDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PassDlgState* ds = (PassDlgState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg) {
    case WM_NCCREATE: {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        ds = (PassDlgState*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ds);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    case WM_CREATE: {
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (ds && ds->title) SetWindowTextW(hwnd, ds->title);

        ds->hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL,
            20, 18, 248, 22, hwnd, (HMENU)IDC_CMD_PASS, GetModuleHandle(NULL), NULL);
        SendMessage(ds->hEdit, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hOk = CreateWindowW(L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            275, 17, 75, 25, hwnd, (HMENU)IDC_CMD_OK, GetModuleHandle(NULL), NULL);
        SendMessage(hOk, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        SetFocus(ds->hEdit);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_CMD_OK || id == IDOK) {
            if (ds->outPass && ds->maxLen > 0) GetWindowTextW(ds->hEdit, ds->outPass, ds->maxLen);
            ds->confirmed = TRUE;
            HWND hParent = GetParent(hwnd);
            if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
            DestroyWindow(hwnd);
            return 0;
        } else if (id == IDCANCEL) {
            HWND hParent = GetParent(hwnd);
            if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_CLOSE: {
        HWND hParent = GetParent(hwnd);
        if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
        DestroyWindow(hwnd);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static BOOL ShowPasswordDialog(HWND hwndParent, const wchar_t* title, wchar_t* outPass, int maxLen) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = PasswordDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = PASS_DLG_CLASS;
    RegisterClassEx(&wc);

    PassDlgState ds;
    ds.title = title;
    ds.outPass = outPass;
    ds.maxLen = maxLen;
    ds.confirmed = FALSE;
    ds.hEdit = NULL;

    RECT rcParent;
    GetWindowRect(hwndParent, &rcParent);
    int dlgW = 380, dlgH = 100;
    int x = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, PASS_DLG_CLASS, title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH, hwndParent, NULL, GetModuleHandle(NULL), &ds
    );

    SetWindowTextW(hDlg, title);
    EnableWindow(hwndParent, FALSE);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            SendMessage(hDlg, WM_COMMAND, IDC_CMD_OK, 0);
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
    SetFocus(hwndParent);
    return ds.confirmed;
}

static LRESULT CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        HWND hTitle = CreateWindowW(L"STATIC", APPNAME, WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 18, 300, 20, hwnd, NULL, NULL, NULL);
        SendMessage(hTitle, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        wchar_t verStr[64];
        wsprintf(verStr, L"Version %s", VERSION);
        HWND hVer = CreateWindowW(L"STATIC", verStr, WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 42, 300, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hVer, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hSub = CreateWindowW(L"STATIC", L"Lightweight Syntax Editor", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 64, 300, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hSub, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 120, 96, 80, 26, hwnd, (HMENU)IDOK, NULL, NULL);
        SendMessage(hOk, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SetFocus(hOk);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            HWND hParent = GetParent(hwnd);
            if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE: {
        HWND hParent = GetParent(hwnd);
        if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
        DestroyWindow(hwnd);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void ShowAboutDialog(HWND hwndParent) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = AboutDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = ABOUT_DLG_CLASS;
    RegisterClassEx(&wc);

    RECT rcParent;
    GetWindowRect(hwndParent, &rcParent);
    int dlgW = 335, dlgH = 170;
    int x = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, ABOUT_DLG_CLASS, L"About " APPNAME,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH, hwndParent, NULL, GetModuleHandle(NULL), NULL
    );

    EnableWindow(hwndParent, FALSE);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && (msg.wParam == VK_RETURN || msg.wParam == VK_ESCAPE)) {
            SendMessage(hDlg, WM_COMMAND, IDOK, 0);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
    SetFocus(hwndParent);
}

static void SafeFreeDocText(wchar_t* buf) {
    if (buf) free(buf);
}

static wchar_t* Editor_GetText(const EditorState* s) {
    if (!s || s->lineCount <= 0) return NULL;

    size_t totalChars = 0;
    for (int r = 0; r < s->lineCount; r++) {
        totalChars += s->lines[r].length;
        if (r < s->lineCount - 1) totalChars += 2;
    }

    size_t allocBytes = (totalChars + 1) * sizeof(wchar_t);
    wchar_t* buf = (wchar_t*)malloc(allocBytes);
    if (!buf) return NULL;

    wchar_t* p = buf;
    for (int r = 0; r < s->lineCount; r++) {
        size_t len = s->lines[r].length;
        if (len > 0 && s->lines[r].text) {
            memcpy(p, s->lines[r].text, len * sizeof(wchar_t));
            p += len;
        }
        if (r < s->lineCount - 1) {
            *p++ = L'\r';
            *p++ = L'\n';
        }
    }
    *p = L'\0';
    return buf;
}

#ifndef IS_HIGH_SURROGATE
#define IS_HIGH_SURROGATE(wch) (((wch) >= 0xD800) && ((wch) <= 0xDBFF))
#endif

// -------------------------------------------------------------
// 64-bit Safe Chunked Multibyte Conversion (Bypasses Win32 2GB Limit)
// -------------------------------------------------------------
static unsigned char* Chunked_WideToUTF8(const wchar_t* wstr, size_t wlen, size_t* outLen) {
    if (!wstr || wlen == 0) {
        if (outLen) *outLen = 0;
        return NULL;
    }

    const size_t CHUNK_SIZE = 64 * 1024 * 1024;
    size_t offset = 0;
    size_t totalBytes = 0;

    while (offset < wlen) {
        size_t thisChunk = wlen - offset;
        if (thisChunk > CHUNK_SIZE) {
            thisChunk = CHUNK_SIZE;
            if (offset + thisChunk < wlen && IS_HIGH_SURROGATE(wstr[offset + thisChunk - 1])) {
                thisChunk--;
            }
        }

        int bytes = WideCharToMultiByte(CP_UTF8, 0, wstr + offset, (int)thisChunk, NULL, 0, NULL, NULL);
        if (bytes <= 0) return NULL;
        totalBytes += (size_t)bytes;
        offset += thisChunk;
    }

    unsigned char* outBuf = (unsigned char*)malloc(totalBytes + 32);
    if (!outBuf) return NULL;

    offset = 0;
    size_t writeOffset = 0;

    while (offset < wlen) {
        size_t thisChunk = wlen - offset;
        if (thisChunk > CHUNK_SIZE) {
            thisChunk = CHUNK_SIZE;
            if (offset + thisChunk < wlen && IS_HIGH_SURROGATE(wstr[offset + thisChunk - 1])) {
                thisChunk--;
            }
        }

        int bytes = WideCharToMultiByte(CP_UTF8, 0, wstr + offset, (int)thisChunk, (char*)outBuf + writeOffset, (int)(totalBytes - writeOffset + 32), NULL, NULL);
        if (bytes <= 0) {
            free(outBuf);
            return NULL;
        }
        writeOffset += (size_t)bytes;
        offset += thisChunk;
    }

    outBuf[writeOffset] = '\0';
    if (outLen) *outLen = writeOffset;
    return outBuf;
}

static wchar_t* Chunked_UTF8ToWide(const unsigned char* utf8, size_t utf8Len, size_t* outWLen) {
    if (!utf8 || utf8Len == 0) {
        if (outWLen) *outWLen = 0;
        return NULL;
    }

    const size_t CHUNK_SIZE = 64 * 1024 * 1024;
    size_t offset = 0;
    size_t totalChars = 0;

    while (offset < utf8Len) {
        size_t thisChunk = utf8Len - offset;
        if (thisChunk > CHUNK_SIZE) {
            thisChunk = CHUNK_SIZE;
            while (thisChunk > 0 && (utf8[offset + thisChunk] & 0xC0) == 0x80) {
                thisChunk--;
            }
        }

        int chars = MultiByteToWideChar(CP_UTF8, 0, (const char*)utf8 + offset, (int)thisChunk, NULL, 0);
        if (chars <= 0) {
            chars = MultiByteToWideChar(CP_ACP, 0, (const char*)utf8 + offset, (int)thisChunk, NULL, 0);
            if (chars <= 0) return NULL;
        }
        totalChars += (size_t)chars;
        offset += thisChunk;
    }

    wchar_t* outBuf = (wchar_t*)malloc((totalChars + 2) * sizeof(wchar_t));
    if (!outBuf) return NULL;

    offset = 0;
    size_t writeOffset = 0;

    while (offset < utf8Len) {
        size_t thisChunk = utf8Len - offset;
        if (thisChunk > CHUNK_SIZE) {
            thisChunk = CHUNK_SIZE;
            while (thisChunk > 0 && (utf8[offset + thisChunk] & 0xC0) == 0x80) {
                thisChunk--;
            }
        }

        int chars = MultiByteToWideChar(CP_UTF8, 0, (const char*)utf8 + offset, (int)thisChunk, outBuf + writeOffset, (int)(totalChars - writeOffset + 2));
        if (chars <= 0) {
            chars = MultiByteToWideChar(CP_ACP, 0, (const char*)utf8 + offset, (int)thisChunk, outBuf + writeOffset, (int)(totalChars - writeOffset + 2));
            if (chars <= 0) {
                free(outBuf);
                return NULL;
            }
        }
        writeOffset += (size_t)chars;
        offset += thisChunk;
    }

    outBuf[writeOffset] = L'\0';
    if (outWLen) *outWLen = writeOffset;
    return outBuf;
}

static inline int hex_char_val(wchar_t c) {
    if (c >= L'0' && c <= L'9') return (int)(c - L'0');
    if (c >= L'a' && c <= L'f') return (int)(c - L'a' + 10);
    if (c >= L'A' && c <= L'F') return (int)(c - L'A' + 10);
    return -1;
}

static void Tool_Encrypt(HWND hwndEditor, EditorState* s) 
{
    if (!s) s = (EditorState*)GetWindowLongPtr(hwndEditor, GWLP_USERDATA);
    if (!s) return;
    if (s->lineCount <= 0 || (s->lineCount == 1 && s->lines[0].length == 0)) return;

    wchar_t wpass[256] = { 0 };
    char pass[512] = { 0 };
    HWND hOwner = (s && s->hwndMain) ? s->hwndMain : hwndEditor;

    if (!ShowPasswordDialog(hOwner, L"Encrypt", wpass, 256) || wpass[0] == L'\0') {
        SecureZeroMemory(wpass, sizeof(wpass));
        return;
    }

    WideCharToMultiByte(CP_UTF8, 0, wpass, -1, pass, sizeof(pass), NULL, NULL);
    SecureZeroMemory(wpass, sizeof(wpass));

    wchar_t* text = Editor_GetText(s);
    if (!text) {
        SecureZeroMemory(pass, sizeof(pass));
        MessageBoxW(hOwner, L"Insufficient memory for encryption.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    size_t wLen = wcslen(text);
    if (wLen == 0) {
        SafeFreeDocText(text);
        SecureZeroMemory(pass, sizeof(pass));
        return;
    }

    size_t lentext = 0;
    unsigned char* bufftext = Chunked_WideToUTF8(text, wLen, &lentext);
    SafeFreeDocText(text);
    if (!bufftext) {
        SecureZeroMemory(pass, sizeof(pass));
        MessageBoxW(hOwner, L"Insufficient memory for encryption.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    uint32_t lencrypt = 0;
    unsigned char* buffcrypt = AES_Encrypt(bufftext, (uint32_t)lentext, pass, &lencrypt);
    free(bufftext);
    SecureZeroMemory(pass, sizeof(pass));

    if (!buffcrypt) {
        MessageBoxW(hOwner, L"Insufficient memory for encryption.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    uint32_t lenhex = 0;
    char* buffhex = hex_encode(buffcrypt, lencrypt, &lenhex);
    free(buffcrypt);
    if (!buffhex) {
        MessageBoxW(hOwner, L"Insufficient memory for encryption.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t* wOutHex = (wchar_t*)malloc(((size_t)lenhex + 2) * sizeof(wchar_t));
    if (!wOutHex) {
        free(buffhex);
        MessageBoxW(hOwner, L"Insufficient memory for encryption.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    for (size_t i = 0; i < (size_t)lenhex; i++) {
        wOutHex[i] = (wchar_t)buffhex[i];
    }
    wOutHex[lenhex] = L'\0';
    free(buffhex);
  
    PushUndo(s);
    Editor_SetText(s, wOutHex);
    free(wOutHex);

    s->isModified = TRUE;
    UpdateTitle(s);
    UpdateScrollbars(hwndEditor, s);
    UpdateCaret(hwndEditor, s);
    FindMatchingBrace(s);
    InvalidateRect(hwndEditor, NULL, FALSE);
    UpdateWindow(hwndEditor);
}

static void Tool_Decrypt(HWND hwndEditor, EditorState* s) 
{
    if (!s) s = (EditorState*)GetWindowLongPtr(hwndEditor, GWLP_USERDATA);
    if (!s) return;
    if (s->lineCount <= 0 || (s->lineCount == 1 && s->lines[0].length == 0)) return;

    size_t wLen = 0;
    wchar_t* textAllocated = NULL;
    const wchar_t* textPtr = NULL;

    if (s->lineCount == 1 && s->lines[0].text) {
        wLen = s->lines[0].length;
        textPtr = s->lines[0].text;
    } else {
        textAllocated = Editor_GetText(s);
        if (!textAllocated) {
            HWND hOwner = (s && s->hwndMain) ? s->hwndMain : hwndEditor;
            MessageBoxW(hOwner, L"Insufficient memory for decryption.", L"Error", MB_OK | MB_ICONERROR);
            return;
        }
        wLen = wcslen(textAllocated);
        textPtr = textAllocated;
    }

    size_t start = 0;
    while (start < wLen && (textPtr[start] == L' ' || textPtr[start] == L'\t' || textPtr[start] == L'\r' || textPtr[start] == L'\n')) start++;
    size_t end = wLen;
    while (end > start && (textPtr[end - 1] == L' ' || textPtr[end - 1] == L'\t' || textPtr[end - 1] == L'\r' || textPtr[end - 1] == L'\n')) end--;

    size_t hexLen = end - start;

    // Minimum header length requirement for AES-GCM payload
    if (hexLen < (CRYPTO_HEADER_SIZE * 2) || (hexLen % 2) != 0) {
        if (textAllocated) SafeFreeDocText(textAllocated);
        return;
    }

    for (size_t i = start; i < end; i++) {
        wchar_t c = textPtr[i];
        if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F'))) {
            if (textAllocated) SafeFreeDocText(textAllocated);
            return;
        }
    }

    wchar_t wpass[256] = { 0 };
    char pass[512] = { 0 };
    HWND hOwner = (s && s->hwndMain) ? s->hwndMain : hwndEditor;
    
    if (!ShowPasswordDialog(hOwner, L"Decrypt", wpass, 256) || wpass[0] == L'\0') {
        SecureZeroMemory(wpass, sizeof(wpass));
        if (textAllocated) SafeFreeDocText(textAllocated);
        return;
    }

    WideCharToMultiByte(CP_UTF8, 0, wpass, -1, pass, sizeof(pass), NULL, NULL);
    SecureZeroMemory(wpass, sizeof(wpass));

    size_t binLen = hexLen / 2;
    uint8_t* buffhex = (uint8_t*)malloc(binLen + 32);
    if (!buffhex) {
        SecureZeroMemory(pass, sizeof(pass));
        if (textAllocated) SafeFreeDocText(textAllocated);
        MessageBoxW(hOwner, L"Insufficient memory for decryption.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
 
    const wchar_t* hexData = textPtr + start;
    for (size_t i = 0; i < binLen; i++) {
        int hi = hex_char_val((wchar_t)hexData[i * 2]);
        int lo = hex_char_val((wchar_t)hexData[i * 2 + 1]);
        buffhex[i] = (uint8_t)((hi << 4) | lo);
    }

    if (textAllocated) {
        SafeFreeDocText(textAllocated);
        textAllocated = NULL;
    }

    uint32_t pt_len = 0;
    BOOL dec_success = AES_Decrypt(buffhex, (uint32_t)binLen, pass, &pt_len);
    SecureZeroMemory(pass, sizeof(pass));

    if (!dec_success || pt_len == 0) {
        free(buffhex);
        return;
    }

    size_t outWideLen = 0;
    wchar_t* wOutText = Chunked_UTF8ToWide(buffhex, (size_t)pt_len, &outWideLen);
    free(buffhex);

    if (!wOutText) {
        MessageBoxW(hOwner, L"Insufficient memory for decryption.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    PushUndo(s);
    Editor_SetText(s, wOutText);
    free(wOutText);

    //autoscroll to the end of the decrypted text:
    s->caret.row = s->lineCount - 1;
    s->caret.col = (int64_t)s->lines[s->caret.row].length;
    s->preferredCol = s->caret.col;
    ClearSelection(s);
    EnsureCaretVisible(hwndEditor, s);
    // ---------------------------------------------------

    s->isModified = TRUE;
    UpdateTitle(s);
    UpdateScrollbars(hwndEditor, s);
    UpdateCaret(hwndEditor, s);
    FindMatchingBrace(s);
    InvalidateRect(hwndEditor, NULL, FALSE);
    UpdateWindow(hwndEditor);
}

static void UndoStack_Clear(UndoStack* stack) {
    if (!stack || !stack->steps) return;
    for (int i = 0; i < stack->count; i++) {
        if (stack->steps[i].text) {
            SafeFreeDocText(stack->steps[i].text);
            stack->steps[i].text = NULL;
        }
    }
    free(stack->steps);
    stack->steps = NULL;
    stack->count = 0;
    stack->capacity = 0;
}

#define MAX_UNDO_MEM_BYTES (1024ULL * 1024 * 1024)

static void UndoStack_PushInternal(UndoStack* stack, const EditorState* s) {
    if (!stack || !s) return;

    size_t estBytes = 0;
    for (int r = 0; r < s->lineCount; r++) {
        estBytes += (s->lines[r].length + 2) * sizeof(wchar_t);
        if (estBytes > UNDOSTACK_MEMORY) break;
    }

    if (estBytes > UNDOSTACK_MEMORY || s->lineCount > UNDOSTACK_MAX_LINES) {
        UndoStack_Clear(stack);
        return;
    }

    wchar_t* docText = Editor_GetText(s);
    if (!docText) return;

    size_t docBytes = (wcslen(docText) + 1) * sizeof(wchar_t);

    while (stack->count > 0 && ((stack->count >= MAX_UNDO_STEPS) || ((size_t)docBytes * stack->count > MAX_UNDO_MEM_BYTES))) {
        if (stack->steps[0].text) SafeFreeDocText(stack->steps[0].text);
        memmove(&stack->steps[0], &stack->steps[1], (size_t)(stack->count - 1) * sizeof(UndoStep));
        stack->count--;
    }

    if (stack->capacity == 0) {
        stack->capacity = 16;
        stack->steps = (UndoStep*)malloc(stack->capacity * sizeof(UndoStep));
    } else if (stack->count >= stack->capacity) {
        int newCap = stack->capacity * 2;
        UndoStep* newSteps = (UndoStep*)realloc(stack->steps, (size_t)newCap * sizeof(UndoStep));
        if (newSteps) {
            stack->steps = newSteps;
            stack->capacity = newCap;
        }
    }

    if (!stack->steps) {
        SafeFreeDocText(docText);
        return;
    }

    UndoStep* step = &stack->steps[stack->count++];
    step->text = docText;
    step->caret = s->caret;
    step->selAnchor = s->selAnchor;
    step->isModified = s->isModified;
    //record viewport position:
    step->scrollX = s->scrollX;
    step->scrollY = s->scrollY;
}

static void PushUndo(EditorState* s) {
    UndoStack_PushInternal(&s->undoStack, s);
    UndoStack_Clear(&s->redoStack);
}

static void Editor_Undo(HWND hwnd, EditorState* s) {
    if (s->undoStack.count <= 0) return;
    
    // 1. Remember where the user is currently looking
    int64_t prevScrollX = s->scrollX;
    int prevScrollY = s->scrollY;

    UndoStack_PushInternal(&s->redoStack, s);
    UndoStep step = s->undoStack.steps[--s->undoStack.count];

    if (step.text) {
        Editor_SetText(s, step.text);
        SafeFreeDocText(step.text);
    }
    s->caret = step.caret;
    s->selAnchor = step.selAnchor;
    s->preferredCol = step.caret.col;
    s->isModified = step.isModified;
    
    // 2. Check if the restored caret is already visible in the user's viewport
    RECT rc;
    GetClientRect(hwnd, &rc);
    int charH = (s->charHeight > 0) ? s->charHeight : 18;
    int caretY = (s->caret.row * charH) - prevScrollY;
    BOOL isCaretVisible = (caretY >= 0 && (caretY + charH) <= rc.bottom);

    if (isCaretVisible) {
        // KEEP EXACT VIEWPORT (Zero jump)
        s->scrollX = prevScrollX;
        s->scrollY = prevScrollY;
    } else {
        // Off-screen: Restore previous view and ensure it's visible
        s->scrollX = step.scrollX;
        s->scrollY = step.scrollY;
        EnsureCaretVisible(hwnd, s);
    }

    UpdateCodeBlocks(s);
    RecalculateMaxCols(s);
    RecalculateGutter(s);
    FindMatchingBrace(s);
    UpdateScrollbars(hwnd, s);
    UpdateCaret(hwnd, s);
    UpdateTitle(s);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void Editor_Redo(HWND hwnd, EditorState* s) {
    if (s->redoStack.count <= 0) return;
    
    // 1. Remember where the user is currently looking
    int64_t prevScrollX = s->scrollX;
    int prevScrollY = s->scrollY;

    UndoStack_PushInternal(&s->undoStack, s);
    UndoStep step = s->redoStack.steps[--s->redoStack.count];

    if (step.text) {
        Editor_SetText(s, step.text);
        SafeFreeDocText(step.text);
    }
    s->caret = step.caret;
    s->selAnchor = step.selAnchor;
    s->preferredCol = step.caret.col;
    s->isModified = step.isModified;
    
    // 2. Check if the restored caret is already visible in the user's viewport
    RECT rc;
    GetClientRect(hwnd, &rc);
    int charH = (s->charHeight > 0) ? s->charHeight : 18;
    int caretY = (s->caret.row * charH) - prevScrollY;
    BOOL isCaretVisible = (caretY >= 0 && (caretY + charH) <= rc.bottom);

    if (isCaretVisible) {
        // KEEP EXACT VIEWPORT (Zero jump)
        s->scrollX = prevScrollX;
        s->scrollY = prevScrollY;
    } else {
        // Off-screen: Restore previous view and ensure it's visible
        s->scrollX = step.scrollX;
        s->scrollY = step.scrollY;
        EnsureCaretVisible(hwnd, s);
    }

    UpdateCodeBlocks(s);
    RecalculateMaxCols(s);
    RecalculateGutter(s);
    FindMatchingBrace(s);
    UpdateScrollbars(hwnd, s);
    UpdateCaret(hwnd, s);
    UpdateTitle(s);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void UpdateEncodingMenuCheck(HWND hwndMain, int activeIdx) {
    HMENU hMenuBar = GetMenu(hwndMain);
    if (!hMenuBar) return;
    HMENU hViewMenu = GetSubMenu(hMenuBar, 3);
    if (!hViewMenu) return;
    
    int count = GetMenuItemCount(hViewMenu);
    for (int i = 0; i < count; i++) {
        HMENU hSub = GetSubMenu(hViewMenu, i);
        if (hSub) {
            MENUITEMINFOW mii;
            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_ID;
            if (GetMenuItemInfoW(hSub, 0, TRUE, &mii) && mii.wID >= IDM_ENC_BASE && mii.wID < IDM_ENC_BASE + ENCODING_COUNT) {
                CheckMenuRadioItem(hSub, IDM_ENC_BASE, IDM_ENC_BASE + ENCODING_COUNT - 1,
                                   IDM_ENC_BASE + activeIdx, MF_BYCOMMAND);
                break;
            }
        }
    }
}

static BOOL IsInWordList(const wchar_t* word, const wchar_t* list) {
    wchar_t buf[4096];
    wcscpy_s(buf, 4096, list);
    wchar_t* context = NULL;
    wchar_t* token = wcstok_s(buf, L" ", &context);
    while (token) {
        if (wcscmp(word, token) == 0) return TRUE;
        token = wcstok_s(NULL, L" ", &context);
    }
    return FALSE;
}

static BOOL IsIdentChar(wchar_t c) {
    return (c >= L'a' && c <= L'z') ||
           (c >= L'A' && c <= L'Z') ||
           (c >= L'0' && c <= L'9') ||
           (c == L'_');
}

static int PosCompare(TextPos a, TextPos b) {
    if (a.row != b.row) return a.row - b.row;
    if (a.col < b.col) return -1;
    if (a.col > b.col) return 1;
    return 0;
}

static BOOL HasSelection(EditorState* s) {
    return PosCompare(s->selAnchor, s->caret) != 0;
}

static void GetSelectionRange(EditorState* s, TextPos* start, TextPos* end) {
    if (PosCompare(s->selAnchor, s->caret) <= 0) {
        *start = s->selAnchor; *end = s->caret;
    } else {
        *start = s->caret; *end = s->selAnchor;
    }
}

static BOOL IsPosInSelection(EditorState* s, TextPos pos) {
    if (!HasSelection(s)) return FALSE;
    TextPos start, end;
    GetSelectionRange(s, &start, &end);
    return (PosCompare(pos, start) >= 0 && PosCompare(pos, end) < 0);
}

static void ClearSelection(EditorState* s) {
    s->selAnchor = s->caret;
}

static void SetModified(EditorState* s, BOOL mod) {
    if (s->isModified != mod) {
        s->isModified = mod;
        UpdateTitle(s);
    }
}

static void UpdateTitle(EditorState* s) {
    if (!s->hwndMain) return;
    wchar_t title[MAX_PATH + 32];
    const wchar_t* fn = (s->fileName[0] != L'\0') ? s->fileName : L"Untitled";
    wsprintf(title, L"%s%s", fn, s->isModified ? L"*" : L"");
    SetWindowTextW(s->hwndMain, title);
}

static void RecalculateGutter(EditorState* s) {
    if (!s->showLineNumbers) {
        s->gutterWidth = 0;
        return;
    }
    int digits = 1;
    int temp = s->lineCount;
    while (temp >= 10) { digits++; temp /= 10; }
    if (digits < 3) digits = 3;
    s->gutterWidth = (digits + 2) * s->charWidth;
}

static void RecalculateMaxCols(EditorState* s) {
    s->maxCols = 0;
    for (int i = 0; i < s->lineCount; i++) {
        if ((int64_t)s->lines[i].length > s->maxCols) {
            s->maxCols = (int64_t)s->lines[i].length;
        }
    }
}

static void RecreateEditorFont(HWND hwnd, EditorState* s) {
    if (s->hFont) DeleteObject(s->hFont);
    HDC hdc = GetDC(hwnd);
    int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    int effectiveSize = s->pointSize + s->zoomLevel;
    if (effectiveSize < MIN_ZOOM_POINT_SIZE) effectiveSize = MIN_ZOOM_POINT_SIZE;
    if (effectiveSize > MAX_ZOOM_POINT_SIZE) effectiveSize = MAX_ZOOM_POINT_SIZE;
    int fontHeight = -MulDiv(effectiveSize, logPixelsY, 72);

    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = fontHeight;
    lf.lfWeight = s->fontBold ? FW_BOLD : FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = (BYTE)s->fontQuality;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    wcscpy_s(lf.lfFaceName, 64, s->fontFace);

    s->hFont = CreateFontIndirectW(&lf);

    HGDIOBJ oldFont = SelectObject(hdc, s->hFont);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    s->charWidth = tm.tmAveCharWidth;
    s->charHeight = tm.tmHeight;
    SelectObject(hdc, oldFont);
    ReleaseDC(hwnd, hdc);

    RecalculateGutter(s);
    UpdateScrollbars(hwnd, s);

    if (GetFocus() == hwnd) {
        DestroyCaret();
        CreateCaret(hwnd, NULL, 2, s->charHeight > 0 ? s->charHeight : 18);
        UpdateCaret(hwnd, s);
        ShowCaret(hwnd);
    }

    if (g_hwndOutputList && IsWindow(g_hwndOutputList)) {
        SendMessage(g_hwndOutputList, WM_SETFONT, (WPARAM)s->hFont, TRUE);
        SendMessage(g_hwndOutputList, LB_SETITEMHEIGHT, 0, (s->charHeight > 0 ? s->charHeight : 18));
        RecalculateAllOutputListExtent();
        InvalidateRect(g_hwndOutputList, NULL, TRUE);
    }

    InvalidateRect(hwnd, NULL, TRUE);
}

static void Line_FreeText(Line* line) {
    if (!line) return;
    if (line->text) {
        free(line->text);
        line->text = NULL;
    }
    line->capacity = 0;
    line->length = 0;
    line->inComment = FALSE;
}

static void Line_Init(Line* line) {
    if (!line) return;
    line->capacity = 16;
    line->length = 0;
    line->inComment = FALSE;
    line->text = (wchar_t*)malloc(line->capacity * sizeof(wchar_t));
    if (line->text) {
        line->text[0] = L'\0';
    } else {
        line->capacity = 0;
    }
}

static void Line_InsertChar(Line* line, size_t index, wchar_t ch) {
    if (!line) return;
    if (index > line->length) index = line->length;

    if (!line->text || line->length + 2 > line->capacity) {
        size_t add = (line->capacity < 65536) ? (line->capacity < 16 ? 16 : line->capacity) : 65536;
        size_t newCap = line->capacity + add;
        if (newCap < line->length + 2) newCap = line->length + 2;

        wchar_t* newText = (wchar_t*)realloc(line->text, newCap * sizeof(wchar_t));
        if (!newText) return;

        line->text = newText;
        line->capacity = newCap;
    }

    size_t moveCount = line->length - index + 1;
    if (moveCount > 0 && line->text) {
        memmove(&line->text[index + 1], &line->text[index], moveCount * sizeof(wchar_t));
    }
    line->text[index] = ch;
    line->length++;
    line->text[line->length] = L'\0';
}

static void Line_DeleteChars(Line* line, size_t index, size_t count) {
    if (index >= line->length || count == 0) return;
    if (index + count > line->length) count = line->length - index;

    memmove(&line->text[index], &line->text[index + count], (line->length - index - count + 1) * sizeof(wchar_t));
    line->length -= count;
}

static BOOL Editor_InsertLine(EditorState* s, int row) {
    if (s->lineCount + 1 > s->lineCapacity) {
        int add = (s->lineCapacity < 65536) ? (s->lineCapacity + 16) : 65536;
        int newCap = s->lineCapacity + add;
        Line* newLines = (Line*)realloc(s->lines, (size_t)newCap * sizeof(Line));
        if (!newLines) {
            newCap = s->lineCount + 1024;
            newLines = (Line*)realloc(s->lines, (size_t)newCap * sizeof(Line));
            if (!newLines) return FALSE;
        }
        s->lines = newLines;
        s->lineCapacity = newCap;
    }
    if (row < s->lineCount) {
        memmove(&s->lines[row + 1], &s->lines[row], (size_t)(s->lineCount - row) * sizeof(Line));
    }
    Line_Init(&s->lines[row]);
    s->lineCount++;
    return TRUE;
}

static void Editor_DeleteLine(EditorState* s, int row) {
    if (s->lineCount <= 1 || row < 0 || row >= s->lineCount) return;
    Line_FreeText(&s->lines[row]);
    memmove(&s->lines[row], &s->lines[row + 1], (size_t)(s->lineCount - row - 1) * sizeof(Line));
    s->lineCount--;
    RecalculateGutter(s);
}

static void Editor_ClearAll(EditorState* s) {
    if (s->lines) {
        for (int i = 0; i < s->lineCount; i++) {
            Line_FreeText(&s->lines[i]);
        }
        free(s->lines);
        s->lines = NULL;
    }

    s->lineCapacity = 16;
    s->lineCount = 1;
    s->lines = (Line*)malloc(s->lineCapacity * sizeof(Line));
    Line_Init(&s->lines[0]);
    s->caret.row = 0;
    s->caret.col = 0;
    s->preferredCol = 0;
    ClearSelection(s);
    s->scrollX = 0;
    s->scrollY = 0;
    RecalculateGutter(s);
}

static void Editor_SetText(EditorState* s, const wchar_t* text) {
    Editor_ClearAll(s);
    if (!text || !*text) {
        UpdateCodeBlocks(s);
        RecalculateMaxCols(s);
        RecalculateGutter(s);
        UpdateStatusBar(s);
        return;
    }

    int count = 1;
    for (const wchar_t* scan = text; *scan; scan++) {
        if (*scan == L'\n') count++;
    }

    if (count > s->lineCapacity) {
        Line* newLines = (Line*)realloc(s->lines, (size_t)(count + 16) * sizeof(Line));
        if (newLines) {
            s->lines = newLines;
            s->lineCapacity = count + 16;
        }
    }

    int tabW = (s->tabSize > 0) ? s->tabSize : 2;
    const wchar_t* p = text;
    int curRow = 0;

    while (*p) {
        const wchar_t* lineStart = p;
        BOOL hasTab = FALSE;
        while (*p && *p != L'\r' && *p != L'\n') {
            if (*p == L'\t') hasTab = TRUE;
            p++;
        }
        size_t rawLen = (size_t)(p - lineStart);

        if (curRow >= s->lineCount) {
            Line_Init(&s->lines[curRow]);
            s->lineCount = curRow + 1;
        }

        Line* curLine = &s->lines[curRow];
        if (!hasTab) {
            if (rawLen + 1 > curLine->capacity) {
                Line_FreeText(curLine);
                curLine->text = (wchar_t*)malloc((rawLen + 1) * sizeof(wchar_t));
                if (curLine->text) curLine->capacity = rawLen + 1;
            }
            if (curLine->text && curLine->capacity >= rawLen + 1) {
                memcpy(curLine->text, lineStart, rawLen * sizeof(wchar_t));
                curLine->text[rawLen] = L'\0';
                curLine->length = rawLen;
            }
        } else {
            curLine->length = 0;
            if (curLine->text) curLine->text[0] = L'\0';
            for (size_t k = 0; k < rawLen; k++) {
                if (lineStart[k] == L'\t') {
                    size_t spaces = (size_t)tabW - (curLine->length % tabW);
                    if (spaces == 0) spaces = (size_t)tabW;
                    for (size_t sp = 0; sp < spaces; sp++) Line_InsertChar(curLine, curLine->length, L' ');
                } else {
                    Line_InsertChar(curLine, curLine->length, lineStart[k]);
                }
            }
        }

        BOOL hadTrailingNewline = FALSE;
        if (*p == L'\r' && *(p + 1) == L'\n') {
            p += 2;
            hadTrailingNewline = (*p == L'\0');
        } else if (*p == L'\r' || *p == L'\n') {
            p++;
            hadTrailingNewline = (*p == L'\0');
        }

        curRow++;

        if (hadTrailingNewline) {
            if (curRow >= s->lineCount) {
                Line_Init(&s->lines[curRow]);
                s->lineCount = curRow + 1;
            }
        }
    }
    
    if (s->lineCount < 1) {
        s->lineCount = 1;
        Line_Init(&s->lines[0]);
    }

    s->caret.row = 0;
    s->caret.col = 0;
    s->preferredCol = 0;
    ClearSelection(s);
    UpdateCodeBlocks(s);
    RecalculateMaxCols(s);
    RecalculateGutter(s);
    UpdateStatusBar(s);
}

static void Editor_ConvertIndentation(EditorState* s, int oldTabSize, int newTabSize) {
    if (oldTabSize <= 0) oldTabSize = 2;
    if (newTabSize <= 0) newTabSize = 2;
    if (oldTabSize == newTabSize) return;

    PushUndo(s);

    for (int r = 0; r < s->lineCount; r++) {
        Line* line = &s->lines[r];
        size_t wsLen = 0;
        int visualCols = 0;

        while (wsLen < line->length && (line->text[wsLen] == L' ' || line->text[wsLen] == L'\t')) {
            if (line->text[wsLen] == L'\t') visualCols += oldTabSize - (visualCols % oldTabSize);
            else visualCols++;
            wsLen++;
        }

        if (wsLen > 0) {
            int indentLevels = visualCols / oldTabSize;
            int remSpaces = visualCols % oldTabSize;
            size_t newWsLen = (size_t)(indentLevels * newTabSize + remSpaces);
            size_t tailLen = line->length - wsLen;

            wchar_t* tail = NULL;
            if (tailLen > 0) {
                tail = (wchar_t*)malloc(tailLen * sizeof(wchar_t));
                if (tail) memcpy(tail, &line->text[wsLen], tailLen * sizeof(wchar_t));
            }

            line->length = 0;
            line->text[0] = L'\0';

            for (size_t k = 0; k < newWsLen; k++) Line_InsertChar(line, k, L' ');

            if (tail) {
                for (size_t k = 0; k < tailLen; k++) Line_InsertChar(line, line->length, tail[k]);
                free(tail);
            }
        }
    }

    if (oldTabSize > 0) {
        int64_t caretLevels = s->caret.col / oldTabSize;
        int64_t caretRem = s->caret.col % oldTabSize;
        s->caret.col = (caretLevels * newTabSize) + caretRem;
        if ((size_t)s->caret.col > s->lines[s->caret.row].length) s->caret.col = (int64_t)s->lines[s->caret.row].length;
        s->preferredCol = s->caret.col;

        int64_t anchorLevels = s->selAnchor.col / oldTabSize;
        int64_t anchorRem = s->selAnchor.col % oldTabSize;
        s->selAnchor.col = (anchorLevels * newTabSize) + anchorRem;
        if ((size_t)s->selAnchor.col > s->lines[s->selAnchor.row].length) s->selAnchor.col = (int64_t)s->lines[s->selAnchor.row].length;
    }

    SetModified(s, TRUE);
    UpdateCodeBlocks(s);
    RecalculateMaxCols(s);
    RecalculateGutter(s);
    FindMatchingBrace(s);
    UpdateStatusBar(s);
}

static void Editor_MakeSelectionCase(EditorState* s, BOOL makeUpper) {
    if (!HasSelection(s)) return;
    PushUndo(s);

    TextPos start, end;
    GetSelectionRange(s, &start, &end);

    for (int r = start.row; r <= end.row; r++) {
        Line* line = &s->lines[r];
        size_t c1 = (r == start.row) ? (size_t)start.col : 0;
        size_t c2 = (r == end.row) ? (size_t)end.col : line->length;

        for (size_t c = c1; c < c2 && c < line->length; c++) {
            if (makeUpper) line->text[c] = (wchar_t)towupper(line->text[c]);
            else line->text[c] = (wchar_t)towlower(line->text[c]);
        }
    }

    SetModified(s, TRUE);
    InvalidateRect(g_hwndEditor, NULL, FALSE);
}

static void Editor_SplitLine(EditorState* s) {
    Line* cur = &s->lines[s->caret.row];
    if (!cur->text && cur->capacity == 0) Line_Init(cur);

    // 1. Calculate indentation levels
    size_t leadingSpaces = 0;
    while (cur->text && leadingSpaces < cur->length && cur->text[leadingSpaces] == L' ') leadingSpaces++;

    int64_t baseIndent = (int64_t)leadingSpaces;
    if (s->caret.col < baseIndent) baseIndent = s->caret.col;

    // Check if splitting after '{'
    int64_t checkIdx = s->caret.col - 1;
    while (cur->text && checkIdx >= 0 && cur->text[checkIdx] == L' ') checkIdx--;
    BOOL afterOpenBrace = (cur->text && checkIdx >= 0 && cur->text[checkIdx] == L'{');

    // Check if splitting before '}'
    int64_t tailCheck = s->caret.col;
    while (cur->text && (size_t)tailCheck < cur->length && cur->text[tailCheck] == L' ') tailCheck++;
    BOOL beforeCloseBrace = (cur->text && (size_t)tailCheck < cur->length && cur->text[tailCheck] == L'}');

    if (afterOpenBrace) baseIndent += (s->tabSize > 0 ? s->tabSize : 2);

    // 2. Extract tail characters to move to the next line
    size_t tailLen = ((size_t)s->caret.col <= cur->length) ? (cur->length - (size_t)s->caret.col) : 0;
    wchar_t* tailBuf = NULL;
    if (tailLen > 0 && cur->text) {
        tailBuf = (wchar_t*)malloc(tailLen * sizeof(wchar_t));
        if (tailBuf) memcpy(tailBuf, &cur->text[s->caret.col], tailLen * sizeof(wchar_t));
    }

    // 3. Snapshot document BEFORE any lines or buffers are modified
    PushUndo(s);

    // 4. Insert the new line (with transactional rollback on failure)
    if (!Editor_InsertLine(s, s->caret.row + 1)) {
        if (tailBuf) free(tailBuf);
        // Rollback undo push if memory allocation failed
        if (s->undoStack.count > 0) {
            SafeFreeDocText(s->undoStack.steps[--s->undoStack.count].text);
        }
        return;
    }

    // 5. Re-fetch pointers after Editor_InsertLine (prevents dangling pointer access)
    cur = &s->lines[s->caret.row];
    Line* next = &s->lines[s->caret.row + 1];

    if (afterOpenBrace && beforeCloseBrace) {
        // Expand: { \n    \n }
        if (Editor_InsertLine(s, s->caret.row + 2)) {
            cur = &s->lines[s->caret.row];
            next = &s->lines[s->caret.row + 1];
            Line* closingLine = &s->lines[s->caret.row + 2];
            for (size_t k = 0; k < leadingSpaces; k++) Line_InsertChar(closingLine, k, L' ');
            if (tailBuf) {
                for (size_t i = 0; i < tailLen; i++) Line_InsertChar(closingLine, closingLine->length, tailBuf[i]);
            }
            for (int64_t k = 0; k < baseIndent; k++) Line_InsertChar(next, (size_t)k, L' ');
        }
    } else {
        // Standard Enter
        for (int64_t k = 0; k < baseIndent; k++) Line_InsertChar(next, (size_t)k, L' ');
        size_t tailStart = 0;
        if ((size_t)s->caret.col > leadingSpaces && tailBuf) {
            while (tailStart < tailLen && tailBuf[tailStart] == L' ') tailStart++;
        }
        if (tailBuf && tailLen > tailStart) {
            for (size_t i = tailStart; i < tailLen; i++) Line_InsertChar(next, next->length, tailBuf[i]);
        }
    }

    if (tailBuf) free(tailBuf);

    // Truncate original line at caret position
    if (cur->text) {
        cur->length = ((size_t)s->caret.col <= cur->length) ? (size_t)s->caret.col : cur->length;
        cur->text[cur->length] = L'\0';
    }

    // Move caret to next line
    s->caret.row++;
    s->caret.col = baseIndent;
    s->preferredCol = s->caret.col;
    ClearSelection(s);
    RecalculateGutter(s);
    SetModified(s, TRUE);
}

static void Editor_DeleteSelection(EditorState* s) {
    if (!HasSelection(s)) return;
    PushUndo(s);

    TextPos start, end;
    GetSelectionRange(s, &start, &end);

    if (start.row == end.row) {
        Line_DeleteChars(&s->lines[start.row], (size_t)start.col, (size_t)(end.col - start.col));
    } else {
        Line* startLine = &s->lines[start.row];
        Line* endLine = &s->lines[end.row];

        // Safe truncation using Line_DeleteChars
        if ((size_t)start.col < startLine->length) {
            Line_DeleteChars(startLine, (size_t)start.col, startLine->length - (size_t)start.col);
        }

        // Append remainder of end line
        if ((size_t)end.col < endLine->length) {
            for (size_t i = (size_t)end.col; i < endLine->length; i++) {
                Line_InsertChar(startLine, startLine->length, endLine->text[i]);
            }
        }

        for (int r = end.row; r > start.row; r--) {
            Editor_DeleteLine(s, r);
        }
    }

    s->caret = start;
    s->preferredCol = s->caret.col;
    ClearSelection(s);
    RecalculateMaxCols(s);
    SetModified(s, TRUE);
}

static void Editor_Copy(HWND hwnd, EditorState* s) {
    if (!HasSelection(s)) return;
    TextPos start, end;
    GetSelectionRange(s, &start, &end);

    size_t totalLen = 0;
    for (int r = start.row; r <= end.row; r++) {
        size_t c1 = (r == start.row) ? (size_t)start.col : 0;
        size_t c2 = (r == end.row) ? (size_t)end.col : s->lines[r].length;
        totalLen += (c2 - c1);
        if (r < end.row) totalLen += 2;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (totalLen + 1) * sizeof(wchar_t));
    if (!hMem) return;

    wchar_t* cur = (wchar_t*)GlobalLock(hMem);
    for (int r = start.row; r <= end.row; r++) {
        size_t c1 = (r == start.row) ? (size_t)start.col : 0;
        size_t c2 = (r == end.row) ? (size_t)end.col : s->lines[r].length;
        size_t count = c2 - c1;
        if (count > 0 && s->lines[r].text) {
            memcpy(cur, &s->lines[r].text[c1], count * sizeof(wchar_t));
            cur += count;
        }
        if (r < end.row) {
            *cur++ = L'\r';
            *cur++ = L'\n';
        }
    }
    *cur = L'\0';
    GlobalUnlock(hMem);

    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hMem);
        CloseClipboard();
    }
}

static void Editor_Cut(HWND hwnd, EditorState* s) {
    if (!HasSelection(s)) return;
    Editor_Copy(hwnd, s);
    Editor_DeleteSelection(s);
    UpdateCodeBlocks(s);
    FindMatchingBrace(s);
    EnsureCaretVisible(hwnd, s);
    InvalidateRect(hwnd, NULL, FALSE);
}

static void Editor_Paste(HWND hwnd, EditorState* s) {
    if (!OpenClipboard(hwnd)) return;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) { CloseClipboard(); return; }

    const wchar_t* p = (const wchar_t*)GlobalLock(hData);
    if (p && *p) {
        if (HasSelection(s)) Editor_DeleteSelection(s);
        PushUndo(s);

        int tabWidth = (s->tabSize > 0) ? s->tabSize : 2;
        int curRow = s->caret.row;
        size_t curCol = (size_t)s->caret.col;

        Line* curLine = &s->lines[curRow];
        size_t tailLen = (curLine->length >= curCol) ? (curLine->length - curCol) : 0;
        wchar_t* tailBuf = NULL;
        if (tailLen > 0 && curLine->text) {
            tailBuf = (wchar_t*)malloc(tailLen * sizeof(wchar_t));
            if (tailBuf) memcpy(tailBuf, &curLine->text[curCol], tailLen * sizeof(wchar_t));
        }

        // Truncate current line at caret position
        curLine->length = curCol;
        if (curLine->text) curLine->text[curCol] = L'\0';

        const wchar_t* scan = p;
        BOOL isFirstLine = TRUE;

        while (*scan) {
            const wchar_t* lineStart = scan;
            while (*scan && *scan != L'\r' && *scan != L'\n') scan++;
            size_t segLen = (size_t)(scan - lineStart);

            Line* targetLine = NULL;
            if (isFirstLine) {
                targetLine = &s->lines[curRow];
                isFirstLine = FALSE;
            } else {
                curRow++;
                Editor_InsertLine(s, curRow);
                targetLine = &s->lines[curRow];
            }

            for (size_t i = 0; i < segLen; i++) {
                if (lineStart[i] == L'\t') {
                    int64_t cCol = targetLine->length;
                    int spacesToAdd = tabWidth - (int)(cCol % tabWidth);
                    if (spacesToAdd <= 0) spacesToAdd = tabWidth;
                    for (int sp = 0; sp < spacesToAdd; sp++) Line_InsertChar(targetLine, targetLine->length, L' ');
                } else {
                    Line_InsertChar(targetLine, targetLine->length, lineStart[i]);
                }
            }

            if (*scan == L'\r' && *(scan + 1) == L'\n') {
                scan += 2;
                if (*scan == L'\0') { // Ends with newline: open empty row
                    curRow++;
                    Editor_InsertLine(s, curRow);
                }
            } else if (*scan == L'\r' || *scan == L'\n') {
                scan++;
                if (*scan == L'\0') { // Ends with newline: open empty row
                    curRow++;
                    Editor_InsertLine(s, curRow);
                }
            }
        }

        s->caret.row = curRow;
        s->caret.col = (int64_t)s->lines[curRow].length;

        if (tailBuf) {
            for (size_t i = 0; i < tailLen; i++) Line_InsertChar(&s->lines[curRow], s->lines[curRow].length, tailBuf[i]);
            free(tailBuf);
        }

        s->preferredCol = s->caret.col;
        ClearSelection(s);
        SetModified(s, TRUE);
        GlobalUnlock(hData);
    }
    CloseClipboard();

    UpdateCodeBlocks(s);
    RecalculateMaxCols(s);
    RecalculateGutter(s);
    FindMatchingBrace(s);
    EnsureCaretVisibleEx(hwnd, s, 0.15f);
    UpdateScrollbars(hwnd, s);
    UpdateCaret(hwnd, s);
    InvalidateRect(hwnd, NULL, FALSE);
}

// Search Logic
static int FindStringInLine(const wchar_t* lineText, size_t lineLen, const wchar_t* pattern, size_t patLen, size_t startCol, BOOL matchCase, BOOL wholeWord, BOOL forward) {
    if (patLen == 0 || lineLen < patLen) return -1;

    if (forward) {
        if (startCol > lineLen - patLen) return -1;
        for (size_t c = startCol; c <= lineLen - patLen; c++) {
            BOOL match = matchCase ? (wcsncmp(&lineText[c], pattern, patLen) == 0) : (_wcsnicmp(&lineText[c], pattern, patLen) == 0);
            if (match && wholeWord) {
                if (c > 0 && IsIdentChar(lineText[c - 1])) match = FALSE;
                if (c + patLen < lineLen && IsIdentChar(lineText[c + patLen])) match = FALSE;
            }
            if (match) return (int)c;
        }
    } else {
        size_t maxStart = (startCol < lineLen - patLen) ? startCol : (lineLen - patLen);
        for (size_t c = maxStart + 1; c > 0; c--) {
            size_t idx = c - 1;
            BOOL match = matchCase ? (wcsncmp(&lineText[idx], pattern, patLen) == 0) : (_wcsnicmp(&lineText[idx], pattern, patLen) == 0);
            if (match && wholeWord) {
                if (idx > 0 && IsIdentChar(lineText[idx - 1])) match = FALSE;
                if (idx + patLen < lineLen && IsIdentChar(lineText[idx + patLen])) match = FALSE;
            }
            if (match) return (int)idx;
        }
    }
    return -1;
}

static BOOL FindNextText(EditorState* s, BOOL searchDown) {
    if (g_search.findText[0] == L'\0') {
        ShowFindDialog(s->hwndMain, s);
        return FALSE;
    }

    size_t patLen = wcslen(g_search.findText);
    TextPos start;
    if (HasSelection(s)) {
        TextPos selS, selE;
        GetSelectionRange(s, &selS, &selE);
        start = searchDown ? selE : selS;
    } else {
        start = s->caret;
    }

    int r = start.row;
    size_t c = (size_t)start.col;
    int wrapped = 0;

    while (wrapped <= 1) {
        if (searchDown) {
            while (r < s->lineCount) {
                int found = FindStringInLine(s->lines[r].text, s->lines[r].length, g_search.findText, patLen, c, g_search.matchCase, g_search.matchWholeWord, TRUE);
                if (found != -1) {
                    s->selAnchor.row = r; s->selAnchor.col = found;
                    s->caret.row = r; s->caret.col = found + patLen;
                    s->preferredCol = s->caret.col;
                    FindMatchingBrace(s);
                    EnsureCaretVisibleEx(g_hwndEditor, s, 0.15f);
                    InvalidateRect(g_hwndEditor, NULL, FALSE);
                    return TRUE;
                }
                r++;
                c = 0;
            }
            if (g_search.wrapAround && wrapped == 0) { r = 0; c = 0; wrapped++; }
            else break;
        } else {
            while (r >= 0) {
                size_t searchStartCol = (r == start.row && wrapped == 0 && c > 0) ? (c - 1) : ((s->lines[r].length >= patLen) ? s->lines[r].length - patLen : 0);
                int found = FindStringInLine(s->lines[r].text, s->lines[r].length, g_search.findText, patLen, searchStartCol, g_search.matchCase, g_search.matchWholeWord, FALSE);
                if (found != -1) {
                    s->selAnchor.row = r; s->selAnchor.col = found;
                    s->caret.row = r; s->caret.col = found + patLen;
                    s->preferredCol = s->caret.col;
                    FindMatchingBrace(s);
                    EnsureCaretVisibleEx(g_hwndEditor, s, 0.15f);
                    InvalidateRect(g_hwndEditor, NULL, FALSE);
                    return TRUE;
                }
                r--;
            }
            if (g_search.wrapAround && wrapped == 0) { r = s->lineCount - 1; c = s->lines[r].length; wrapped++; }
            else break;
        }
    }

    wchar_t msg[300];
    wsprintf(msg, L"Cannot find \"%s\"", g_search.findText);
    HWND hOwner = g_hFindDlg ? g_hFindDlg : (g_hReplaceDlg ? g_hReplaceDlg : s->hwndMain);
    MessageBoxW(hOwner, msg, L"Find", MB_OK | MB_ICONINFORMATION);
    return FALSE;
}

static int DetectEncodingIndex(const unsigned char* buf, DWORD size) {
    if (!buf || size == 0) return 0;
    if (size >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) return 2;
    if (size >= 2 && buf[0] == 0xFF && buf[1] == 0xFE) return 3;
    if (size >= 2 && buf[0] == 0xFE && buf[1] == 0xFF) return 4;
    if (size >= 4 && buf[1] == 0x00 && buf[3] == 0x00 && buf[0] != 0x00) return 3;
    if (size >= 4 && buf[0] == 0x00 && buf[2] == 0x00 && buf[1] != 0x00) return 4;

    DWORD i = 0;
    BOOL hasUtf8Multibyte = FALSE;
    BOOL invalidUtf8 = FALSE;

    while (i < size) {
        if (buf[i] <= 0x7F) i++;
        else if ((buf[i] & 0xE0) == 0xC0) {
            if (i + 1 < size && (buf[i + 1] & 0xC0) == 0x80 && (buf[i] & 0x1E) != 0) { hasUtf8Multibyte = TRUE; i += 2; }
            else { invalidUtf8 = TRUE; break; }
        } else if ((buf[i] & 0xF0) == 0xE0) {
            if (i + 2 < size && (buf[i + 1] & 0xC0) == 0x80 && (buf[i + 2] & 0xC0) == 0x80) { hasUtf8Multibyte = TRUE; i += 3; }
            else { invalidUtf8 = TRUE; break; }
        } else if ((buf[i] & 0xF8) == 0xF0) {
            if (i + 3 < size && (buf[i + 1] & 0xC0) == 0x80 && (buf[i + 2] & 0xC0) == 0x80 && (buf[i + 3] & 0xC0) == 0x80) { hasUtf8Multibyte = TRUE; i += 4; }
            else { invalidUtf8 = TRUE; break; }
        } else { invalidUtf8 = TRUE; break; }
    }

    if (!invalidUtf8 && hasUtf8Multibyte) return 1;
    if (!invalidUtf8) return 0;

    int count1251 = 0, count1251_runs = 0, count1251_lower = 0, curRun = 0;
    for (DWORD k = 0; k < size; k++) {
        unsigned char c = buf[k];
        if ((c >= 0xC0 && c <= 0xFF) || c == 0xA8 || c == 0xB8) {
            count1251++;
            if (c >= 0xE0 || c == 0xB8) count1251_lower++;
            curRun++;
            if (curRun >= 2) count1251_runs++;
        } else curRun = 0;
    }
    if (count1251 >= 3 && count1251_runs >= 2 && count1251_lower >= (count1251 / 2)) return 5;

    return 0;
}

static BOOL LoadFileContent(HWND hwnd, EditorState* s, const wchar_t* path) {
    wchar_t fullPath[MAX_PATH];
    if (GetFullPathNameW(path, MAX_PATH, fullPath, NULL) == 0) wcscpy_s(fullPath, MAX_PATH, path);

    HANDLE hFile = CreateFileW(fullPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        wchar_t errMsg[MAX_PATH + 64];
        wsprintf(errMsg, L"Could not open file:\n%s\n(Error code: %lu)", fullPath, GetLastError());
        MessageBoxW(hwnd ? hwnd : s->hwndMain, errMsg, L"Open Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    LARGE_INTEGER liSize;
    if (!GetFileSizeEx(hFile, &liSize) || liSize.QuadPart == 0) {
        CloseHandle(hFile);
        if (liSize.QuadPart == 0) {
            Editor_ClearAll(s);
            wcscpy_s(s->filePath, MAX_PATH, fullPath);
            const wchar_t* lastSlash = wcsrchr(fullPath, L'\\');
            wcscpy_s(s->fileName, MAX_PATH, lastSlash ? lastSlash + 1 : fullPath);
            SetModified(s, FALSE);
            UpdateTitle(s);
            UpdateScrollbars(hwnd, s);
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            return TRUE;
        }
        return FALSE;
    }

    size_t fileSize = (size_t)liSize.QuadPart;

    unsigned char probe[65536];
    DWORD probeRead = 0;
    ReadFile(hFile, probe, sizeof(probe), &probeRead, NULL);
    s->encodingIdx = DetectEncodingIndex(probe, probeRead);
    if (s->encodingIdx < 0 || s->encodingIdx >= ENCODING_COUNT) s->encodingIdx = 0;
    const EncodingDesc* enc = &g_encodings[s->encodingIdx];

    LARGE_INTEGER liZero = { 0 };
    SetFilePointerEx(hFile, liZero, NULL, FILE_BEGIN);

    size_t maxChars = (fileSize / (enc->codePage == 1200 || enc->codePage == 1201 ? 2 : 1)) + 4;
    wchar_t* wbuffer = (wchar_t*)malloc((maxChars + 2) * sizeof(wchar_t));
    if (!wbuffer) {
        CloseHandle(hFile);
        MessageBoxW(hwnd ? hwnd : s->hwndMain, L"Out of memory allocating document buffer.", L"Open Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    size_t wideOffset = 0;

    if (enc->codePage == 1200 || enc->codePage == 1201) {
        if (enc->bomType == 2 || enc->bomType == 3) {
            LARGE_INTEGER liOffset;
            liOffset.QuadPart = 2;
            SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN);
            fileSize -= 2;
        }

        const size_t CHUNK_BYTES = 64 * 1024 * 1024;
        size_t totalRead = 0;
        while (totalRead < fileSize) {
            DWORD toRead = (DWORD)((fileSize - totalRead > CHUNK_BYTES) ? CHUNK_BYTES : (fileSize - totalRead));
            DWORD chunkRead = 0;
            if (!ReadFile(hFile, (char*)wbuffer + totalRead, toRead, &chunkRead, NULL) || chunkRead == 0) break;
            totalRead += chunkRead;
        }
        wideOffset = totalRead / sizeof(wchar_t);

        if (enc->codePage == 1201) {
            for (size_t k = 0; k < wideOffset; k++) {
                wchar_t c = wbuffer[k];
                wbuffer[k] = (wchar_t)(((c & 0xFF) << 8) | ((c >> 8) & 0xFF));
            }
        }
    } else {
        if (enc->bomType == 1) {
            LARGE_INTEGER liOffset;
            liOffset.QuadPart = 3;
            SetFilePointerEx(hFile, liOffset, NULL, FILE_BEGIN);
            fileSize -= 3;
        }

        const size_t CHUNK_SIZE = 16 * 1024 * 1024;
        unsigned char* chunk = (unsigned char*)malloc(CHUNK_SIZE + 16);
        if (!chunk) {
            free(wbuffer);
            CloseHandle(hFile);
            MessageBoxW(hwnd ? hwnd : s->hwndMain, L"Out of memory allocating chunk buffer.", L"Open Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        size_t totalFileBytesProcessed = 0;
        while (totalFileBytesProcessed < fileSize) {
            DWORD toRead = (DWORD)((fileSize - totalFileBytesProcessed > CHUNK_SIZE) ? CHUNK_SIZE : (fileSize - totalFileBytesProcessed));
            DWORD bytesRead = 0;
            if (!ReadFile(hFile, chunk, toRead, &bytesRead, NULL) || bytesRead == 0) break;

            size_t validBytes = bytesRead;
            if (totalFileBytesProcessed + bytesRead < fileSize && (enc->codePage == 65001 || enc->bomType == 1)) {
                size_t lookback = (validBytes > 4) ? 4 : validBytes;
                for (size_t k = 0; k < lookback; k++) {
                    unsigned char b = chunk[validBytes - 1 - k];
                    if ((b & 0xC0) != 0x80) {
                        size_t seqLen = 1;
                        if ((b & 0xE0) == 0xC0) seqLen = 2;
                        else if ((b & 0xF0) == 0xE0) seqLen = 3;
                        else if ((b & 0xF8) == 0xF0) seqLen = 4;

                        size_t avail = k + 1;
                        if (avail < seqLen) {
                            validBytes -= avail;
                            LARGE_INTEGER liRewind;
                            liRewind.QuadPart = -(LONGLONG)avail;
                            SetFilePointerEx(hFile, liRewind, NULL, FILE_CURRENT);
                        }
                        break;
                    }
                }
            }

            int destCap = (int)CHUNK_SIZE;
            int chars = MultiByteToWideChar(enc->codePage, 0, (const char*)chunk, (int)validBytes, wbuffer + wideOffset, destCap);
            if (chars <= 0 && enc->codePage != CP_ACP) {
                chars = MultiByteToWideChar(CP_ACP, 0, (const char*)chunk, (int)validBytes, wbuffer + wideOffset, destCap);
            }
            if (chars > 0) wideOffset += (size_t)chars;
            totalFileBytesProcessed += validBytes;
        }

        free(chunk);
    }

    CloseHandle(hFile);
    wbuffer[wideOffset] = L'\0';

    Editor_SetText(s, wbuffer);
    free(wbuffer);

    UndoStack_Clear(&s->undoStack);
    UndoStack_Clear(&s->redoStack);

    wcscpy_s(s->filePath, MAX_PATH, fullPath);
    const wchar_t* lastSlash = wcsrchr(fullPath, L'\\');
    wcscpy_s(s->fileName, MAX_PATH, lastSlash ? lastSlash + 1 : fullPath);

    s->caret.row = 0;
    s->caret.col = 0;
    s->preferredCol = 0;
    ClearSelection(s);
    SetModified(s, FALSE);
    UpdateCodeBlocks(s);
    RecalculateMaxCols(s);
    RecalculateGutter(s);
    FindMatchingBrace(s);
    UpdateScrollbars(hwnd, s);
    UpdateCaret(hwnd, s);
    UpdateEncodingMenuCheck(s->hwndMain, s->encodingIdx);
    UpdateTitle(s);
    UpdateStatusBar(s);
    InvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);
    return TRUE;
}

static BOOL SaveFileContent(HWND hwnd, EditorState* s, const wchar_t* path) {
    if (!s || !path || path[0] == L'\0') return FALSE;

    wchar_t fullPath[MAX_PATH];
    if (GetFullPathNameW(path, MAX_PATH, fullPath, NULL) == 0) wcscpy_s(fullPath, MAX_PATH, path);

    wchar_t tempPath[MAX_PATH + 32];
    wsprintf(tempPath, L"%s.tmp_%lu", fullPath, GetCurrentProcessId());

    HANDLE hFile = CreateFileW(tempPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        hFile = CreateFileW(fullPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            if (err == ERROR_ACCESS_DENIED) {
                int choice = MessageBoxW(hwnd ? hwnd : s->hwndMain, 
                    L"Access Denied (Error 5).\n\nThe file is in a protected location or locked.\nWould you like to save it to a different location (Save As)?", 
                    L"Access Denied", MB_YESNO | MB_ICONWARNING);
                if (choice == IDYES) return DoFileSaveAs(s->hwndMain, s);
                return FALSE;
            }
            wchar_t errMsg[MAX_PATH + 64];
            wsprintf(errMsg, L"Failed to save file:\n%s\n(Error code: %lu)", fullPath, err);
            MessageBoxW(hwnd ? hwnd : s->hwndMain, errMsg, L"Save Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        tempPath[0] = L'\0';
    }

    if (s->encodingIdx < 0 || s->encodingIdx >= ENCODING_COUNT) s->encodingIdx = 0;
    const EncodingDesc* enc = &g_encodings[s->encodingIdx];
    DWORD written = 0;

    if (enc->bomType == 1) { const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF }; WriteFile(hFile, bom, 3, &written, NULL); }
    else if (enc->bomType == 2) { const unsigned char bom[2] = { 0xFF, 0xFE }; WriteFile(hFile, bom, 2, &written, NULL); }
    else if (enc->bomType == 3) { const unsigned char bom[2] = { 0xFE, 0xFF }; WriteFile(hFile, bom, 2, &written, NULL); }

    const size_t CHUNK_CAP = 1024 * 1024;
    char* chunkBuf = (char*)malloc(CHUNK_CAP + 65536);
    if (!chunkBuf) {
        CloseHandle(hFile);
        if (tempPath[0] != L'\0') DeleteFileW(tempPath);
        MessageBoxW(hwnd ? hwnd : s->hwndMain, L"Out of memory allocating save buffer.", L"Save Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    size_t chunkLen = 0;
    BOOL writeError = FALSE;

    for (int i = 0; i < s->lineCount; i++) {
        Line* line = &s->lines[i];
        size_t len = line->length;
        const wchar_t* pText = line->text;

        if (enc->codePage == 1200) {
            if (len > 0 && pText) {
                size_t bytes = len * sizeof(wchar_t);
                size_t srcOff = 0;
                while (srcOff < bytes) {
                    size_t toCopy = bytes - srcOff;
                    if (chunkLen + toCopy > CHUNK_CAP) toCopy = CHUNK_CAP - chunkLen;
                    memcpy(chunkBuf + chunkLen, (const char*)pText + srcOff, toCopy);
                    chunkLen += toCopy;
                    srcOff += toCopy;
                    if (chunkLen >= CHUNK_CAP) {
                        if (!WriteFile(hFile, chunkBuf, (DWORD)chunkLen, &written, NULL) || written != chunkLen) { writeError = TRUE; break; }
                        chunkLen = 0;
                    }
                }
            }
            if (i < s->lineCount - 1 && !writeError) {
                const wchar_t crlf[2] = { L'\r', L'\n' };
                memcpy(chunkBuf + chunkLen, crlf, 4);
                chunkLen += 4;
                if (chunkLen >= CHUNK_CAP) {
                    if (!WriteFile(hFile, chunkBuf, (DWORD)chunkLen, &written, NULL) || written != chunkLen) { writeError = TRUE; }
                    chunkLen = 0;
                }
            }
        } else if (enc->codePage == 1201) {
            if (len > 0 && pText) {
                for (size_t k = 0; k < len; k++) {
                    chunkBuf[chunkLen++] = (char)(pText[k] >> 8);
                    chunkBuf[chunkLen++] = (char)(pText[k] & 0xFF);
                    if (chunkLen >= CHUNK_CAP) {
                        if (!WriteFile(hFile, chunkBuf, (DWORD)chunkLen, &written, NULL) || written != chunkLen) { writeError = TRUE; break; }
                        chunkLen = 0;
                    }
                }
            }
            if (i < s->lineCount - 1 && !writeError) {
                chunkBuf[chunkLen++] = 0x00; chunkBuf[chunkLen++] = 0x0D;
                chunkBuf[chunkLen++] = 0x00; chunkBuf[chunkLen++] = 0x0A;
                if (chunkLen >= CHUNK_CAP) {
                    if (!WriteFile(hFile, chunkBuf, (DWORD)chunkLen, &written, NULL) || written != chunkLen) { writeError = TRUE; }
                    chunkLen = 0;
                }
            }
        } else {
            if (len > 0 && pText) {
                size_t srcOff = 0;
                while (srcOff < len) {
                    size_t slice = len - srcOff;
                    if (slice > 8192) slice = 8192;
                    int mbBytes = WideCharToMultiByte(enc->codePage, 0, pText + srcOff, (int)slice, chunkBuf + chunkLen, 65536, NULL, NULL);
                    if (mbBytes > 0) chunkLen += (size_t)mbBytes;
                    srcOff += slice;
                    if (chunkLen >= CHUNK_CAP) {
                        if (!WriteFile(hFile, chunkBuf, (DWORD)chunkLen, &written, NULL) || written != chunkLen) { writeError = TRUE; break; }
                        chunkLen = 0;
                    }
                }
            }
            if (i < s->lineCount - 1 && !writeError) {
                chunkBuf[chunkLen++] = '\r';
                chunkBuf[chunkLen++] = '\n';
                if (chunkLen >= CHUNK_CAP) {
                    if (!WriteFile(hFile, chunkBuf, (DWORD)chunkLen, &written, NULL) || written != chunkLen) { writeError = TRUE; }
                    chunkLen = 0;
                }
            }
        }
        if (writeError) break;
    }

    if (!writeError && chunkLen > 0) {
        if (!WriteFile(hFile, chunkBuf, (DWORD)chunkLen, &written, NULL) || written != chunkLen) {
            writeError = TRUE;
        }
    }

    free(chunkBuf);
    CloseHandle(hFile);

    if (writeError) {
        if (tempPath[0] != L'\0') DeleteFileW(tempPath);
        wchar_t errMsg[MAX_PATH + 64];
        wsprintf(errMsg, L"Disk write failed for:\n%s\n(Disk full or write error)", fullPath);
        MessageBoxW(hwnd ? hwnd : s->hwndMain, errMsg, L"Save Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (tempPath[0] != L'\0') {
        if (!MoveFileExW(tempPath, fullPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(fullPath);
            if (!MoveFileW(tempPath, fullPath)) {
                wchar_t errMsg[MAX_PATH + 64];
                wsprintf(errMsg, L"Failed to finalize file:\n%s\nTemporary backup preserved at:\n%s", fullPath, tempPath);
                MessageBoxW(hwnd ? hwnd : s->hwndMain, errMsg, L"Save Error", MB_OK | MB_ICONERROR);
                return FALSE;
            }
        }
    }

    wcscpy_s(s->filePath, MAX_PATH, fullPath);
    const wchar_t* lastSlash = wcsrchr(fullPath, L'\\');
    wcscpy_s(s->fileName, MAX_PATH, lastSlash ? lastSlash + 1 : fullPath);
    SetModified(s, FALSE);
    UpdateTitle(s);
    UpdateStatusBar(s);
    return TRUE;
}

static UINT_PTR CALLBACK SaveHookProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)wParam; //suppress warning
    switch (msg) {
    case WM_INITDIALOG: break;
    case WM_NOTIFY: {
        OFNOTIFYW* pOfn = (OFNOTIFYW*)lParam;
        if (pOfn->hdr.code == CDN_INITDONE) {
            HWND hParent = GetParent(hdlg);
            HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            HWND hFileTypeCombo = GetDlgItem(hParent, 0x0470);
            HWND hFileTypeStatic = GetDlgItem(hParent, 0x0441);

            RECT rcCombo, rcStatic;
            ZeroMemory(&rcCombo, sizeof(rcCombo));
            ZeroMemory(&rcStatic, sizeof(rcStatic));
            int posX = 80, posY = 0, width = 240;

            if (hFileTypeCombo) {
                GetWindowRect(hFileTypeCombo, &rcCombo);
                MapWindowPoints(NULL, hParent, (LPPOINT)&rcCombo, 2);
                posX = rcCombo.left;
                posY = rcCombo.bottom + 8;
                width = rcCombo.right - rcCombo.left;
            }

            int staticX = 20, staticY = posY + 3, staticW = 60;
            if (hFileTypeStatic) {
                GetWindowRect(hFileTypeStatic, &rcStatic);
                MapWindowPoints(NULL, hParent, (LPPOINT)&rcStatic, 2);
                staticX = rcStatic.left;
                staticW = rcStatic.right - rcStatic.left;
            }

            RECT rcParent;
            GetWindowRect(hParent, &rcParent);
            SetWindowPos(hParent, NULL, 0, 0, rcParent.right - rcParent.left, rcParent.bottom - rcParent.top + 35,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

            HWND hStatic = CreateWindowExW(0, L"STATIC", L"Encoding:",
                WS_CHILD | WS_VISIBLE | SS_RIGHT, staticX, staticY, staticW, 20,
                hParent, (HMENU)IDC_SAVE_ENC_STATIC, GetModuleHandle(NULL), NULL);
            SendMessage(hStatic, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

            g_hDlgCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                posX, posY, width, 260, hParent, (HMENU)IDC_SAVE_ENC_COMBO, GetModuleHandle(NULL), NULL);
            SendMessage(g_hDlgCombo, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

            for (int i = 0; i < ENCODING_COUNT; i++) SendMessageW(g_hDlgCombo, CB_ADDSTRING, 0, (LPARAM)g_encodings[i].name);
            SendMessage(g_hDlgCombo, CB_SETCURSEL, g_selectedDlgEncoding, 0);
        } else if (pOfn->hdr.code == CDN_FILEOK) {
            if (g_hDlgCombo) g_selectedDlgEncoding = (int)SendMessage(g_hDlgCombo, CB_GETCURSEL, 0, 0);
        }
        break;
    }
    }
    return 0;
}

static BOOL DoFileSaveAs(HWND hwndMain, EditorState* s) {
    wchar_t filename[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndMain;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0C/C++ Files (*.c;*.cpp;*.cxx;*.h;*.hpp)\0*.c;*.cpp;*.cxx;*.h;*.hpp\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
    ofn.lpfnHook = SaveHookProc;

    g_selectedDlgEncoding = s->encodingIdx;

    if (GetSaveFileNameW(&ofn)) {
        s->encodingIdx = g_selectedDlgEncoding;
        UpdateEncodingMenuCheck(hwndMain, s->encodingIdx);
        return SaveFileContent(hwndMain, s, filename);
    }
    return FALSE;
}

static BOOL PromptSaveIfModified(HWND hwndMain, EditorState* s) {
    if (!s->isModified) return TRUE;

    wchar_t msg[MAX_PATH + 64];
    wsprintf(msg, L"Save changes to \"%s\"?", s->fileName[0] != L'\0' ? s->fileName : L"Untitled");
    int res = MessageBoxW(hwndMain, msg, L"Editor", MB_YESNOCANCEL | MB_ICONQUESTION);

    if (res == IDYES) {
        if (s->filePath[0] != L'\0') return SaveFileContent(hwndMain, s, s->filePath);
        else return DoFileSaveAs(hwndMain, s);
    } else if (res == IDNO) return TRUE;
    return FALSE;
}

static void GetIniPath(wchar_t* path, size_t maxLen) {
    GetModuleFileNameW(NULL, path, (DWORD)maxLen);
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    wcscat_s(path, maxLen, L"codepad.ini");
}

static void SaveColorToIni(const wchar_t* section, const wchar_t* key, COLORREF clr, const wchar_t* path) {
    wchar_t buf[32];
    wsprintf(buf, L"0x%06X", clr);
    WritePrivateProfileStringW(section, key, buf, path);
}

static COLORREF LoadColorFromIni(const wchar_t* section, const wchar_t* key, COLORREF defClr, const wchar_t* path) {
    wchar_t buf[32];
    GetPrivateProfileStringW(section, key, L"", buf, 32, path);
    if (buf[0] != L'\0') return (COLORREF)wcstoul(buf, NULL, 0);
    return defClr;
}

static void SaveThemeSection(const wchar_t* section, const Theme* t, const wchar_t* iniPath) {
    SaveColorToIni(section, L"Background", t->bg, iniPath);
    SaveColorToIni(section, L"Text", t->text, iniPath);
    SaveColorToIni(section, L"Keywords", t->keyword, iniPath);
    SaveColorToIni(section, L"Preprocessor", t->preprocessor, iniPath);
    SaveColorToIni(section, L"Comments", t->comment, iniPath);
    SaveColorToIni(section, L"Strings", t->string, iniPath);
    SaveColorToIni(section, L"Numbers", t->number, iniPath);
    SaveColorToIni(section, L"IndentGuide", t->indentGuide, iniPath);
    SaveColorToIni(section, L"ActiveIndent", t->activeIndentGuide, iniPath);
    SaveColorToIni(section, L"BraceMatchBorder", t->braceMatchBorder, iniPath);
    SaveColorToIni(section, L"BraceMatchBg", t->braceMatchBg, iniPath);
    SaveColorToIni(section, L"Selection", t->selection, iniPath);
    SaveColorToIni(section, L"GutterBg", t->gutterBg, iniPath);
    SaveColorToIni(section, L"GutterText", t->gutterText, iniPath);
    SaveColorToIni(section, L"GutterLine", t->gutterLine, iniPath);
}

static void LoadThemeSection(const wchar_t* section, Theme* t, const wchar_t* iniPath) {
    t->bg = LoadColorFromIni(section, L"Background", t->bg, iniPath);
    t->text = LoadColorFromIni(section, L"Text", t->text, iniPath);
    t->keyword = LoadColorFromIni(section, L"Keywords", t->keyword, iniPath);
    t->preprocessor = LoadColorFromIni(section, L"Preprocessor", t->preprocessor, iniPath);
    t->comment = LoadColorFromIni(section, L"Comments", t->comment, iniPath);
    t->string = LoadColorFromIni(section, L"Strings", t->string, iniPath);
    t->number = LoadColorFromIni(section, L"Numbers", t->number, iniPath);
    t->indentGuide = LoadColorFromIni(section, L"IndentGuide", t->indentGuide, iniPath);
    t->activeIndentGuide = LoadColorFromIni(section, L"ActiveIndent", t->activeIndentGuide, iniPath);
    t->braceMatchBorder = LoadColorFromIni(section, L"BraceMatchBorder", t->braceMatchBorder, iniPath);
    t->braceMatchBg = LoadColorFromIni(section, L"BraceMatchBg", t->braceMatchBg, iniPath);
    t->selection = LoadColorFromIni(section, L"Selection", t->selection, iniPath);
    t->gutterBg = LoadColorFromIni(section, L"GutterBg", t->gutterBg, iniPath);
    t->gutterText = LoadColorFromIni(section, L"GutterText", t->gutterText, iniPath);
    t->gutterLine = LoadColorFromIni(section, L"GutterLine", t->gutterLine, iniPath);
}

static void SaveSettingsToIni(HWND hwndMain, EditorState* s) {
    wchar_t iniPath[MAX_PATH];
    GetIniPath(iniPath, MAX_PATH);

    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (g_isFullscreen) wp = g_prevPlacement;
    else GetWindowPlacement(hwndMain, &wp);

    wchar_t buf[64];
    wsprintf(buf, L"%d", wp.rcNormalPosition.left);   WritePrivateProfileStringW(L"Window", L"Left", buf, iniPath);
    wsprintf(buf, L"%d", wp.rcNormalPosition.top);    WritePrivateProfileStringW(L"Window", L"Top", buf, iniPath);
    wsprintf(buf, L"%d", wp.rcNormalPosition.right - wp.rcNormalPosition.left);  WritePrivateProfileStringW(L"Window", L"Width", buf, iniPath);
    wsprintf(buf, L"%d", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top); WritePrivateProfileStringW(L"Window", L"Height", buf, iniPath);
    wsprintf(buf, L"%d", (wp.showCmd == SW_SHOWMAXIMIZED) ? 1 : 0); WritePrivateProfileStringW(L"Window", L"Maximized", buf, iniPath);

    wsprintf(buf, L"%d", s->isDark ? 1 : 0); WritePrivateProfileStringW(L"Editor", L"IsDark", buf, iniPath);
    wsprintf(buf, L"%d", s->showIndentGuides ? 1 : 0);      WritePrivateProfileStringW(L"View", L"ShowGuides", buf, iniPath);
    wsprintf(buf, L"%d", s->highlightIndentGuides ? 1 : 0); WritePrivateProfileStringW(L"View", L"HighlightGuides", buf, iniPath);
    wsprintf(buf, L"%d", s->showBraceMatch ? 1 : 0);   WritePrivateProfileStringW(L"View", L"ShowBraceMatch", buf, iniPath);
    wsprintf(buf, L"%d", s->showLineNumbers ? 1 : 0);  WritePrivateProfileStringW(L"View", L"ShowLineNumbers", buf, iniPath);
    wsprintf(buf, L"%d", s->showStatusBar ? 1 : 0);    WritePrivateProfileStringW(L"View", L"ShowStatusBar", buf, iniPath);
    wsprintf(buf, L"%d", s->tabSize);                  WritePrivateProfileStringW(L"Editor", L"TabSize", buf, iniPath);
    wsprintf(buf, L"%d", s->useSpaces ? 1 : 0);        WritePrivateProfileStringW(L"Editor", L"UseSpaces", buf, iniPath);

    wsprintf(buf, L"%d", s->pointSize); WritePrivateProfileStringW(L"Font", L"Size", buf, iniPath);
    wsprintf(buf, L"%d", s->fontBold ? 1 : 0); WritePrivateProfileStringW(L"Font", L"Bold", buf, iniPath);
    wsprintf(buf, L"%d", s->fontQuality); WritePrivateProfileStringW(L"Font", L"Quality", buf, iniPath);
    WritePrivateProfileStringW(L"Font", L"Face", s->fontFace, iniPath);

    WritePrivateProfileStringW(L"Syntax", L"Keywords", g_keywordsBuffer, iniPath);
    WritePrivateProfileStringW(L"Syntax", L"Preprocessor", g_preprocBuffer, iniPath);

    // Find in Files settings
    WritePrivateProfileStringW(L"FindInFiles", L"Patterns", g_grepPatterns, iniPath);
    WritePrivateProfileStringW(L"FindInFiles", L"Directory", g_grepDirectory, iniPath);
    wsprintf(buf, L"%d", g_splitterHeight);
    WritePrivateProfileStringW(L"FindInFiles", L"SplitterHeight", buf, iniPath);

    SaveThemeSection(L"DarkTheme", &g_darkTheme, iniPath);
    SaveThemeSection(L"LightTheme", &g_lightTheme, iniPath);
}

static void LoadSettingsFromIni(EditorState* s) {
    wchar_t iniPath[MAX_PATH];
    GetIniPath(iniPath, MAX_PATH);

    s->isDark = GetPrivateProfileIntW(L"Editor", L"IsDark", 1, iniPath);
    s->showIndentGuides = GetPrivateProfileIntW(L"View", L"ShowGuides", 1, iniPath);
    s->highlightIndentGuides = GetPrivateProfileIntW(L"View", L"HighlightGuides", 1, iniPath);
    s->showBraceMatch = GetPrivateProfileIntW(L"View", L"ShowBraceMatch", 1, iniPath);
    s->showLineNumbers = GetPrivateProfileIntW(L"View", L"ShowLineNumbers", 1, iniPath);
    s->showStatusBar = GetPrivateProfileIntW(L"View", L"ShowStatusBar", 0, iniPath);
    s->tabSize = GetPrivateProfileIntW(L"Editor", L"TabSize", 2, iniPath);
    s->useSpaces = GetPrivateProfileIntW(L"Editor", L"UseSpaces", 1, iniPath);
    if (s->tabSize != 2 && s->tabSize != 4 && s->tabSize != 8) s->tabSize = 2;

    s->pointSize = GetPrivateProfileIntW(L"Font", L"Size", 10, iniPath);
    s->fontBold = GetPrivateProfileIntW(L"Font", L"Bold", 1, iniPath);
    s->fontQuality = GetPrivateProfileIntW(L"Font", L"Quality", CLEARTYPE_QUALITY, iniPath);
    GetPrivateProfileStringW(L"Font", L"Face", L"Courier New", s->fontFace, 64, iniPath);

    GetPrivateProfileStringW(L"Syntax", L"Keywords", g_keywordsBuffer, g_keywordsBuffer, 4096, iniPath);
    GetPrivateProfileStringW(L"Syntax", L"Preprocessor", g_preprocBuffer, g_preprocBuffer, 2048, iniPath);

    // Find in Files settings
    GetPrivateProfileStringW(L"FindInFiles", L"Patterns", L"*.c *.cpp *.h *.asm *.inc *.cxx *.pas", g_grepPatterns, 512, iniPath);
    GetPrivateProfileStringW(L"FindInFiles", L"Directory", L"", g_grepDirectory, MAX_PATH, iniPath);
    g_splitterHeight = GetPrivateProfileIntW(L"FindInFiles", L"SplitterHeight", -1, iniPath);

    LoadThemeSection(L"DarkTheme", &g_darkTheme, iniPath);
    LoadThemeSection(L"LightTheme", &g_lightTheme, iniPath);
}

static BOOL IsCharInQuoteOrComment(EditorState* s, int r, int64_t c) {
    if (!s || r < 0 || r >= s->lineCount) return FALSE;
    Line* line = &s->lines[r];
    if (!line || !line->text || line->length == 0 || c < 0 || (size_t)c >= line->length) return FALSE;

    size_t len = line->length;
    size_t targetCol = (size_t)c;
    BOOL inBlockComment = line->inComment;

    // Optimization: Localize scan window for large lines
    size_t startScan = (targetCol > 2048) ? (targetCol - 2048) : 0;
    size_t i = startScan;

    while (i < len) {
        if (inBlockComment) {
            size_t start = i;
            while (i < len) {
                if (line->text[i] == L'*' && (i + 1 < len) && line->text[i + 1] == L'/') {
                    i += 2; inBlockComment = FALSE; break;
                }
                i++;
            }
            if (targetCol >= start && targetCol < i) return TRUE;
            continue;
        }

        wchar_t ch = line->text[i];
        if (ch == L'/' && (i + 1 < len) && line->text[i + 1] == L'/') return (targetCol >= i);
        else if (ch == L'/' && (i + 1 < len) && line->text[i + 1] == L'*') {
            size_t start = i; i += 2; inBlockComment = TRUE;
            while (i < len) {
                if (line->text[i] == L'*' && (i + 1 < len) && line->text[i + 1] == L'/') {
                    i += 2; inBlockComment = FALSE; break;
                }
                i++;
            }
            if (targetCol >= start && targetCol < i) return TRUE;
        } else if (ch == L'"' || ch == L'\'') {
            wchar_t q = ch; size_t start = i++;
            while (i < len && line->text[i] != q) {
                if (line->text[i] == L'\\' && (i + 1 < len)) i++;
                i++;
            }
            if (i < len) i++;
            if (targetCol >= start && targetCol < i) return TRUE;
        } else {
            if (i == targetCol) return FALSE;
            i++;
        }

        // If we passed the target column and are outside any string/comment, return FALSE immediately
        if (i > targetCol + 100) return FALSE;
    }
    return FALSE;
}

static void FindMatchingBrace(EditorState* s) {
    s->hasBraceMatch = FALSE;
    if (!s->showBraceMatch || s->lineCount <= 0) return;

    TextPos pos = s->caret;
    if (pos.row < 0 || pos.row >= s->lineCount) return;
    Line* curLine = &s->lines[pos.row];
    if (!curLine || !curLine->text) return;

    wchar_t ch = L'\0';
    if ((size_t)pos.col < curLine->length) {
        ch = curLine->text[pos.col];
    }
    if (!(ch == L'(' || ch == L')' || ch == L'{' || ch == L'}' || ch == L'[' || ch == L']')) {
        if (pos.col > 0 && (size_t)(pos.col - 1) < curLine->length) {
            pos.col--;
            ch = curLine->text[pos.col];
        } else {
            return;
        }
    }

    wchar_t matchCh = L'\0';
    int dir = 0;
    if (ch == L'(') { matchCh = L')'; dir = 1; }
    else if (ch == L'[') { matchCh = L']'; dir = 1; }
    else if (ch == L'{') { matchCh = L'}'; dir = 1; }
    else if (ch == L')') { matchCh = L'('; dir = -1; }
    else if (ch == L']') { matchCh = L'['; dir = -1; }
    else if (ch == L'}') { matchCh = L'{'; dir = -1; }
    else return;

    if (IsCharInQuoteOrComment(s, pos.row, pos.col)) return;

    int depth = 1;
    int r = pos.row;
    int64_t c = pos.col + dir;

    int minRow = (pos.row - 2000 > 0) ? pos.row - 2000 : 0;
    int maxRow = (pos.row + 2000 < s->lineCount) ? pos.row + 2000 : s->lineCount - 1;

    while (r >= minRow && r <= maxRow) {
        Line* line = &s->lines[r];
        if (!line->text || line->length == 0) {
            r += dir;
            if (r >= minRow && r <= maxRow) c = (dir == 1) ? 0 : (int64_t)s->lines[r].length - 1;
            continue;
        }
        
        int64_t maxScanCols = 2000;
        int64_t startCol = c;
        int64_t endCol = (dir == 1) ? (startCol + maxScanCols) : (startCol - maxScanCols);
        if (endCol < 0) endCol = 0;
        if ((size_t)endCol > line->length) endCol = (int64_t)line->length;

        while ((dir == 1 && c < endCol && (size_t)c < line->length) ||
               (dir == -1 && c >= endCol && c >= 0 && (size_t)c < line->length)) {
            if (!IsCharInQuoteOrComment(s, r, c)) {
                wchar_t cur = line->text[c];
                if (cur == ch) depth++;
                else if (cur == matchCh) {
                    depth--;
                    if (depth == 0) {
                        s->hasBraceMatch = TRUE;
                        s->brace1 = pos;
                        s->brace2.row = r;
                        s->brace2.col = c;
                        return;
                    }
                }
            }
            c += dir;
        }
        r += dir;
        if (r >= minRow && r <= maxRow) c = (dir == 1) ? 0 : (int64_t)s->lines[r].length - 1;
    }
}

// -------------------------------------------------------------
// Screen & Caret Coordinate System (Immune to Integer Overflow)
// -------------------------------------------------------------
static TextPos ScreenToTextPos(EditorState* s, int x, int y) {
    TextPos pos;
    pos.row = (y + s->scrollY) / s->charHeight;
    if (pos.row < 0) pos.row = 0;
    if (pos.row >= s->lineCount) pos.row = s->lineCount - 1;

    int xOffset = s->gutterWidth + TEXT_MARGIN_X;
    int relativeX = x - xOffset + (s->charWidth / 2);
    if (relativeX < 0) relativeX = 0;
    
    pos.col = (int64_t)(relativeX / s->charWidth) + s->scrollX;
    if ((size_t)pos.col > s->lines[pos.row].length) {
        pos.col = (int64_t)s->lines[pos.row].length;
    }
    return pos;
}

static void UpdateStatusBar(EditorState* s) {
    if (!s || !s->hwndStatus || !IsWindow(s->hwndStatus)) return;
    if (!s->showStatusBar) return;

    unsigned long long charOffset = 1;
    for (int r = 0; r < s->caret.row && r < s->lineCount; r++) {
        charOffset += (unsigned long long)s->lines[r].length + 2;
    }
    charOffset += (unsigned long long)s->caret.col;

    unsigned long long totalChars = 0;
    for (int r = 0; r < s->lineCount; r++) {
        totalChars += (unsigned long long)s->lines[r].length;
        if (r < s->lineCount - 1) totalChars += 2;
    }

    wchar_t buf1[128], buf2[128];
    swprintf_s(buf1, 128, L"Ln %d, Col %lld, Char %llu", s->caret.row + 1, (long long)(s->caret.col + 1), charOffset);
    swprintf_s(buf2, 128, L"Lines: %d, Chars: %llu", s->lineCount, totalChars);

    SendMessageW(s->hwndStatus, SB_SETTEXT, 0, (LPARAM)buf1);
    SendMessageW(s->hwndStatus, SB_SETTEXT, 1, (LPARAM)buf2);
    SendMessageW(s->hwndStatus, SB_SETTEXT, 2, (LPARAM)g_encodings[s->encodingIdx].name);
}

static void UpdateCaret(HWND hwnd, EditorState* s) {
    if (GetFocus() == hwnd) {
        TextPos targetPos = s->isDraggingText ? s->dragDropPos : s->caret;
        int64_t relCol = targetPos.col - s->scrollX;
        int x = (int)(relCol * s->charWidth) + s->gutterWidth + TEXT_MARGIN_X;
        int y = (targetPos.row * s->charHeight) - s->scrollY;

        RECT rc;
        GetClientRect(hwnd, &rc);
        if (x >= s->gutterWidth && x <= rc.right && y >= 0 && y <= rc.bottom) {
            SetCaretPos(x, y);
        } else {
            SetCaretPos(-2000, -2000);
        }
    }
    UpdateStatusBar(s);
}

// -------------------------------------------------------------
// Scaled & Virtualized Scrollbars
// -------------------------------------------------------------
static void UpdateScrollbars(HWND hwnd, EditorState* s) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientW = rc.right - (s->gutterWidth + TEXT_MARGIN_X);
    int clientH = rc.bottom;

    s->visibleLines = (clientH > 0 && s->charHeight > 0) ? (clientH / s->charHeight) : 1;

    int charW = (s->charWidth > 0) ? s->charWidth : 8;
    int charH = (s->charHeight > 0) ? s->charHeight : 18;

    // 1. Vertical Scrollbar
    SCROLLINFO siVert;
    ZeroMemory(&siVert, sizeof(siVert));
    siVert.cbSize = sizeof(siVert);
    siVert.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;

    int totalLines = s->lineCount;
    int curLinePos = (charH > 0) ? (s->scrollY / charH) : 0;

    if (totalLines <= MAX_SCROLL_UNITS) {
        siVert.nMin = 0;
        siVert.nMax = (totalLines > 0) ? totalLines : 1;
        siVert.nPage = (UINT)s->visibleLines;
        siVert.nPos = curLinePos;
    } else {
        siVert.nMin = 0;
        siVert.nMax = MAX_SCROLL_UNITS;
        int pageUnits = (int)((double)s->visibleLines * (double)MAX_SCROLL_UNITS / (double)totalLines);
        siVert.nPage = (pageUnits > 0) ? (UINT)pageUnits : 1;
        siVert.nPos = (int)((double)curLinePos * (double)MAX_SCROLL_UNITS / (double)totalLines);
    }
    SetScrollInfo(hwnd, SB_VERT, &siVert, TRUE);

    // 2. Horizontal Scrollbar (64-bit Virtualized)
    SCROLLINFO siHorz;
    ZeroMemory(&siHorz, sizeof(siHorz));
    siHorz.cbSize = sizeof(siHorz);
    siHorz.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;

    int64_t totalCols = (int64_t)s->maxCols + 5;
    int64_t curColPos = s->scrollX;
    int64_t visCols = (clientW > 0 && charW > 0) ? (clientW / charW) : 1;
    if (visCols <= 0) visCols = 1;

    int64_t maxColPos = totalCols - visCols;
    if (maxColPos < 0) maxColPos = 0;

    if (totalCols <= MAX_SCROLL_UNITS) {
        siHorz.nMin = 0;
        siHorz.nMax = (int)totalCols;
        siHorz.nPage = (UINT)visCols;
        siHorz.nPos = (int)curColPos;
    } else {
        siHorz.nMin = 0;
        siHorz.nMax = MAX_SCROLL_UNITS;
        
        int pageUnits = (int)((double)visCols * (double)MAX_SCROLL_UNITS / (double)totalCols);
        if (pageUnits < 300) pageUnits = 300;
        if (pageUnits > MAX_SCROLL_UNITS / 2) pageUnits = MAX_SCROLL_UNITS / 2;
        siHorz.nPage = (UINT)pageUnits;

        double trackRange = (double)(MAX_SCROLL_UNITS - pageUnits + 1);
        if (trackRange <= 0.0) trackRange = 1.0;

        if (maxColPos > 0) {
            siHorz.nPos = (int)((double)curColPos * trackRange / (double)maxColPos);
        } else {
            siHorz.nPos = 0;
        }
    }
    SetScrollInfo(hwnd, SB_HORZ, &siHorz, TRUE);
}

static void EnsureCaretVisible(HWND hwnd, EditorState* s) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientW = rc.right - (s->gutterWidth + TEXT_MARGIN_X);
    int clientH = rc.bottom;

    int64_t targetCol = s->isDraggingText ? s->dragDropPos.col : s->caret.col;
    int targetRow = s->isDraggingText ? s->dragDropPos.row : s->caret.row;
    int charW = (s->charWidth > 0) ? s->charWidth : 8;
    int charH = (s->charHeight > 0) ? s->charHeight : 18;

    if (clientW > 0 && charW > 0) {
        int64_t visCols = clientW / charW;
        if (targetCol < s->scrollX) {
            s->scrollX = targetCol;
        } else if (targetCol >= s->scrollX + visCols) {
            s->scrollX = targetCol - visCols + 3;
        }

        int64_t maxScrollX = (int64_t)s->maxCols - visCols + 5;
        if (maxScrollX < 0) maxScrollX = 0;
        if (s->scrollX < 0) s->scrollX = 0;
        if (s->scrollX > maxScrollX) s->scrollX = maxScrollX;
    }

    if (clientH > 0 && charH > 0) {
        int visibleLines = clientH / charH;
        int bottomMargin = (visibleLines > 3) ? (2 * charH) : 0;

        int caretY = targetRow * charH;
        if (caretY < s->scrollY) {
            s->scrollY = caretY;
        } else if (caretY + charH + bottomMargin > s->scrollY + clientH) {
            s->scrollY = caretY + charH + bottomMargin - clientH;
        }

        int maxScrollY = (s->lineCount - visibleLines) * charH;
        if (maxScrollY < 0) maxScrollY = 0;
        if (s->scrollY < 0) s->scrollY = 0;
        if (s->scrollY > maxScrollY) s->scrollY = maxScrollY;
    }

    UpdateScrollbars(hwnd, s);
    UpdateCaret(hwnd, s);
}

static void EnsureCaretVisibleEx(HWND hwnd, EditorState* s, float marginRatio) {
    (void)marginRatio;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int clientW = rc.right - (s->gutterWidth + TEXT_MARGIN_X);
    int clientH = rc.bottom;
    if (clientH <= 0 || s->charHeight <= 0) return;

    int visibleLines = clientH / s->charHeight;
    int marginLines = (visibleLines > 3) ? 2 : 0;

    int curTopLine = s->scrollY / s->charHeight;
    int curBottomLine = curTopLine + visibleLines - 1;
    int targetRow = s->caret.row;

    if (targetRow < curTopLine + marginLines) {
        s->scrollY = (targetRow - marginLines) * s->charHeight;
    } else if (targetRow > curBottomLine - marginLines) {
        s->scrollY = (targetRow - visibleLines + 1 + marginLines) * s->charHeight;
    }

    int maxScroll = (s->lineCount - visibleLines) * s->charHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (s->scrollY < 0) s->scrollY = 0;
    if (s->scrollY > maxScroll) s->scrollY = maxScroll;

    int charW = (s->charWidth > 0) ? s->charWidth : 8;
    if (clientW > 0 && charW > 0) {
        int64_t visCols = clientW / charW;
        if (s->caret.col < s->scrollX) {
            s->scrollX = s->caret.col;
        } else if (s->caret.col >= s->scrollX + visCols) {
            s->scrollX = s->caret.col - visCols + 3;
        }
        int64_t maxScrollX = s->maxCols - visCols + 5;
        if (maxScrollX < 0) maxScrollX = 0;
        if (s->scrollX < 0) s->scrollX = 0;
        if (s->scrollX > maxScrollX) s->scrollX = maxScrollX;
    }

    UpdateScrollbars(hwnd, s);
    UpdateCaret(hwnd, s);
}

static void UpdateCodeBlocks(EditorState* s) {
    s->blockCount = 0;
    if (s->lineCount > CODEBLOCKS_MAX_LINES) return;

    typedef struct {
        int row;
        int64_t col;
        int guideCol;
    } BlockStackItem;

    static BlockStackItem stack[4096];
    int stackTop = 0;
    BOOL inBlockComment = FALSE;
    int tabW = (s->tabSize > 0) ? s->tabSize : 2;

    for (int r = 0; r < s->lineCount; r++) {
        s->lines[r].inComment = inBlockComment;
        Line* line = &s->lines[r];
        size_t len = line->length;
        if (!line->text || len == 0) continue;
        if (len > 50000) continue;

        int openIndent = 0;
        while ((size_t)openIndent < len && line->text[openIndent] == L' ') openIndent++;

        size_t i = 0;
        while (i < len) {
            if (inBlockComment) {
                if (line->text[i] == L'*' && (i + 1 < len) && line->text[i + 1] == L'/') {
                    inBlockComment = FALSE; i += 2;
                } else i++;
                continue;
            }

            wchar_t c = line->text[i];
            if (c == L'/' && (i + 1 < len) && line->text[i + 1] == L'/') break;
            else if (c == L'/' && (i + 1 < len) && line->text[i + 1] == L'*') { inBlockComment = TRUE; i += 2; }
            else if (c == L'"' || c == L'\'') {
                wchar_t q = c; i++;
                while (i < len && line->text[i] != q) {
                    if (line->text[i] == L'\\' && (i + 1 < len)) i++;
                    i++;
                }
                if (i < len) i++;
            } else if (c == L'{') {
                int guideCol = openIndent;
                // If opening multiple braces on the same line, step each deeper by 1 tab stop:
                if (stackTop > 0 && stack[stackTop - 1].row == r) {
                    guideCol = stack[stackTop - 1].guideCol + tabW;
                }

                if (stackTop < 4096) {
                    stack[stackTop].row = r;
                    stack[stackTop].col = (int64_t)i;
                    stack[stackTop].guideCol = guideCol;
                    stackTop++;
                }
                i++;
            } else if (c == L'}') {
                if (stackTop > 0) {
                    stackTop--;
                    if (s->blockCount >= s->blockCapacity) {
                        int newCap = (s->blockCapacity == 0) ? 512 : (s->blockCapacity * 2);
                        CodeBlock* newB = (CodeBlock*)realloc(s->blocks, newCap * sizeof(CodeBlock));
                        if (newB) { s->blocks = newB; s->blockCapacity = newCap; }
                    }
                    if (s->blocks && s->blockCount < s->blockCapacity) {
                        s->blocks[s->blockCount].openRow = stack[stackTop].row;
                        s->blocks[s->blockCount].openCol = (int)stack[stackTop].col;
                        s->blocks[s->blockCount].closeRow = r;
                        s->blocks[s->blockCount].closeCol = (int)i;
                        s->blocks[s->blockCount].guideCol = stack[stackTop].guideCol;
                        s->blockCount++;
                    }
                }
                i++;
            } else i++;
        }
    }
}

static int GetLineIndentCols(const EditorState* s, int r, int tabW) {
    if (r < 0 || r >= s->lineCount) return 0;
    const Line* line = &s->lines[r];
    int cols = 0;
    size_t idx = 0;
    while (idx < line->length && (line->text[idx] == L' ' || line->text[idx] == L'\t')) {
        if (line->text[idx] == L'\t') cols += tabW - (cols % tabW);
        else cols++;
        idx++;
    }
    return cols;
}

static BOOL IsLineBlank(const Line* line) {
    if (!line || line->length == 0) return TRUE;
    for (size_t i = 0; i < line->length; i++) {
        if (line->text[i] != L' ' && line->text[i] != L'\t') return FALSE;
    }
    return TRUE;
}

static int GetActiveBlockIndex(const EditorState* s) {
    if (!s || !s->showIndentGuides || !s->highlightIndentGuides || s->lineCount <= 0) return -1;
    int caretRow = s->caret.row;
    int64_t caretCol = s->caret.col;

    // 1. Direct hit on '{' or '}'
    for (int b = 0; b < s->blockCount; b++) {
        if (s->blocks[b].closeRow > s->blocks[b].openRow) {
            if ((s->blocks[b].openRow == caretRow && (s->blocks[b].openCol == caretCol || s->blocks[b].openCol == caretCol - 1)) ||
                (s->blocks[b].closeRow == caretRow && (s->blocks[b].closeCol == caretCol || s->blocks[b].closeCol == caretCol - 1))) {
                return b;
            }
        }
    }

    // 2. Scope check: deepest enclosing multi-line block where guideCol <= caretCol
    int bestGuideCol = -1;
    int smallestSpan = 9999999;
    int activeBlockIdx = -1;

    Line* curLine = &s->lines[caretRow];
    size_t firstNonSpace = 0;
    while (curLine && curLine->text && firstNonSpace < curLine->length && 
           (curLine->text[firstNonSpace] == L' ' || curLine->text[firstNonSpace] == L'\t')) {
        firstNonSpace++;
    }

    for (int b = 0; b < s->blockCount; b++) {
        if (s->blocks[b].closeRow > s->blocks[b].openRow) {
            if (caretRow >= s->blocks[b].openRow && caretRow <= s->blocks[b].closeRow) {
                // On the opening line, if caret is past leading whitespace,
                // it must be at or past this block's '{' to be inside this block:
                if (caretRow == s->blocks[b].openRow && (size_t)caretCol >= firstNonSpace && caretCol < s->blocks[b].openCol) {
                    continue;
                }

                if (s->blocks[b].guideCol <= caretCol) {
                    int span = s->blocks[b].closeRow - s->blocks[b].openRow;
                    if (s->blocks[b].guideCol > bestGuideCol ||
                       (s->blocks[b].guideCol == bestGuideCol && span < smallestSpan)) {
                        bestGuideCol = s->blocks[b].guideCol;
                        smallestSpan = span;
                        activeBlockIdx = b;
                    }
                }
            }
        }
    }
    return activeBlockIdx;
}

/*VERSION 1 - PAINTS INDENT LINES ONLY IF THERE IS TEXT ON THE LINE
static void PaintIndentGuides(HDC hdc, EditorState* s, int lineIdx, int y, const Theme* theme, int activeBlockIdx) {
    if (!s->showIndentGuides) return;

    int tabW = (s->tabSize > 0) ? s->tabSize : 2;
    int activeGuideCol = (activeBlockIdx != -1) ? s->blocks[activeBlockIdx].guideCol : -1;
    int lineIndent = 0;

    if (IsLineBlank(&s->lines[lineIdx])) {
        int prevIndent = 0;
        for (int r = lineIdx - 1; r >= 0; r--) {
            if (!IsLineBlank(&s->lines[r])) { prevIndent = GetLineIndentCols(s, r, tabW); break; }
        }
        int nextIndent = 0;
        for (int r = lineIdx + 1; r < s->lineCount; r++) {
            if (!IsLineBlank(&s->lines[r])) { nextIndent = GetLineIndentCols(s, r, tabW); break; }
        }
        lineIndent = (prevIndent > nextIndent) ? prevIndent : nextIndent;

        for (int b = 0; b < s->blockCount; b++) {
            if (lineIdx > s->blocks[b].openRow && lineIdx < s->blocks[b].closeRow) {
                int blockIndent = s->blocks[b].guideCol + tabW;
                if (blockIndent > lineIndent) lineIndent = blockIndent;
            }
        }
    } else {
        lineIndent = GetLineIndentCols(s, lineIdx, tabW);
    }

    if (lineIndent <= 0) return;

    TextPos selStart = {0, 0};
    TextPos selEnd   = {0, 0};
    BOOL hasSelection = FALSE;
    if (s->isDraggingText) {
        if (PosCompare(s->dragSelAnchor, s->dragSelCaret) <= 0) { selStart = s->dragSelAnchor; selEnd = s->dragSelCaret; }
        else { selStart = s->dragSelCaret; selEnd = s->dragSelAnchor; }
        hasSelection = (PosCompare(selStart, selEnd) != 0);
    } else if (HasSelection(s)) {
        GetSelectionRange(s, &selStart, &selEnd);
        hasSelection = TRUE;
    }

    for (int col = 0; col < lineIndent; col += tabW) {
        BOOL isActive = (s->highlightIndentGuides && activeBlockIdx != -1 && col == activeGuideCol &&
                         lineIdx > s->blocks[activeBlockIdx].openRow && lineIdx < s->blocks[activeBlockIdx].closeRow);

        if (col == 0 && !isActive) continue;

        BOOL isColSelected = FALSE;
        if (hasSelection) {
            if (lineIdx > selStart.row && lineIdx < selEnd.row) isColSelected = TRUE;
            else if (lineIdx == selStart.row && lineIdx == selEnd.row) {
                if (col >= selStart.col && col < selEnd.col) isColSelected = TRUE;
            } else if (lineIdx == selStart.row) {
                if (col >= selStart.col) isColSelected = TRUE;
            } else if (lineIdx == selEnd.row) {
                if (col < selEnd.col) isColSelected = TRUE;
            }
        }

        int x = (int)((col - s->scrollX) * s->charWidth) + s->gutterWidth + TEXT_MARGIN_X - 1;
        if (x >= s->gutterWidth) {
            COLORREF guideColor;
            if (isActive) guideColor = isColSelected ? (s->isDark ? RGB(100, 200, 255) : RGB(0, 80, 220)) : theme->activeIndentGuide;
            else guideColor = isColSelected ? (s->isDark ? RGB(100, 155, 210) : RGB(125, 125, 125)) : theme->indentGuide;

            for (int py = y; py < y + s->charHeight; py++) {
                if (((py + s->scrollY) % 2) == 0) SetPixelV(hdc, x, py, guideColor);
            }
        }
    }
}
*/

//VERSION 2 - PAINTS INDENT LINES ALWAYS
static void PaintIndentGuides(HDC hdc, EditorState* s, int lineIdx, int y, const Theme* theme, int activeBlockIdx) {
    if (!s->showIndentGuides) return;

    int tabW = (s->tabSize > 0) ? s->tabSize : 2;
    int activeGuideCol = (activeBlockIdx != -1) ? s->blocks[activeBlockIdx].guideCol : -1;
    
    // Read the line's actual tabs/spaces indentation first
    int lineIndent = GetLineIndentCols(s, lineIdx, tabW);

    // Only infer from surrounding lines if the current line has no indentation at all
    if (lineIndent == 0 && IsLineBlank(&s->lines[lineIdx])) {
        int prevIndent = 0;
        for (int r = lineIdx - 1; r >= 0; r--) {
            if (!IsLineBlank(&s->lines[r])) { prevIndent = GetLineIndentCols(s, r, tabW); break; }
        }
        int nextIndent = 0;
        for (int r = lineIdx + 1; r < s->lineCount; r++) {
            if (!IsLineBlank(&s->lines[r])) { nextIndent = GetLineIndentCols(s, r, tabW); break; }
        }
        lineIndent = (prevIndent > nextIndent) ? prevIndent : nextIndent;

        for (int b = 0; b < s->blockCount; b++) {
            if (lineIdx > s->blocks[b].openRow && lineIdx < s->blocks[b].closeRow) {
                int blockIndent = s->blocks[b].guideCol + tabW;
                if (blockIndent > lineIndent) lineIndent = blockIndent;
            }
        }
    }

    if (lineIndent <= 0) return;

    TextPos selStart = {0, 0};
    TextPos selEnd   = {0, 0};
    BOOL hasSelection = FALSE;
    if (s->isDraggingText) {
        if (PosCompare(s->dragSelAnchor, s->dragSelCaret) <= 0) { selStart = s->dragSelAnchor; selEnd = s->dragSelCaret; }
        else { selStart = s->dragSelCaret; selEnd = s->dragSelAnchor; }
        hasSelection = (PosCompare(selStart, selEnd) != 0);
    } else if (HasSelection(s)) {
        GetSelectionRange(s, &selStart, &selEnd);
        hasSelection = TRUE;
    }

    for (int col = 0; col < lineIndent; col += tabW) {
        BOOL isActive = (s->highlightIndentGuides && activeBlockIdx != -1 && col == activeGuideCol &&
                         lineIdx > s->blocks[activeBlockIdx].openRow && lineIdx < s->blocks[activeBlockIdx].closeRow);

        BOOL isColSelected = FALSE;
        if (hasSelection) {
            if (lineIdx > selStart.row && lineIdx < selEnd.row) isColSelected = TRUE;
            else if (lineIdx == selStart.row && lineIdx == selEnd.row) {
                if (col >= selStart.col && col < selEnd.col) isColSelected = TRUE;
            } else if (lineIdx == selStart.row) {
                if (col >= selStart.col) isColSelected = TRUE;
            } else if (lineIdx == selEnd.row) {
                if (col < selEnd.col) isColSelected = TRUE;
            }
        }

        int x = (int)((col - s->scrollX) * s->charWidth) + s->gutterWidth + TEXT_MARGIN_X - 1;
        if (x >= s->gutterWidth) {
            COLORREF guideColor;
            if (isActive) guideColor = isColSelected ? (s->isDark ? RGB(100, 200, 255) : RGB(0, 80, 220)) : theme->activeIndentGuide;
            else guideColor = isColSelected ? (s->isDark ? RGB(100, 155, 210) : RGB(125, 125, 125)) : theme->indentGuide;

            for (int py = y; py < y + s->charHeight; py++) {
                if (((py + s->scrollY) % 2) == 0) SetPixelV(hdc, x, py, guideColor);
            }
        }
    }
}

// -------------------------------------------------------------
// Viewport-Clamped Line Rendering (Immune to Integer Overflow)
// -------------------------------------------------------------
static void PaintSyntaxLine(HDC hdc, EditorState* s, int lineIdx, int y, RECT* clientRc, const Theme* theme, int activeBlockIdx){
  Line* line = &s->lines[lineIdx];
    size_t len = line->length;
    if (len == 0 || !line->text) {
        PaintIndentGuides(hdc, s, lineIdx, y, theme, activeBlockIdx);
        return;
    }

    int charW = (s->charWidth > 0) ? s->charWidth : 8;
    int xOffset = s->gutterWidth + TEXT_MARGIN_X;
    int clientW = clientRc->right - xOffset;
    if (clientW <= 0) clientW = 800;
    int visCols = (clientW / charW) + 4;

    size_t firstVisCol = (s->scrollX > 0) ? (size_t)s->scrollX : 0;
    if (firstVisCol > len) firstVisCol = len;

    size_t lastVisCol = firstVisCol + (size_t)visCols;
    if (lastVisCol > len) lastVisCol = len;

    TextPos selStart = {0, 0};
    TextPos selEnd = {0, 0};
    BOOL lineHasSel = FALSE;
    if (s->isDraggingText) {
        if (PosCompare(s->dragSelAnchor, s->dragSelCaret) <= 0) { selStart = s->dragSelAnchor; selEnd = s->dragSelCaret; }
        else { selStart = s->dragSelCaret; selEnd = s->dragSelAnchor; }
        if (lineIdx >= selStart.row && lineIdx <= selEnd.row) lineHasSel = TRUE;
    } else if (HasSelection(s)) {
        GetSelectionRange(s, &selStart, &selEnd);
        if (lineIdx >= selStart.row && lineIdx <= selEnd.row) lineHasSel = TRUE;
    }

    BOOL inBlockComment = line->inComment;
    
    // Optimization: For huge lines, only tokenize around the visible viewport window
    size_t startScan = (firstVisCol > 256) ? (firstVisCol - 256) : 0;
    size_t limitEnd = (lastVisCol + 256 < len) ? (lastVisCol + 256) : len;
    size_t i = startScan;

    while (i < limitEnd) {
        size_t tokenStart = i;
        COLORREF color = theme->text;
        wchar_t c = line->text[i];

        if (inBlockComment) {
            color = theme->comment;
            while (i < limitEnd) {
                if (line->text[i] == L'*' && (i + 1 < len) && line->text[i + 1] == L'/') {
                    i += 2; inBlockComment = FALSE; break;
                }
                i++;
            }
        } else if (c == L'/' && (i + 1 < len) && line->text[i + 1] == L'/') {
            color = theme->comment; i = limitEnd;
        } else if (c == L'/' && (i + 1 < len) && line->text[i + 1] == L'*') {
            color = theme->comment; i += 2; inBlockComment = TRUE;
            while (i < limitEnd) {
                if (line->text[i] == L'*' && (i + 1 < len) && line->text[i + 1] == L'/') {
                    i += 2; inBlockComment = FALSE; break;
                }
                i++;
            }
        } else if (c == L'"' || c == L'\'') {
            color = theme->string; wchar_t q = c; i++;
            while (i < limitEnd && line->text[i] != q) {
                if (line->text[i] == L'\\' && (i + 1 < len)) i++;
                i++;
            }
            if (i < limitEnd) i++;
        } else if (c == L'#') {
            color = theme->preprocessor; i++;
            while (i < limitEnd) {
                if (line->text[i] == L'/' && (i + 1 < len) && (line->text[i + 1] == L'/' || line->text[i + 1] == L'*')) break;
                i++;
            }
        } else if (c >= L'0' && c <= L'9') {
            color = theme->number;
            while (i < limitEnd && (IsIdentChar(line->text[i]) || line->text[i] == L'.')) i++;
        } else if (IsIdentChar(c)) {
            while (i < limitEnd && IsIdentChar(line->text[i])) i++;
            size_t tokLen = i - tokenStart;
            wchar_t temp[128];
            if (tokLen < 128) {
                wcsncpy_s(temp, 128, &line->text[tokenStart], tokLen);
                temp[tokLen] = L'\0';
                if (IsInWordList(temp, g_keywordsBuffer)) color = theme->keyword;
            }
        } else i++;

        size_t drawStart = (tokenStart > firstVisCol) ? tokenStart : firstVisCol;
        size_t drawEnd = (i < lastVisCol) ? i : lastVisCol;

        for (size_t chIdx = drawStart; chIdx < drawEnd; chIdx++) {
            int charX = ((int)(chIdx - s->scrollX) * charW) + xOffset;
            if (charX + charW < s->gutterWidth || charX > clientRc->right) continue;

            BOOL isCharSelected = FALSE;
            if (lineHasSel) {
                size_t c1 = (lineIdx == selStart.row) ? (size_t)selStart.col : 0;
                size_t c2 = (lineIdx == selEnd.row) ? (size_t)selEnd.col : len + 1;
                if (chIdx >= c1 && chIdx < c2) isCharSelected = TRUE;
            }

            BOOL isBrace = FALSE;
            if (s->showBraceMatch && s->hasBraceMatch) {
                if ((s->brace1.row == lineIdx && (size_t)s->brace1.col == chIdx) ||
                    (s->brace2.row == lineIdx && (size_t)s->brace2.col == chIdx)) isBrace = TRUE;
            }

            RECT charRc = { charX, y, charX + charW, y + s->charHeight };
            COLORREF bgCol = isCharSelected ? theme->selection : (isBrace ? theme->braceMatchBg : theme->bg);

            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, bgCol);
            SetTextColor(hdc, color);
            ExtTextOutW(hdc, charX, y, ETO_OPAQUE | ETO_CLIPPED, &charRc, &line->text[chIdx], 1, NULL);

            if (isBrace) {
                HPEN bmPen = CreatePen(PS_SOLID, 1, theme->braceMatchBorder);
                HGDIOBJ oldPen = SelectObject(hdc, bmPen);
                HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, charRc.left, charRc.top, charRc.right, charRc.bottom);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(bmPen);
            }
        }
    }

    PaintIndentGuides(hdc, s, lineIdx, y, theme, activeBlockIdx);
}

static void ShowContextMenu(HWND hwnd, EditorState* s, int screenX, int screenY) {
    HMENU hMenu = CreatePopupMenu();
    BOOL hasSel = HasSelection(s);
    BOOL canPaste = IsClipboardFormatAvailable(CF_UNICODETEXT);
    BOOL canUndo = (s->undoStack.count > 0);
    BOOL canRedo = (s->redoStack.count > 0);

    AppendMenuW(hMenu, MF_STRING | (canUndo ? MF_ENABLED : MF_GRAYED), IDM_EDIT_UNDO, L"Undo\tCtrl+Z");
    AppendMenuW(hMenu, MF_STRING | (canRedo ? MF_ENABLED : MF_GRAYED), IDM_EDIT_REDO, L"Redo\tCtrl+Y");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING | (hasSel ? MF_ENABLED : MF_GRAYED), IDM_EDIT_CUT, L"Cut\tCtrl+X");
    AppendMenuW(hMenu, MF_STRING | (hasSel ? MF_ENABLED : MF_GRAYED), IDM_EDIT_COPY, L"Copy\tCtrl+C");
    AppendMenuW(hMenu, MF_STRING | (canPaste ? MF_ENABLED : MF_GRAYED), IDM_EDIT_PASTE, L"Paste\tCtrl+V");
    AppendMenuW(hMenu, MF_STRING | (hasSel ? MF_ENABLED : MF_GRAYED), IDM_EDIT_DELETE, L"Delete\tDel");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_EDIT_SELECTALL, L"Select All\tCtrl+A");

    if (screenX == -1 && screenY == -1) {
        POINT pt;
        GetCaretPos(&pt);
        ClientToScreen(hwnd, &pt);
        screenX = pt.x; screenY = pt.y;
    }
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, screenX, screenY, 0, hwnd, NULL);
    DestroyMenu(hMenu);
}

// -------------------------------------------------------------
// Search, Replace, Goto & Settings Dialogs
// -------------------------------------------------------------
LRESULT CALLBACK FindDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static EditorState* s = NULL;
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        s = (EditorState*)cs->lpCreateParams;
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        HWND hFindWhat = CreateWindowW(L"STATIC", L"Fi&nd what:", WS_CHILD | WS_VISIBLE, 12, 16, 80, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hFindWhat, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        HWND hFindEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_search.findText,
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP, 95, 14, 210, 22, hwnd, (HMENU)IDFINDWHAT, NULL, NULL);
        SendMessage(hFindEdit, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hWhole = CreateWindowW(L"BUTTON", L"Match &whole word only", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP | WS_GROUP,
            12, 48, 180, 20, hwnd, (HMENU)IDWHOLEWORD, NULL, NULL);
        SendMessage(hWhole, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hWhole, BM_SETCHECK, g_search.matchWholeWord ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hCase = CreateWindowW(L"BUTTON", L"Case sensiti&ve", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
            12, 72, 180, 20, hwnd, (HMENU)IDMATCHCASE, NULL, NULL);
        SendMessage(hCase, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hCase, BM_SETCHECK, g_search.matchCase ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hWrap = CreateWindowW(L"BUTTON", L"Wrap ar&ound", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
            12, 96, 180, 20, hwnd, (HMENU)IDWRAP, NULL, NULL);
        SendMessage(hWrap, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hWrap, BM_SETCHECK, g_search.wrapAround ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hDirection = CreateWindowW(L"BUTTON", L"Direction", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_GROUP, 195, 48, 110, 52, hwnd, NULL, NULL, NULL);
        SendMessage(hDirection, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        
        HWND hUp = CreateWindowW(L"BUTTON", L"&Up", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,
            205, 68, 45, 18, hwnd, (HMENU)IDDIRUP, NULL, NULL);
        HWND hDown = CreateWindowW(L"BUTTON", L"&Down", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            255, 68, 45, 18, hwnd, (HMENU)IDDIRDOWN, NULL, NULL);
        SendMessage(hUp, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hDown, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(!g_search.searchDown ? hUp : hDown, BM_SETCHECK, BST_CHECKED, 0);

        HWND hBtnFind = CreateWindowW(L"BUTTON", L"&Find Next", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP,
            320, 14, 85, 24, hwnd, (HMENU)IDOK, NULL, NULL);
        SendMessage(hBtnFind, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hBtnClose = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            320, 44, 85, 24, hwnd, (HMENU)IDCANCEL, NULL, NULL);
        SendMessage(hBtnClose, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        SetFocus(hFindEdit);
        SendMessage(hFindEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDOK: {
            GetDlgItemTextW(hwnd, IDFINDWHAT, g_search.findText, 256);
            g_search.matchWholeWord = (IsDlgButtonChecked(hwnd, IDWHOLEWORD) == BST_CHECKED);
            g_search.matchCase = (IsDlgButtonChecked(hwnd, IDMATCHCASE) == BST_CHECKED);
            g_search.wrapAround = (IsDlgButtonChecked(hwnd, IDWRAP) == BST_CHECKED);
            g_search.searchDown = (IsDlgButtonChecked(hwnd, IDDIRDOWN) == BST_CHECKED);

            if (FindNextText(s, g_search.searchDown)) {
                DestroyWindow(hwnd);
                SetFocus(g_hwndEditor);
            }
            break;
        }
        case IDCANCEL:
            DestroyWindow(hwnd);
            SetFocus(g_hwndEditor);
            break;
        }
        return 0;
    }
    case WM_DESTROY: g_hFindDlg = NULL; return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void ShowFindDialog(HWND hwndParent, EditorState* s) {
    if (HasSelection(s)) {
        TextPos p1, p2;
        GetSelectionRange(s, &p1, &p2);
        if (p1.row == p2.row && p2.col - p1.col < 256) {
            wcsncpy_s(g_search.findText, 256, &s->lines[p1.row].text[p1.col], (size_t)(p2.col - p1.col));
            g_search.findText[p2.col - p1.col] = L'\0';
        }
    }

    if (g_hFindDlg && IsWindow(g_hFindDlg)) {
        SetDlgItemTextW(g_hFindDlg, IDFINDWHAT, g_search.findText);
        HWND hFindEdit = GetDlgItem(g_hFindDlg, IDFINDWHAT);
        SendMessage(hFindEdit, EM_SETSEL, 0, -1);
        ShowWindow(g_hFindDlg, SW_SHOWNORMAL);
        SetForegroundWindow(g_hFindDlg);
        SetFocus(hFindEdit);
        return;
    }

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = FindDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = FIND_DLG_CLASS;
    RegisterClassEx(&wc);

    RECT rc;
    GetWindowRect(hwndParent, &rc);
    g_hFindDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, FIND_DLG_CLASS, L"Find",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rc.left + 50, rc.top + 50, 430, 165, hwndParent, NULL, GetModuleHandle(NULL), s);
}

LRESULT CALLBACK ReplaceDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static EditorState* s = NULL;
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        s = (EditorState*)cs->lpCreateParams;
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        HWND hLabel1 = CreateWindowW(L"STATIC", L"Fi&nd what:", WS_CHILD | WS_VISIBLE, 12, 14, 85, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLabel1, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        
        HWND hFind = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_search.findText,
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP, 100, 12, 180, 22, hwnd, (HMENU)IDFINDWHAT, NULL, NULL);
        SendMessage(hFind, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hLabel2 = CreateWindowW(L"STATIC", L"Re&place with:", WS_CHILD | WS_VISIBLE, 12, 42, 85, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLabel2, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        
        HWND hRepl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_search.replaceText,
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP, 100, 40, 180, 22, hwnd, (HMENU)IDREPLACEWITH, NULL, NULL);
        SendMessage(hRepl, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hWhole = CreateWindowW(L"BUTTON", L"Match &whole word only", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP | WS_GROUP,
            12, 74, 180, 20, hwnd, (HMENU)IDWHOLEWORD, NULL, NULL);
        SendMessage(hWhole, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hWhole, BM_SETCHECK, g_search.matchWholeWord ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hCase = CreateWindowW(L"BUTTON", L"Case sensiti&ve", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
            12, 96, 180, 20, hwnd, (HMENU)IDMATCHCASE, NULL, NULL);
        SendMessage(hCase, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hCase, BM_SETCHECK, g_search.matchCase ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hWrap = CreateWindowW(L"BUTTON", L"Wrap ar&ound", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
            12, 118, 180, 20, hwnd, (HMENU)IDWRAP, NULL, NULL);
        SendMessage(hWrap, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hWrap, BM_SETCHECK, g_search.wrapAround ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hBtnFind = CreateWindowW(L"BUTTON", L"&Find Next", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP,
            295, 10, 130, 24, hwnd, (HMENU)IDOK, NULL, NULL);
        HWND hBtnRepl = CreateWindowW(L"BUTTON", L"&Replace", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            295, 38, 130, 24, hwnd, (HMENU)IDREPLACE, NULL, NULL);
        HWND hBtnAll = CreateWindowW(L"BUTTON", L"Replace &All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            295, 66, 130, 24, hwnd, (HMENU)IDREPLACEALL, NULL, NULL);
        HWND hBtnInSel = CreateWindowW(L"BUTTON", L"Replace in &Selection", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            295, 94, 130, 24, hwnd, (HMENU)IDREPLACEINSEL, NULL, NULL);
        HWND hBtnClose = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            295, 122, 130, 24, hwnd, (HMENU)IDCANCEL, NULL, NULL);

        SendMessage(hBtnFind, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hBtnRepl, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hBtnAll, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hBtnInSel, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hBtnClose, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hLabel3 = CreateWindowW(L"STATIC", L"Replacements:", WS_CHILD | WS_VISIBLE, 12, 150, 95, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLabel3, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        
        HWND hLabel4 = CreateWindowW(L"STATIC", L"0", WS_CHILD | WS_VISIBLE, 110, 150, 60, 18, hwnd, (HMENU)IDREPLDONE, NULL, NULL);
        SendMessage(hLabel4, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        SetFocus(hFind);
        SendMessage(hFind, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND: {
        GetDlgItemTextW(hwnd, IDFINDWHAT, g_search.findText, 256);
        GetDlgItemTextW(hwnd, IDREPLACEWITH, g_search.replaceText, 256);
        g_search.matchWholeWord = (IsDlgButtonChecked(hwnd, IDWHOLEWORD) == BST_CHECKED);
        g_search.matchCase = (IsDlgButtonChecked(hwnd, IDMATCHCASE) == BST_CHECKED);
        g_search.wrapAround = (IsDlgButtonChecked(hwnd, IDWRAP) == BST_CHECKED);

        switch (LOWORD(wParam)) {
        case IDOK: FindNextText(s, TRUE); break;
        case IDREPLACE: {
            if (HasSelection(s)) {
                TextPos p1, p2;
                GetSelectionRange(s, &p1, &p2);
                size_t selLen = (size_t)(p2.col - p1.col);
                if (p1.row == p2.row) {
                    BOOL match = g_search.matchCase ? (wcsncmp(&s->lines[p1.row].text[p1.col], g_search.findText, selLen) == 0) :
                                                      (_wcsnicmp(&s->lines[p1.row].text[p1.col], g_search.findText, selLen) == 0);
                    if (match) {
                        PushUndo(s);
                        Line_DeleteChars(&s->lines[p1.row], (size_t)p1.col, selLen);
                        for (size_t k = 0; k < wcslen(g_search.replaceText); k++) Line_InsertChar(&s->lines[p1.row], (size_t)p1.col + k, g_search.replaceText[k]);
                        s->caret.col = p1.col + (int64_t)wcslen(g_search.replaceText);
                        s->preferredCol = s->caret.col;
                        ClearSelection(s);
                        FindMatchingBrace(s);
                        SetModified(s, TRUE);
                    }
                }
            }
            FindNextText(s, TRUE);
            break;
        }
        case IDREPLACEALL: {
            int count = 0;
            size_t findLen = wcslen(g_search.findText), replLen = wcslen(g_search.replaceText);
            if (findLen > 0) {
                PushUndo(s);
                for (int r = 0; r < s->lineCount; r++) {
                    size_t c = 0;
                    while (c + findLen <= s->lines[r].length) {
                        int found = FindStringInLine(s->lines[r].text, s->lines[r].length, g_search.findText, findLen, c, g_search.matchCase, g_search.matchWholeWord, TRUE);
                        if (found == -1) break;

                        Line_DeleteChars(&s->lines[r], (size_t)found, findLen);
                        for (size_t k = 0; k < replLen; k++) Line_InsertChar(&s->lines[r], (size_t)found + k, g_search.replaceText[k]);
                        c = (size_t)found + replLen;
                        count++;
                    }
                }
                if (count > 0) {
                    SetModified(s, TRUE);
                    UpdateCodeBlocks(s);
                    RecalculateMaxCols(s);
                    FindMatchingBrace(s);
                    InvalidateRect(g_hwndEditor, NULL, FALSE);
                }
            }
            SetDlgItemInt(hwnd, IDREPLDONE, count, FALSE);
            break;
        }
        case IDREPLACEINSEL: {
            int count = 0;
            if (HasSelection(s)) {
                TextPos p1, p2;
                GetSelectionRange(s, &p1, &p2);
                size_t findLen = wcslen(g_search.findText), replLen = wcslen(g_search.replaceText);
                if (findLen > 0) {
                    PushUndo(s);
                    for (int r = p1.row; r <= p2.row; r++) {
                        size_t c1 = (r == p1.row) ? (size_t)p1.col : 0;
                        size_t c2 = (r == p2.row) ? (size_t)p2.col : s->lines[r].length;

                        while (c1 + findLen <= c2) {
                            int found = FindStringInLine(s->lines[r].text, c2, g_search.findText, findLen, c1, g_search.matchCase, g_search.matchWholeWord, TRUE);
                            if (found == -1 || (size_t)found + findLen > c2) break;

                            Line_DeleteChars(&s->lines[r], (size_t)found, findLen);
                            for (size_t k = 0; k < replLen; k++) Line_InsertChar(&s->lines[r], (size_t)found + k, g_search.replaceText[k]);
                            c1 = (size_t)found + replLen;
                            if (replLen >= findLen) {
                                c2 += (replLen - findLen);
                            } else {
                                size_t diff = findLen - replLen;
                                c2 = (c2 >= diff) ? (c2 - diff) : 0;
                            }
                            count++;
                        }
                    }
                    if (count > 0) {
                        SetModified(s, TRUE);
                        UpdateCodeBlocks(s);
                        RecalculateMaxCols(s);
                        FindMatchingBrace(s);
                        InvalidateRect(g_hwndEditor, NULL, FALSE);
                    }
                }
            }
            SetDlgItemInt(hwnd, IDREPLDONE, count, FALSE);
            break;
        }
        case IDCANCEL: DestroyWindow(hwnd); break;
        }
        return 0;
    }
    case WM_DESTROY: g_hReplaceDlg = NULL; return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void ShowReplaceDialog(HWND hwndParent, EditorState* s) {
    if (HasSelection(s)) {
        TextPos p1, p2;
        GetSelectionRange(s, &p1, &p2);
        if (p1.row == p2.row && p2.col - p1.col < 256) {
            wcsncpy_s(g_search.findText, 256, &s->lines[p1.row].text[p1.col], (size_t)(p2.col - p1.col));
            g_search.findText[p2.col - p1.col] = L'\0';
        }
    }

    if (g_hReplaceDlg && IsWindow(g_hReplaceDlg)) {
        SetDlgItemTextW(g_hReplaceDlg, IDFINDWHAT, g_search.findText);
        HWND hFind = GetDlgItem(g_hReplaceDlg, IDFINDWHAT);
        SendMessage(hFind, EM_SETSEL, 0, -1);
        ShowWindow(g_hReplaceDlg, SW_SHOWNORMAL);
        SetForegroundWindow(g_hReplaceDlg);
        SetFocus(hFind);
        return;
    }

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = ReplaceDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = REPLACE_DLG_CLASS;
    RegisterClassEx(&wc);

    RECT rc;
    GetWindowRect(hwndParent, &rc);
    g_hReplaceDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, REPLACE_DLG_CLASS, L"Replace",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rc.left + 50, rc.top + 50, 450, 215, hwndParent, NULL, GetModuleHandle(NULL), s);
}

LRESULT CALLBACK GotoDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static EditorState* s = NULL;
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        s = (EditorState*)cs->lpCreateParams;
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        wchar_t prompt[128];
        wsprintf(prompt, L"Line number (1 - %d):", s->lineCount);
        HWND hLabel = CreateWindowW(L"STATIC", prompt, WS_CHILD | WS_VISIBLE, 12, 14, 180, 18, hwnd, (HMENU)1001, NULL, NULL);
        SendMessage(hLabel, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP, 12, 36, 200, 22, hwnd, (HMENU)IDGOTO_LINE, NULL, NULL);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hOk = CreateWindowW(L"BUTTON", L"Go To", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP,
            45, 70, 75, 24, hwnd, (HMENU)IDOK, NULL, NULL);
        HWND hCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            125, 70, 75, 24, hwnd, (HMENU)IDCANCEL, NULL, NULL);

        SendMessage(hOk, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hCancel, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        SetFocus(hEdit);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam)) {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                int line = GetDlgItemInt(hwnd, IDGOTO_LINE, NULL, FALSE);
                if (line < 1) line = 1;
                if (line > s->lineCount) line = s->lineCount;

                int targetRow = line - 1;
                s->caret.row = targetRow;
                s->caret.col = 0;
                s->preferredCol = 0;
                ClearSelection(s);

                RECT rc;
                GetClientRect(g_hwndEditor, &rc);
                int visibleLines = (rc.bottom > 0 && s->charHeight > 0) ? (rc.bottom / s->charHeight) : 1;
                int centerRow = targetRow - (visibleLines / 2);
                if (centerRow < 0) centerRow = 0;

                s->scrollY = centerRow * s->charHeight;
                int maxScroll = (s->lineCount - visibleLines) * s->charHeight;
                if (maxScroll < 0) maxScroll = 0;
                if (s->scrollY > maxScroll) s->scrollY = maxScroll;
                s->scrollX = 0;

                FindMatchingBrace(s);
                UpdateScrollbars(g_hwndEditor, s);
                UpdateCaret(g_hwndEditor, s);
                InvalidateRect(g_hwndEditor, NULL, FALSE);

                DestroyWindow(hwnd);
                SetFocus(g_hwndEditor);
            } else if (id == IDCANCEL) {
                DestroyWindow(hwnd);
                SetFocus(g_hwndEditor);
            }
        }
        return 0;
    }
    case WM_DESTROY: g_hGotoDlg = NULL; return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void ShowGotoDialog(HWND hwndParent, EditorState* s) {
    if (g_hGotoDlg && IsWindow(g_hGotoDlg)) {
        wchar_t prompt[128];
        wsprintf(prompt, L"Line number (1 - %d):", s->lineCount);
        SetDlgItemTextW(g_hGotoDlg, 1001, prompt);
        HWND hEdit = GetDlgItem(g_hGotoDlg, IDGOTO_LINE);
        SetDlgItemInt(g_hGotoDlg, IDGOTO_LINE, s->caret.row + 1, FALSE);
        SendMessage(hEdit, EM_SETSEL, 0, -1);
        ShowWindow(g_hGotoDlg, SW_SHOWNORMAL);
        SetForegroundWindow(g_hGotoDlg);
        SetFocus(hEdit);
        return;
    }

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = GotoDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = GOTO_DLG_CLASS;
    RegisterClassEx(&wc);

    RECT rc;
    GetWindowRect(hwndParent, &rc);
    g_hGotoDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, GOTO_DLG_CLASS, L"Go To Line",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rc.left + 80, rc.top + 80, 240, 140, hwndParent, NULL, GetModuleHandle(NULL), s);
}

// -------------------------------------------------------------
// Find in Files Implementation
// -------------------------------------------------------------
static int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    (void)lParam;
    if (uMsg == BFFM_INITIALIZED && lpData) {
        SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
    }
    return 0;
}

static BOOL DoBrowseDirectory(HWND hwndParent, wchar_t* outDir, int maxLen) {
    (void)maxLen;
    BROWSEINFOW bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hwndParent;
    bi.lpszTitle = L"Select directory to search:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseFolderCallback;
    bi.lParam = (LPARAM)outDir;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        BOOL ok = SHGetPathFromIDListW(pidl, outDir);
        CoTaskMemFree(pidl);
        return ok;
    }
    return FALSE;
}

static BOOL MatchesWildcard(const wchar_t* filename, const wchar_t* patterns) {
    if (!patterns || !*patterns) return TRUE;
    wchar_t buf[512];
    wcscpy_s(buf, 512, patterns);
    wchar_t* context = NULL;
    wchar_t* token = wcstok_s(buf, L" ;", &context);
    while (token) {
        if (PathMatchSpecW(filename, token)) return TRUE;
        token = wcstok_s(NULL, L" ;", &context);
    }
    return FALSE;
}

static void UpdateOutputListExtent(const wchar_t* str) {
    if (!g_hwndOutputList || !str) return;
    EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;
    HDC hdc = GetDC(g_hwndOutputList);
    HGDIOBJ oldFont = SelectObject(hdc, (s && s->hFont) ? s->hFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT));
    SIZE sz;
    if (GetTextExtentPoint32W(hdc, str, (int)wcslen(str), &sz)) {
        int curExtent = (int)SendMessageW(g_hwndOutputList, LB_GETHORIZONTALEXTENT, 0, 0);
        int neededExtent = sz.cx + 24;
        if (neededExtent > curExtent) {
            SendMessageW(g_hwndOutputList, LB_SETHORIZONTALEXTENT, (WPARAM)neededExtent, 0);
        }
    }
    SelectObject(hdc, oldFont);
    ReleaseDC(g_hwndOutputList, hdc);
}

static void RecalculateAllOutputListExtent(void) {
    if (!g_hwndOutputList || !IsWindow(g_hwndOutputList)) return;
    EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;
    HDC hdc = GetDC(g_hwndOutputList);
    HGDIOBJ oldFont = SelectObject(hdc, (s && s->hFont) ? s->hFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT));

    int count = (int)SendMessageW(g_hwndOutputList, LB_GETCOUNT, 0, 0);
    int maxW = 0;
    for (int i = 0; i < count; i++) {
        int len = (int)SendMessageW(g_hwndOutputList, LB_GETTEXTLEN, i, 0);
        if (len > 0) {
            wchar_t* buf = (wchar_t*)malloc((len + 2) * sizeof(wchar_t));
            if (buf) {
                SendMessageW(g_hwndOutputList, LB_GETTEXT, i, (LPARAM)buf);
                SIZE sz;
                if (GetTextExtentPoint32W(hdc, buf, (int)wcslen(buf), &sz) && sz.cx > maxW) maxW = sz.cx;
                free(buf);
            }
        }
    }
    SelectObject(hdc, oldFont);
    ReleaseDC(g_hwndOutputList, hdc);
    SendMessageW(g_hwndOutputList, LB_SETHORIZONTALEXTENT, maxW > 0 ? (WPARAM)(maxW + 24) : 0, 0);
}

static void SearchFileContent(const wchar_t* filePath, const wchar_t* findText, BOOL matchCase, BOOL wholeWord) {
    HANDLE hFile = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    LARGE_INTEGER liSize;
    if (!GetFileSizeEx(hFile, &liSize) || liSize.QuadPart == 0 || liSize.QuadPart > (64 * 1024 * 1024)) {
        CloseHandle(hFile);
        return;
    }

    size_t fileSize = (size_t)liSize.QuadPart;
    unsigned char* rawBuf = (unsigned char*)malloc(fileSize + 4);
    if (!rawBuf) { CloseHandle(hFile); return; }

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, rawBuf, (DWORD)fileSize, &bytesRead, NULL) || bytesRead == 0) {
        free(rawBuf);
        CloseHandle(hFile);
        return;
    }
    CloseHandle(hFile);

    int encIdx = DetectEncodingIndex(rawBuf, bytesRead);
    const EncodingDesc* enc = &g_encodings[encIdx];
    size_t wCharsCount = (fileSize + 4);
    wchar_t* wBuf = (wchar_t*)malloc((wCharsCount + 2) * sizeof(wchar_t));
    if (!wBuf) { free(rawBuf); return; }

    size_t totalWChars = 0;
    if (enc->codePage == 1200 || enc->codePage == 1201) {
        size_t off = (enc->bomType == 2 || enc->bomType == 3) ? 2 : 0;
        size_t valid = (bytesRead > off) ? bytesRead - off : 0;
        totalWChars = valid / sizeof(wchar_t);
        memcpy(wBuf, rawBuf + off, totalWChars * sizeof(wchar_t));
        if (enc->codePage == 1201) {
            for (size_t k = 0; k < totalWChars; k++) {
                wchar_t c = wBuf[k];
                wBuf[k] = (wchar_t)(((c & 0xFF) << 8) | ((c >> 8) & 0xFF));
            }
        }
    } else {
        size_t off = (enc->bomType == 1) ? 3 : 0;
        size_t valid = (bytesRead > off) ? bytesRead - off : 0;
        int chars = MultiByteToWideChar(enc->codePage, 0, (const char*)rawBuf + off, (int)valid, wBuf, (int)wCharsCount);
        if (chars <= 0 && enc->codePage != CP_ACP) {
            chars = MultiByteToWideChar(CP_ACP, 0, (const char*)rawBuf + off, (int)valid, wBuf, (int)wCharsCount);
        }
        totalWChars = (chars > 0) ? (size_t)chars : 0;
    }
    wBuf[totalWChars] = L'\0';
    free(rawBuf);

    size_t patLen = wcslen(findText);
    const wchar_t* p = wBuf;
    int lineNum = 1;

    while (*p) {
        const wchar_t* lineStart = p;
        while (*p && *p != L'\r' && *p != L'\n') p++;
        size_t lineLen = (size_t)(p - lineStart);

        int found = FindStringInLine(lineStart, lineLen, findText, patLen, 0, matchCase, wholeWord, TRUE);
        if (found != -1) {
            wchar_t* lineText = (wchar_t*)malloc((lineLen + 1) * sizeof(wchar_t));
            if (lineText) {
                wcsncpy_s(lineText, lineLen + 1, lineStart, lineLen);
                lineText[lineLen] = L'\0';

                size_t outSize = wcslen(filePath) + lineLen + 64;
                wchar_t* outMsg = (wchar_t*)malloc(outSize * sizeof(wchar_t));
                if (outMsg) {
                    swprintf_s(outMsg, outSize, L"%s:%d:%s", filePath, lineNum, lineText);
                    SendMessageW(g_hwndOutputList, LB_ADDSTRING, 0, (LPARAM)outMsg);
                    UpdateOutputListExtent(outMsg);
                    free(outMsg);
                }
                free(lineText);
            }
        }

        if (*p == L'\r' && *(p + 1) == L'\n') p += 2;
        else if (*p == L'\r' || *p == L'\n') p++;
        lineNum++;
    }

    free(wBuf);
}

static void GrepSearchDirectory(const wchar_t* dirPath, const wchar_t* patterns, const wchar_t* findText, BOOL matchCase, BOOL wholeWord) {
    wchar_t searchSpec[MAX_PATH];
    swprintf_s(searchSpec, MAX_PATH, L"%s\\*.*", dirPath);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(searchSpec, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) continue;

        wchar_t fullPath[MAX_PATH];
        swprintf_s(fullPath, MAX_PATH, L"%s\\%s", dirPath, ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            GrepSearchDirectory(fullPath, patterns, findText, matchCase, wholeWord);
        } else {
            if (MatchesWildcard(ffd.cFileName, patterns)) {
                SearchFileContent(fullPath, findText, matchCase, wholeWord);
            }
        }
    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
}

static void ExecuteFindInFiles(HWND hwndMain, const wchar_t* findText, const wchar_t* patterns, const wchar_t* directory, BOOL matchCase, BOOL wholeWord) {
    if (!findText || !*findText || !directory || !*directory) return;

    g_showOutputPane = TRUE;
    ShowWindow(g_hwndOutputPane, SW_SHOW);
    ShowWindow(g_hwndSplitter, SW_SHOW);

    SendMessage(g_hwndOutputList, LB_RESETCONTENT, 0, 0);
    SendMessage(g_hwndOutputList, LB_SETHORIZONTALEXTENT, 0, 0);

    wchar_t header[1024];
    swprintf_s(header, 1024, L">Internal search for \"%s\" in \"%s\"", findText, patterns);
    SendMessageW(g_hwndOutputList, LB_ADDSTRING, 0, (LPARAM)header);
    UpdateOutputListExtent(header);

    GrepSearchDirectory(directory, patterns, findText, matchCase, wholeWord);

    RECT rcClient;
    GetClientRect(hwndMain, &rcClient);
    SendMessage(hwndMain, WM_SIZE, 0, MAKELPARAM(rcClient.right, rcClient.bottom));
    InvalidateRect(g_hwndOutputPane, NULL, TRUE);
}

static LRESULT CALLBACK GrepDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static EditorState* s = NULL;
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        s = (EditorState*)cs->lpCreateParams;
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        HWND hLblFind = CreateWindowW(L"STATIC", L"Fi&nd what:", WS_CHILD | WS_VISIBLE, 12, 14, 75, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLblFind, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        HWND hFind = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP, 90, 12, 335, 150, hwnd, (HMENU)IDFINDWHAT, NULL, NULL);
        SendMessage(hFind, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SetWindowTextW(hFind, g_search.findText);

        HWND hLblFiles = CreateWindowW(L"STATIC", L"F&iles:", WS_CHILD | WS_VISIBLE, 12, 44, 75, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLblFiles, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        HWND hFiles = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP, 90, 42, 335, 150, hwnd, (HMENU)IDFILES, NULL, NULL);
        SendMessage(hFiles, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SetWindowTextW(hFiles, g_grepPatterns);

        HWND hLblDir = CreateWindowW(L"STATIC", L"&Directory:", WS_CHILD | WS_VISIBLE, 12, 74, 75, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLblDir, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        HWND hDir = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP, 90, 72, 305, 150, hwnd, (HMENU)IDDIRECTORY, NULL, NULL);
        SendMessage(hDir, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        if (g_grepDirectory[0] == L'\0') {
            if (s && s->filePath[0] != L'\0') {
                wcscpy_s(g_grepDirectory, MAX_PATH, s->filePath);
                wchar_t* lastSlash = wcsrchr(g_grepDirectory, L'\\');
                if (lastSlash) *lastSlash = L'\0';
            } else {
                GetCurrentDirectoryW(MAX_PATH, g_grepDirectory);
            }
        }
        SetWindowTextW(hDir, g_grepDirectory);

        HWND hBtnDotDot = CreateWindowW(L"BUTTON", L"..", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            400, 72, 25, 22, hwnd, (HMENU)IDDOTDOT, NULL, NULL);
        SendMessage(hBtnDotDot, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hWhole = CreateWindowW(L"BUTTON", L"Match &whole word only", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP | WS_GROUP,
            12, 106, 175, 20, hwnd, (HMENU)IDWHOLEWORD, NULL, NULL);
        SendMessage(hWhole, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hWhole, BM_SETCHECK, g_search.matchWholeWord ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hCase = CreateWindowW(L"BUTTON", L"Case sensiti&ve", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
            195, 106, 140, 20, hwnd, (HMENU)IDMATCHCASE, NULL, NULL);
        SendMessage(hCase, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        SendMessage(hCase, BM_SETCHECK, g_search.matchCase ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hBtnFind = CreateWindowW(L"BUTTON", L"&Find", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP,
            440, 12, 85, 24, hwnd, (HMENU)IDOK, NULL, NULL);
        SendMessage(hBtnFind, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            440, 42, 85, 24, hwnd, (HMENU)IDCANCEL, NULL, NULL);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hBtnBrowse = CreateWindowW(L"BUTTON", L"&Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            440, 72, 85, 24, hwnd, (HMENU)IDBROWSE, NULL, NULL);
        SendMessage(hBtnBrowse, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        SetFocus(hFind);
        return 0;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDDOTDOT: {
            wchar_t curDir[MAX_PATH];
            GetDlgItemTextW(hwnd, IDDIRECTORY, curDir, MAX_PATH);
            size_t len = wcslen(curDir);
            if (len > 0 && curDir[len - 1] == L'\\') curDir[len - 1] = L'\0';
            wchar_t* lastSlash = wcsrchr(curDir, L'\\');
            if (lastSlash) {
                if (lastSlash == curDir + 2 && curDir[1] == L':') {
                    *(lastSlash + 1) = L'\0';
                } else {
                    *lastSlash = L'\0';
                }
                SetDlgItemTextW(hwnd, IDDIRECTORY, curDir);
            }
            break;
        }
        case IDBROWSE: {
            wchar_t curDir[MAX_PATH];
            GetDlgItemTextW(hwnd, IDDIRECTORY, curDir, MAX_PATH);
            if (DoBrowseDirectory(hwnd, curDir, MAX_PATH)) {
                SetDlgItemTextW(hwnd, IDDIRECTORY, curDir);
            }
            break;
        }
        case IDOK: {
            GetDlgItemTextW(hwnd, IDFINDWHAT, g_search.findText, 256);
            GetDlgItemTextW(hwnd, IDFILES, g_grepPatterns, 512);
            GetDlgItemTextW(hwnd, IDDIRECTORY, g_grepDirectory, MAX_PATH);
            g_search.matchWholeWord = (IsDlgButtonChecked(hwnd, IDWHOLEWORD) == BST_CHECKED);
            g_search.matchCase = (IsDlgButtonChecked(hwnd, IDMATCHCASE) == BST_CHECKED);

            SaveSettingsToIni(s->hwndMain, s);
            ExecuteFindInFiles(s->hwndMain, g_search.findText, g_grepPatterns, g_grepDirectory, g_search.matchCase, g_search.matchWholeWord);
            DestroyWindow(hwnd);
            break;
        }
        case IDCANCEL:
            DestroyWindow(hwnd);
            break;
        }
        return 0;
    }
    case WM_DESTROY: g_hGrepDlg = NULL; return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void ShowFindInFilesDialog(HWND hwndParent, EditorState* s) {
    if (HasSelection(s)) {
        TextPos p1, p2;
        GetSelectionRange(s, &p1, &p2);
        if (p1.row == p2.row && p2.col - p1.col < 256) {
            wcsncpy_s(g_search.findText, 256, &s->lines[p1.row].text[p1.col], (size_t)(p2.col - p1.col));
            g_search.findText[p2.col - p1.col] = L'\0';
        }
    }

    if (g_hGrepDlg && IsWindow(g_hGrepDlg)) {
        SetDlgItemTextW(g_hGrepDlg, IDFINDWHAT, g_search.findText);
        HWND hFind = GetDlgItem(g_hGrepDlg, IDFINDWHAT);
        SendMessage(hFind, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
        ShowWindow(g_hGrepDlg, SW_SHOWNORMAL);
        SetForegroundWindow(g_hGrepDlg);
        SetFocus(hFind);
        return;
    }

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = GrepDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = GREP_DLG_CLASS;
    RegisterClassEx(&wc);

    RECT rc;
    GetWindowRect(hwndParent, &rc);
    g_hGrepDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, GREP_DLG_CLASS, L"Find in Files",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rc.left + 50, rc.top + 50, 555, 175, hwndParent, NULL, GetModuleHandle(NULL), s);
}

// -------------------------------------------------------------
// Output Window & Splitter
// -------------------------------------------------------------
static void NavigateToResultLine(const wchar_t* resultText) {
    if (!resultText || resultText[0] == L'>' || resultText[0] == L'\0') return;

    const wchar_t* pColon1 = wcschr(resultText, L':');
    if (!pColon1) return;
    if (pColon1 == resultText + 1 && ((resultText[0] >= L'A' && resultText[0] <= L'Z') || (resultText[0] >= L'a' && resultText[0] <= L'z'))) {
        pColon1 = wcschr(resultText + 2, L':');
        if (!pColon1) return;
    }

    const wchar_t* pColon2 = wcschr(pColon1 + 1, L':');
    if (!pColon2) return;

    size_t pathLen = (size_t)(pColon1 - resultText);
    wchar_t filePath[MAX_PATH];
    if (pathLen >= MAX_PATH) pathLen = MAX_PATH - 1;
    wcsncpy_s(filePath, MAX_PATH, resultText, pathLen);
    filePath[pathLen] = L'\0';

    int lineNum = _wtoi(pColon1 + 1);
    if (lineNum <= 0) return;

    EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;
    if (!s) return;

    if (_wcsicmp(s->filePath, filePath) != 0) {
        if (!PromptSaveIfModified(s->hwndMain, s)) {
            return;
        }
        if (!LoadFileContent(g_hwndEditor, s, filePath)) {
            return;
        }
    }

    int targetRow = lineNum - 1;
    if (targetRow < 0) targetRow = 0;
    if (targetRow >= s->lineCount) targetRow = s->lineCount - 1;

    s->caret.row = targetRow;
    s->caret.col = 0;
    s->preferredCol = 0;
    ClearSelection(s);

    RECT rc;
    GetClientRect(g_hwndEditor, &rc);
    int visibleLines = (rc.bottom > 0 && s->charHeight > 0) ? (rc.bottom / s->charHeight) : 1;
    int centerRow = targetRow - (visibleLines / 2);
    if (centerRow < 0) centerRow = 0;

    s->scrollY = centerRow * s->charHeight;
    int maxScroll = (s->lineCount - visibleLines) * s->charHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (s->scrollY > maxScroll) s->scrollY = maxScroll;
    s->scrollX = 0;

    FindMatchingBrace(s);
    UpdateScrollbars(g_hwndEditor, s);
    UpdateCaret(g_hwndEditor, s);
    InvalidateRect(g_hwndEditor, NULL, FALSE);
    SetFocus(g_hwndEditor);
}


#define WM_NAVIGATE_OUTPUT (WM_APP + 101)

static WNDPROC g_oldListProc = NULL;

static LRESULT CALLBACK OutputListSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;

    if (msg == WM_ERASEBKGND) {
        return 1;
    }

    if (msg == WM_HSCROLL) {
        SCROLLINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_HORZ, &si);

        int newPos = si.nPos;
        int lineStep = (s && s->charWidth > 0) ? (s->charWidth * 3) : 24;
        int pageStep = (si.nPage > 0) ? (int)si.nPage : 100;

        switch (LOWORD(wParam)) {
        case SB_LINELEFT:      newPos -= lineStep; break;
        case SB_LINERIGHT:     newPos += lineStep; break;
        case SB_PAGELEFT:      newPos -= pageStep; break;
        case SB_PAGERIGHT:     newPos += pageStep; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: newPos = si.nTrackPos; break;
        }

        int maxPos = si.nMax - (int)si.nPage + 1;
        if (maxPos < 0) maxPos = 0;
        if (newPos < 0) newPos = 0;
        if (newPos > maxPos) newPos = maxPos;

        if (newPos != si.nPos) {
            si.fMask = SIF_POS;
            si.nPos = newPos;
            SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }
        return 0; // Bypass default ScrollWindowEx to eliminate text tearing/smearing
    }

    if (msg == WM_VSCROLL) {
        int action = LOWORD(wParam);
        if (action == SB_THUMBTRACK) {
            SCROLLINFO si;
            ZeroMemory(&si, sizeof(si));
            si.cbSize = sizeof(si);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo(hwnd, SB_VERT, &si);
            wParam = MAKEWPARAM(SB_THUMBPOSITION, (WORD)si.nTrackPos);
        }
        LRESULT res = CallWindowProcW(g_oldListProc, hwnd, WM_VSCROLL, wParam, lParam);
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        return res;
    }

    if (msg == WM_MOUSEWHEEL) {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        UINT scrollLines = 3;
        SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
        if (scrollLines == 0) scrollLines = 3;

        int topIdx = (int)SendMessageW(hwnd, LB_GETTOPINDEX, 0, 0);
        int totalItems = (int)SendMessageW(hwnd, LB_GETCOUNT, 0, 0);

        if (delta > 0) {
            topIdx -= (int)scrollLines;
            if (topIdx < 0) topIdx = 0;
        } else {
            topIdx += (int)scrollLines;
            if (topIdx >= totalItems) topIdx = (totalItems > 0 ? totalItems - 1 : 0);
        }

        SendMessageW(hwnd, LB_SETTOPINDEX, (WPARAM)topIdx, 0);
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        return 0;
    }

    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        // 1. Off-screen double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        const Theme* theme = GetActiveTheme(s);
        COLORREF bgDefault = theme ? theme->bg : RGB(30, 30, 30);
        COLORREF selColor  = theme ? theme->selection : RGB(38, 79, 120);
        COLORREF txtColor  = theme ? theme->text : RGB(220, 220, 220);

        // 2. Fill background of entire pane
        HBRUSH bgBrush = CreateSolidBrush(bgDefault);
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);

        HFONT hUseFont = (s && s->hFont) ? s->hFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HGDIOBJ oldFont = SelectObject(memDC, hUseFont);

        SCROLLINFO siH;
        ZeroMemory(&siH, sizeof(siH));
        siH.cbSize = sizeof(siH);
        siH.fMask = SIF_POS;
        GetScrollInfo(hwnd, SB_HORZ, &siH);
        int scrollX = siH.nPos;

        int totalCount = (int)SendMessageW(hwnd, LB_GETCOUNT, 0, 0);
        int topIdx = (int)SendMessageW(hwnd, LB_GETTOPINDEX, 0, 0);
        int curSel = (int)SendMessageW(hwnd, LB_GETCURSEL, 0, 0);
        int itemH = (int)SendMessageW(hwnd, LB_GETITEMHEIGHT, 0, 0);
        if (itemH <= 0) itemH = (s && s->charHeight > 0) ? s->charHeight : 18;

        int y = 0;
        wchar_t text[2048];

        for (int i = topIdx; i < totalCount && y < rc.bottom; i++) {
            RECT lineRc = { 0, y, rc.right, y + itemH };
            BOOL isSelected = (i == curSel);
            COLORREF itemBg = isSelected ? selColor : bgDefault;

            HBRUSH itemBrush = CreateSolidBrush(itemBg);
            FillRect(memDC, &lineRc, itemBrush);
            DeleteObject(itemBrush);

            text[0] = L'\0';
            SendMessageW(hwnd, LB_GETTEXT, i, (LPARAM)text);
            int textLen = (int)wcslen(text);

            if (textLen > 0) {
                SetBkMode(memDC, TRANSPARENT);
                SetTextColor(memDC, txtColor);
                ExtTextOutW(memDC, 4 - scrollX, y, ETO_CLIPPED, &lineRc, text, textLen, NULL);
            }

            y += itemH;
        }

        SelectObject(memDC, oldFont);

        // 3. Atomic blit to screen
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    return CallWindowProcW(g_oldListProc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK OutputPaneProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;

    switch (msg) {
    case WM_CREATE: {
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        g_hwndOutputClose = CreateWindowW(L"BUTTON", L"\x2715",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_CLIPSIBLINGS, 0, 0, 18, 18, hwnd, (HMENU)IDC_OUTPUT_CLOSE, GetModuleHandle(NULL), NULL);
        SendMessage(g_hwndOutputClose, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        g_hwndOutputList = CreateWindowExW(0, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT | WS_CLIPSIBLINGS,
            0, 0, 0, 0, hwnd, (HMENU)IDC_OUTPUT_LIST, GetModuleHandle(NULL), NULL);

        g_oldListProc = (WNDPROC)SetWindowLongPtr(g_hwndOutputList, GWLP_WNDPROC, (LONG_PTR)OutputListSubclassProc);

        int itemH = (s && s->charHeight > 0) ? s->charHeight : 18;
        SendMessage(g_hwndOutputList, LB_SETITEMHEIGHT, 0, itemH);

        if (s && s->hFont) {
            SendMessage(g_hwndOutputList, WM_SETFONT, (WPARAM)s->hFont, TRUE);
        }
        return 0;
    }
    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lParam;
        if (mis->CtlID == IDC_OUTPUT_LIST) {
            mis->itemHeight = (s && s->charHeight > 0) ? s->charHeight : 18;
            return TRUE;
        }
        break;
    }
    case WM_SIZE: {
        int cx = LOWORD(lParam);
        int cy = HIWORD(lParam);
        int btnSize = 18;
        int topMargin = 2;
        int bottomEdge = 4;
        int listH = cy - topMargin - bottomEdge;
        if (listH < 0) listH = 0;

        if (g_hwndOutputClose) MoveWindow(g_hwndOutputClose, cx - btnSize - 2, 2, btnSize, btnSize, TRUE);
        if (g_hwndOutputList) MoveWindow(g_hwndOutputList, 0, topMargin, cx - btnSize - 4, listH, TRUE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_OUTPUT_CLOSE) {
            g_showOutputPane = FALSE;
            ShowWindow(g_hwndOutputPane, SW_HIDE);
            ShowWindow(g_hwndSplitter, SW_HIDE);
            HWND hParent = GetParent(hwnd);
            if (hParent) {
                RECT rc;
                GetClientRect(hParent, &rc);
                SendMessage(hParent, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
            }
            if (g_hwndEditor) SetFocus(g_hwndEditor);
            return 0;
        }
        if (LOWORD(wParam) == IDC_OUTPUT_LIST) {
            if (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_DBLCLK) {
                PostMessage(hwnd, WM_NAVIGATE_OUTPUT, 0, 0);
            }
        }
        break;
    }
    case WM_NAVIGATE_OUTPUT: {
        int curSel = (int)SendMessage(g_hwndOutputList, LB_GETCURSEL, 0, 0);
        if (curSel != LB_ERR) {
            int len = (int)SendMessage(g_hwndOutputList, LB_GETTEXTLEN, curSel, 0);
            if (len > 0) {
                wchar_t* buf = (wchar_t*)malloc((len + 2) * sizeof(wchar_t));
                if (buf) {
                    SendMessageW(g_hwndOutputList, LB_GETTEXT, curSel, (LPARAM)buf);
                    NavigateToResultLine(buf);
                    free(buf);
                }
            }
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK SplitterProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
        return TRUE;
    case WM_LBUTTONDOWN:
        g_isSplitting = TRUE;
        SetCapture(hwnd);
        return 0;
    case WM_MOUSEMOVE:
        if (g_isSplitting) {
            POINT pt;
            GetCursorPos(&pt);
            HWND hParent = GetParent(hwnd);
            ScreenToClient(hParent, &pt);

            RECT rc;
            GetClientRect(hParent, &rc);
            int newHeight = rc.bottom - pt.y;
            if (newHeight < 40) newHeight = 40;
            if (newHeight > rc.bottom - 60) newHeight = rc.bottom - 60;

            g_splitterHeight = newHeight;
            SendMessage(hParent, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));
            UpdateWindow(hParent);
        }
        return 0;
    case WM_LBUTTONUP:
        if (g_isSplitting) {
            g_isSplitting = FALSE;
            ReleaseCapture();
            EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;
            if (s) SaveSettingsToIni(s->hwndMain, s);
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

typedef struct {
    Theme tempDarkTheme;
    Theme tempLightTheme;
    wchar_t tempKeywords[4096];
    wchar_t tempPreproc[2048];
    int activeTab;
    int selectedElement;
    HWND hwndTab;
    HWND hwndList;
    HWND hwndKwEdit;
    HWND hwndPreprocEdit;
    HWND hwndPreview;
    HBRUSH hPreviewBrush;
    BOOL applied;
} ThemeDlgState;

static const wchar_t* THEME_ELEMENT_NAMES[] = {
    L"Background", L"Default Text", L"Keywords", L"Preprocessor Directives",
    L"Comments", L"Strings & Literals", L"Numbers", L"Indentation Guides",
    L"Active Indent Guide", L"Brace Match Border", L"Brace Match Background",
    L"Selection Highlight", L"Gutter Background", L"Gutter Text", L"Gutter Line"
};

static COLORREF* GetDlgElementPtr(ThemeDlgState* ds) {
    Theme* t = (ds->activeTab == 0) ? &ds->tempDarkTheme : &ds->tempLightTheme;
    switch (ds->selectedElement) {
    case 0:  return &t->bg;
    case 1:  return &t->text;
    case 2:  return &t->keyword;
    case 3:  return &t->preprocessor;
    case 4:  return &t->comment;
    case 5:  return &t->string;
    case 6:  return &t->number;
    case 7:  return &t->indentGuide;
    case 8:  return &t->activeIndentGuide;
    case 9:  return &t->braceMatchBorder;
    case 10: return &t->braceMatchBg;
    case 11: return &t->selection;
    case 12: return &t->gutterBg;
    case 13: return &t->gutterText;
    case 14: return &t->gutterLine;
    default: return &t->bg;
    }
}

LRESULT CALLBACK ThemeDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ThemeDlgState* ds = (ThemeDlgState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_NCCREATE: {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        ds = (ThemeDlgState*)cs->lpCreateParams;
        ds->hPreviewBrush = NULL;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ds);
        return TRUE;
    }
    case WM_CREATE: {
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        ds->hwndTab = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            12, 12, 510, 290, hwnd, (HMENU)IDC_DLG_TAB, GetModuleHandle(NULL), NULL);
        SendMessage(ds->hwndTab, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        TCITEMW tie;
        ZeroMemory(&tie, sizeof(tie));
        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)L"Dark Theme";
        TabCtrl_InsertItem(ds->hwndTab, 0, &tie);
        tie.pszText = (LPWSTR)L"Light Theme";
        TabCtrl_InsertItem(ds->hwndTab, 1, &tie);
        TabCtrl_SetCurSel(ds->hwndTab, ds->activeTab);

        HWND hLbl1 = CreateWindowW(L"STATIC", L"Theme Palette Elements:", WS_CHILD | WS_VISIBLE, 25, 45, 180, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLbl1, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        ds->hwndList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 25, 65, 230, 225, hwnd, (HMENU)IDC_DLG_ELEMENT_LIST, NULL, NULL);
        SendMessage(ds->hwndList, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        for (int i = 0; i < 15; i++) SendMessage(ds->hwndList, LB_ADDSTRING, 0, (LPARAM)THEME_ELEMENT_NAMES[i]);
        SendMessage(ds->hwndList, LB_SETCURSEL, 0, 0);
        ds->selectedElement = 0;

        HWND hLbl2 = CreateWindowW(L"STATIC", L"Color Preview:", WS_CHILD | WS_VISIBLE, 275, 45, 140, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLbl2, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        ds->hwndPreview = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_NOTIFY, 275, 65, 140, 36, hwnd, (HMENU)IDC_DLG_PREVIEW_BOX, NULL, NULL);
        
        HWND hBtnColor = CreateWindowW(L"BUTTON", L"Change Color...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            275, 115, 140, 28, hwnd, (HMENU)IDC_DLG_CHOOSE_COLOR, NULL, NULL);
        SendMessage(hBtnColor, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hLbl3 = CreateWindowW(L"STATIC", L"Custom Keywords (space-separated):", WS_CHILD | WS_VISIBLE, 15, 310, 480, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLbl3, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        ds->hwndKwEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", ds->tempKeywords,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, 15, 330, 505, 65, hwnd, (HMENU)IDC_DLG_KW_EDIT, NULL, NULL);
        SendMessage(ds->hwndKwEdit, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hLbl4 = CreateWindowW(L"STATIC", L"Preprocessor Directives (without #):", WS_CHILD | WS_VISIBLE, 15, 400, 480, 18, hwnd, NULL, NULL, NULL);
        SendMessage(hLbl4, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        ds->hwndPreprocEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", ds->tempPreproc,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, 15, 420, 505, 45, hwnd, (HMENU)IDC_DLG_PREPROC_EDIT, NULL, NULL);
        SendMessage(ds->hwndPreprocEdit, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hBtnOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            335, 480, 85, 28, hwnd, (HMENU)IDC_DLG_OK, NULL, NULL);
        SendMessage(hBtnOk, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            435, 480, 85, 28, hwnd, (HMENU)IDC_DLG_CANCEL, NULL, NULL);
        SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)hGuiFont, TRUE);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        if ((HWND)lParam == ds->hwndPreview) {
            COLORREF clr = *GetDlgElementPtr(ds);
            if (ds->hPreviewBrush) DeleteObject(ds->hPreviewBrush);
            ds->hPreviewBrush = CreateSolidBrush(clr);
            return (LRESULT)ds->hPreviewBrush;
        }
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT)); // Explicitly Black (#000000)
        return (LRESULT)GetStockObject(NULL_BRUSH);
        //break;
    }
    case WM_NOTIFY: {
        NMHDR* pNmhdr = (NMHDR*)lParam;
        if (pNmhdr->idFrom == IDC_DLG_TAB && pNmhdr->code == TCN_SELCHANGE) {
            ds->activeTab = TabCtrl_GetCurSel(ds->hwndTab);
            InvalidateRect(ds->hwndPreview, NULL, TRUE);
            UpdateWindow(ds->hwndPreview);
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_DLG_ELEMENT_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
            ds->selectedElement = (int)SendMessage(ds->hwndList, LB_GETCURSEL, 0, 0);
            InvalidateRect(ds->hwndPreview, NULL, TRUE);
            UpdateWindow(ds->hwndPreview);
        } else if (LOWORD(wParam) == IDC_DLG_CHOOSE_COLOR) {
            CHOOSECOLORW cc;
            ZeroMemory(&cc, sizeof(cc));
            static COLORREF customColors[16];
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = customColors;
            cc.rgbResult = *GetDlgElementPtr(ds);
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                *GetDlgElementPtr(ds) = cc.rgbResult;
                InvalidateRect(ds->hwndPreview, NULL, TRUE);
                UpdateWindow(ds->hwndPreview);
            }
        } else if (LOWORD(wParam) == IDC_DLG_OK) {
            GetWindowTextW(ds->hwndKwEdit, ds->tempKeywords, 4096);
            GetWindowTextW(ds->hwndPreprocEdit, ds->tempPreproc, 2048);
            ds->applied = TRUE;
            HWND hParent = GetParent(hwnd);
            if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
            DestroyWindow(hwnd);
        } else if (LOWORD(wParam) == IDC_DLG_CANCEL) {
            HWND hParent = GetParent(hwnd);
            if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_DESTROY:
        if (ds->hPreviewBrush) { DeleteObject(ds->hPreviewBrush); ds->hPreviewBrush = NULL; }
        break;
    case WM_CLOSE: {
        HWND hParent = GetParent(hwnd);
        if (hParent) { EnableWindow(hParent, TRUE); SetForegroundWindow(hParent); }
        DestroyWindow(hwnd);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void ShowThemeSettingsDialog(HWND hwndParent, EditorState* s) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = ThemeDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = SETTINGS_DLG_CLASS;
    RegisterClassEx(&wc);

    ThemeDlgState ds;
    ds.tempDarkTheme = g_darkTheme;
    ds.tempLightTheme = g_lightTheme;
    wcscpy_s(ds.tempKeywords, 4096, g_keywordsBuffer);
    wcscpy_s(ds.tempPreproc, 2048, g_preprocBuffer);
    ds.activeTab = s->isDark ? 0 : 1;
    ds.applied = FALSE;
    ds.hPreviewBrush = NULL;

    RECT rcParent;
    GetWindowRect(hwndParent, &rcParent);
    int x = rcParent.left + 50, y = rcParent.top + 30;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, SETTINGS_DLG_CLASS, L"Theme & Syntax Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, 550, 560, hwndParent, NULL, GetModuleHandle(NULL), &ds
    );

    EnableWindow(hwndParent, FALSE);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);

    if (ds.applied) {
        g_darkTheme = ds.tempDarkTheme;
        g_lightTheme = ds.tempLightTheme;
        wcscpy_s(g_keywordsBuffer, 4096, ds.tempKeywords);
        wcscpy_s(g_preprocBuffer, 2048, ds.tempPreproc);

        if (s) {
            SaveSettingsToIni(s->hwndMain, s);
            UpdateCodeBlocks(s);
            FindMatchingBrace(s);
        }

        // Directly invalidate and update the editor child window immediately
        if (g_hwndEditor) {
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            UpdateWindow(g_hwndEditor);
        }
        if (g_hwndOutputPane) {
            InvalidateRect(g_hwndOutputPane, NULL, TRUE);
            UpdateWindow(g_hwndOutputPane);
        }
        if (g_hwndOutputList) {
            InvalidateRect(g_hwndOutputList, NULL, TRUE);
            UpdateWindow(g_hwndOutputList);
        }
    }
}

static UINT_PTR CALLBACK FontHookProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG: {
        HWND hParent = hdlg;
        HFONT hGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        RECT rcDlg;
        GetWindowRect(hParent, &rcDlg);
        SetWindowPos(hParent, NULL, 0, 0, rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top + 60, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        GetClientRect(hParent, &rcDlg);
        int dlgWidth = rcDlg.right - rcDlg.left;

        HWND hStatic = CreateWindowExW(0, L"STATIC", L"Rendering Quality / ClearType:", WS_CHILD | WS_VISIBLE, 16, rcDlg.bottom - 50, 220, 16, hParent, NULL, GetModuleHandle(NULL), NULL);
        SendMessage(hStatic, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        int comboW = dlgWidth - 32;
        if (comboW < 220) comboW = 220;
        if (comboW > 280) comboW = 280;

        g_hFontQualCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 16, rcDlg.bottom - 30, comboW, 200, hParent, (HMENU)9999, GetModuleHandle(NULL), NULL);
        SendMessage(g_hFontQualCombo, WM_SETFONT, (WPARAM)hGuiFont, TRUE);

        SendMessageW(g_hFontQualCombo, CB_ADDSTRING, 0, (LPARAM)L"ClearType (Subpixel Smoothed)");
        SendMessageW(g_hFontQualCombo, CB_ADDSTRING, 0, (LPARAM)L"ClearType Natural");
        SendMessageW(g_hFontQualCombo, CB_ADDSTRING, 0, (LPARAM)L"Non-Antialiased (Crisp Pixel / Raster)");
        SendMessageW(g_hFontQualCombo, CB_ADDSTRING, 0, (LPARAM)L"Antialiased (Standard Grayscale)");
        SendMessageW(g_hFontQualCombo, CB_ADDSTRING, 0, (LPARAM)L"Default Quality");
        SendMessageW(g_hFontQualCombo, CB_ADDSTRING, 0, (LPARAM)L"Draft Quality");
        SendMessageW(g_hFontQualCombo, CB_ADDSTRING, 0, (LPARAM)L"Proof Quality");

        int sel = 0;
        if (g_selectedFontQuality == CLEARTYPE_NATURAL_QUALITY) sel = 1;
        else if (g_selectedFontQuality == NONANTIALIASED_QUALITY) sel = 2;
        else if (g_selectedFontQuality == ANTIALIASED_QUALITY) sel = 3;
        else if (g_selectedFontQuality == DEFAULT_QUALITY) sel = 4;
        else if (g_selectedFontQuality == DRAFT_QUALITY) sel = 5;
        else if (g_selectedFontQuality == PROOF_QUALITY) sel = 6;

        SendMessage(g_hFontQualCombo, CB_SETCURSEL, sel, 0);
        break;
    }
    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_SELCHANGE && (HWND)lParam == g_hFontQualCombo) {
            int sel = (int)SendMessage(g_hFontQualCombo, CB_GETCURSEL, 0, 0);
            if (sel == 0) g_selectedFontQuality = CLEARTYPE_QUALITY;
            else if (sel == 1) g_selectedFontQuality = CLEARTYPE_NATURAL_QUALITY;
            else if (sel == 2) g_selectedFontQuality = NONANTIALIASED_QUALITY;
            else if (sel == 3) g_selectedFontQuality = ANTIALIASED_QUALITY;
            else if (sel == 4) g_selectedFontQuality = DEFAULT_QUALITY;
            else if (sel == 5) g_selectedFontQuality = DRAFT_QUALITY;
            else if (sel == 6) g_selectedFontQuality = PROOF_QUALITY;
        }
        break;
    }
    }
    return 0;
}

static LRESULT CALLBACK CustomEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditorState* s = (EditorState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_NCCREATE: {
        s = (EditorState*)calloc(1, sizeof(EditorState));
        s->lineCapacity = 16;
        s->lineCount = 1;
        s->tabSize = 2;
        s->useSpaces = TRUE;
        s->zoomLevel = 0;
        s->lines = (Line*)malloc(s->lineCapacity * sizeof(Line));
        Line_Init(&s->lines[0]);
        LoadSettingsFromIni(s);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)s);
        return TRUE;
    }

    case WM_CREATE: {
        DragAcceptFiles(hwnd, TRUE);
        RecreateEditorFont(hwnd, s);

        s->encodingIdx = 0;
        s->caret.row = 0;
        s->caret.col = 0;
        s->preferredCol = 0;
        ClearSelection(s);
        SetModified(s, FALSE);
        UpdateCodeBlocks(s);
        RecalculateMaxCols(s);
        RecalculateGutter(s);
        FindMatchingBrace(s);
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            if (s && s->isDraggingText) {
                if (!g_hDragCursor) g_hDragCursor = CreateDragCursor();
                SetCursor(g_hDragCursor);
                return TRUE;
            }
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            if (pt.x < s->gutterWidth) {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            } else {
                TextPos pos = ScreenToTextPos(s, pt.x, pt.y);
                if (IsPosInSelection(s, pos)) SetCursor(LoadCursor(NULL, IDC_ARROW));
                else SetCursor(LoadCursor(NULL, IDC_IBEAM));
            }
            return TRUE;
        }
        break;
    }

    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        wchar_t filePath[MAX_PATH];
        if (DragQueryFileW(hDrop, 0, filePath, MAX_PATH)) {
            if (PromptSaveIfModified(s->hwndMain, s)) LoadFileContent(hwnd, s, filePath);
        }
        DragFinish(hDrop);
        return 0;
    }

    case WM_SETFOCUS: {
        int ch = (s && s->charHeight > 0) ? s->charHeight : 18;
        DestroyCaret();
        CreateCaret(hwnd, NULL, 2, ch);
        UpdateCaret(hwnd, s);
        ShowCaret(hwnd);
        return 0;
    }

    case WM_KILLFOCUS:
        s->isCtrlSolo = FALSE;
        KillTimer(hwnd, IDT_AUTOSCROLL);
        HideCaret(hwnd);
        DestroyCaret();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        const Theme* theme = GetActiveTheme(s);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);
        HGDIOBJ oldFont = SelectObject(memDC, s->hFont);

        HBRUSH bgBrush = CreateSolidBrush(theme->bg);
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);
        
        // Compute once per frame here:
        int activeBlockIdx = GetActiveBlockIndex(s);

        HRGN textRgn = CreateRectRgn(s->gutterWidth, 0, rc.right, rc.bottom);
        SelectClipRgn(memDC, textRgn);
        DeleteObject(textRgn);

        int y = -s->scrollY;
        for (int i = 0; i < s->lineCount; i++) {
            if (y + s->charHeight >= 0 && y < rc.bottom) {
                PaintSyntaxLine(memDC, s, i, y, &rc, theme, activeBlockIdx);
            }
            y += s->charHeight;
        }

        SelectClipRgn(memDC, NULL);

        if (s->showLineNumbers) {
            RECT gutterRc = { 0, 0, s->gutterWidth, rc.bottom };
            HBRUSH gutterBrush = CreateSolidBrush(theme->gutterBg);
            FillRect(memDC, &gutterRc, gutterBrush);
            DeleteObject(gutterBrush);

            HPEN borderPen = CreatePen(PS_SOLID, 1, theme->gutterLine);
            HGDIOBJ oldPen = SelectObject(memDC, borderPen);
            MoveToEx(memDC, s->gutterWidth - 1, 0, NULL);
            LineTo(memDC, s->gutterWidth - 1, rc.bottom);
            SelectObject(memDC, oldPen);
            DeleteObject(borderPen);

            SetBkMode(memDC, TRANSPARENT);
            y = -s->scrollY;
            for (int i = 0; i < s->lineCount; i++) {
                if (y + s->charHeight >= 0 && y < rc.bottom) {
                    wchar_t lineNumStr[16];
                    wsprintf(lineNumStr, L"%d", i + 1);
                    SetTextColor(memDC, theme->gutterText);
                    RECT numRc = { 0, y, s->gutterWidth - s->charWidth, y + s->charHeight };
                    DrawTextW(memDC, lineNumStr, -1, &numRc, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
                }
                y += s->charHeight;
            }
        }

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldFont);
        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
      
        s->isCtrlSolo = FALSE;
      
        SetFocus(hwnd);
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (s->showLineNumbers && x < s->gutterWidth) {
            SetCapture(hwnd);
            s->isMouseDown = TRUE;
            SetTimer(hwnd, IDT_AUTOSCROLL, 40, NULL);
            int clickRow = (y + s->scrollY) / s->charHeight;
            if (clickRow < 0) clickRow = 0;
            if (clickRow >= s->lineCount) clickRow = s->lineCount - 1;

            s->selAnchor.row = clickRow;
            s->selAnchor.col = 0;
            if (clickRow + 1 < s->lineCount) {
                s->caret.row = clickRow + 1;
                s->caret.col = 0;
            } else {
                s->caret.row = clickRow;
                s->caret.col = (int64_t)s->lines[clickRow].length;
            }
            s->preferredCol = s->caret.col;
            s->isGutterSelecting = TRUE;
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        TextPos clickPos = ScreenToTextPos(s, x, y);

        if (HasSelection(s)) {
            if (!(GetKeyState(VK_SHIFT) & 0x8000) && IsPosInSelection(s, clickPos)) {
                s->isPendingDrag = TRUE;
                s->dragStartPt.x = x;
                s->dragStartPt.y = y;
                SetCapture(hwnd);
                return 0;
            }
        }

        SetCapture(hwnd);
        s->isMouseDown = TRUE;
        SetTimer(hwnd, IDT_AUTOSCROLL, 40, NULL);

        DWORD clickTime = GetMessageTime();
        int cx = abs(x - s->lastDblClickPt.x);
        int cy = abs(y - s->lastDblClickPt.y);
        if ((clickTime - s->lastDblClickTime) <= GetDoubleClickTime() &&
            cx <= GetSystemMetrics(SM_CXDOUBLECLK) && cy <= GetSystemMetrics(SM_CYDOUBLECLK)) {
            
            TextPos pos = ScreenToTextPos(s, x, y);
            s->selAnchor.row = pos.row;
            s->selAnchor.col = 0;
            if (pos.row + 1 < s->lineCount) {
                s->caret.row = pos.row + 1;
                s->caret.col = 0;
            } else {
                s->caret.row = pos.row;
                s->caret.col = (int64_t)s->lines[pos.row].length;
            }
            s->preferredCol = s->caret.col;
            s->lastDblClickTime = 0;
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        s->caret = clickPos;
        s->preferredCol = s->caret.col;
        if (!(GetKeyState(VK_SHIFT) & 0x8000)) ClearSelection(s);
        FindMatchingBrace(s);
        UpdateCaret(hwnd, s);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        s->lastDblClickTime = GetMessageTime();
        s->lastDblClickPt.x = x;
        s->lastDblClickPt.y = y;

        if (!s->showLineNumbers || x >= s->gutterWidth) {
            TextPos pos = ScreenToTextPos(s, x, y);
            Line* line = &s->lines[pos.row];
            if (line->length > 0) {
                size_t col = (size_t)pos.col;
                if (col >= line->length) col = line->length - 1;
                wchar_t c = line->text[col];

                if (IsIdentChar(c)) {
                    size_t start = col;
                    while (start > 0 && IsIdentChar(line->text[start - 1])) start--;
                    size_t end = col;
                    while (end < line->length && IsIdentChar(line->text[end])) end++;
                    s->selAnchor.row = pos.row; s->selAnchor.col = (int64_t)start;
                    s->caret.row = pos.row; s->caret.col = (int64_t)end;
                } else if (c == L' ') {
                    size_t start = col;
                    while (start > 0 && line->text[start - 1] == L' ') start--;
                    size_t end = col;
                    while (end < line->length && line->text[end] == L' ') end++;
                    s->selAnchor.row = pos.row; s->selAnchor.col = (int64_t)start;
                    s->caret.row = pos.row; s->caret.col = (int64_t)end;
                } else {
                    s->selAnchor.row = pos.row; s->selAnchor.col = (int64_t)col;
                    s->caret.row = pos.row; s->caret.col = (int64_t)col + 1;
                }
                s->preferredCol = s->caret.col;
                UpdateCaret(hwnd, s);
                FindMatchingBrace(s);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (s->isPendingDrag) {
            if (abs(x - s->dragStartPt.x) > GetSystemMetrics(SM_CXDRAG) ||
                abs(y - s->dragStartPt.y) > GetSystemMetrics(SM_CYDRAG)) {
                s->isPendingDrag = FALSE;
                s->isDraggingText = TRUE;
                s->dragSelAnchor = s->selAnchor;
                s->dragSelCaret = s->caret;
                s->dragDropPos = ScreenToTextPos(s, x, y);
                SetTimer(hwnd, IDT_AUTOSCROLL, 40, NULL);
                if (!g_hDragCursor) g_hDragCursor = CreateDragCursor();
                SetCursor(g_hDragCursor);
            }
        }

        if (s->isDraggingText) {
            if (!g_hDragCursor) g_hDragCursor = CreateDragCursor();
            SetCursor(g_hDragCursor);
            s->dragDropPos = ScreenToTextPos(s, x, y);
            EnsureCaretVisible(hwnd, s);
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (s->isMouseDown) {
            if (s->isGutterSelecting) {
                int curRow = (y + s->scrollY) / s->charHeight;
                if (curRow < 0) curRow = 0;
                if (curRow >= s->lineCount) curRow = s->lineCount - 1;

                if (curRow >= s->selAnchor.row) {
                    if (curRow + 1 < s->lineCount) { s->caret.row = curRow + 1; s->caret.col = 0; }
                    else { s->caret.row = curRow; s->caret.col = (int64_t)s->lines[curRow].length; }
                } else { s->caret.row = curRow; s->caret.col = 0; }
            } else {
                s->caret = ScreenToTextPos(s, x, y);
            }

            s->preferredCol = s->caret.col;
            EnsureCaretVisible(hwnd, s);
            FindMatchingBrace(s);
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_TIMER: {
        if (wParam == IDT_AUTOSCROLL && (s->isMouseDown || s->isDraggingText)) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            RECT rc;
            GetClientRect(hwnd, &rc);

            BOOL scrolled = FALSE;
            if (pt.y < 0) {
                s->scrollY -= s->charHeight;
                if (s->scrollY < 0) s->scrollY = 0;
                scrolled = TRUE;
            } else if (pt.y > rc.bottom) {
                int maxScroll = (s->lineCount - (s->visibleLines > 0 ? s->visibleLines : 1)) * s->charHeight;
                if (maxScroll < 0) maxScroll = 0;
                s->scrollY += s->charHeight;
                if (s->scrollY > maxScroll) s->scrollY = maxScroll;
                scrolled = TRUE;
            }

            int leftBound = s->gutterWidth + TEXT_MARGIN_X;
            if (pt.x < leftBound && s->scrollX > 0) {
                s->scrollX -= 2;
                if (s->scrollX < 0) s->scrollX = 0;
                scrolled = TRUE;
            } else if (pt.x > rc.right) {
                int clientW = rc.right - (s->gutterWidth + TEXT_MARGIN_X);
                int charW = (s->charWidth > 0) ? s->charWidth : 8;
                int64_t visCols = (clientW > 0 && charW > 0) ? (clientW / charW) : 1;
                int64_t maxScrollX = s->maxCols - visCols + 5;
                if (maxScrollX < 0) maxScrollX = 0;
                s->scrollX += 2;
                if (s->scrollX > maxScrollX) s->scrollX = maxScrollX;
                scrolled = TRUE;
            }

            if (scrolled || pt.y < 0 || pt.y > rc.bottom) {
                if (s->isDraggingText) s->dragDropPos = ScreenToTextPos(s, pt.x, pt.y);
                else if (s->isGutterSelecting) {
                    int curRow = (pt.y + s->scrollY) / s->charHeight;
                    if (curRow < 0) curRow = 0;
                    if (curRow >= s->lineCount) curRow = s->lineCount - 1;

                    if (curRow >= s->selAnchor.row) {
                        if (curRow + 1 < s->lineCount) { s->caret.row = curRow + 1; s->caret.col = 0; }
                        else { s->caret.row = curRow; s->caret.col = (int64_t)s->lines[curRow].length; }
                    } else { s->caret.row = curRow; s->caret.col = 0; }
                    s->preferredCol = s->caret.col;
                } else {
                    s->caret = ScreenToTextPos(s, pt.x, pt.y);
                    s->preferredCol = s->caret.col;
                }
                UpdateScrollbars(hwnd, s);
                UpdateCaret(hwnd, s);
                FindMatchingBrace(s);
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_LBUTTONUP: {

        if (s->isPendingDrag) {
            s->isPendingDrag = FALSE;
            ReleaseCapture();
            s->caret = ScreenToTextPos(s, s->dragStartPt.x, s->dragStartPt.y);
            s->preferredCol = s->caret.col;
            ClearSelection(s);
            FindMatchingBrace(s);
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (s->isDraggingText) {
            s->isDraggingText = FALSE;
            KillTimer(hwnd, IDT_AUTOSCROLL);
            ReleaseCapture();

            s->selAnchor = s->dragSelAnchor;
            s->caret = s->dragSelCaret;

            TextPos dropPos = s->dragDropPos;
            TextPos start, end;
            GetSelectionRange(s, &start, &end);

            if (PosCompare(dropPos, start) >= 0 && PosCompare(dropPos, end) <= 0) {
                s->caret = dropPos;
                s->preferredCol = s->caret.col;
                ClearSelection(s);
                FindMatchingBrace(s);
                UpdateCaret(hwnd, s);
                InvalidateRect(hwnd, NULL, FALSE);
            } else {
                size_t totalLen = 0;
                for (int r = start.row; r <= end.row; r++) {
                    size_t c1 = (r == start.row) ? (size_t)start.col : 0;
                    size_t c2 = (r == end.row) ? (size_t)end.col : s->lines[r].length;
                    totalLen += (c2 - c1);
                    if (r < end.row) totalLen += 2;
                }

                wchar_t* movedText = (wchar_t*)malloc((totalLen + 1) * sizeof(wchar_t));
                if (movedText) {
                    wchar_t* pCur = movedText;
                    for (int r = start.row; r <= end.row; r++) {
                        size_t c1 = (r == start.row) ? (size_t)start.col : 0;
                        size_t c2 = (r == end.row) ? (size_t)end.col : s->lines[r].length;
                        size_t count = c2 - c1;
                        if (count > 0 && s->lines[r].text) {
                            memcpy(pCur, &s->lines[r].text[c1], count * sizeof(wchar_t));
                            pCur += count;
                        }
                        if (r < end.row) { *pCur++ = L'\r'; *pCur++ = L'\n'; }
                    }
                    *pCur = L'\0';

                    TextPos targetPos;
                    if (PosCompare(dropPos, start) < 0) targetPos = dropPos;
                    else {
                        if (dropPos.row == end.row) {
                            targetPos.row = start.row;
                            targetPos.col = start.col + (dropPos.col - end.col);
                        } else {
                            targetPos.row = dropPos.row - (end.row - start.row);
                            targetPos.col = dropPos.col;
                        }
                    }

                    PushUndo(s);

                    if (start.row == end.row) {
                        Line_DeleteChars(&s->lines[start.row], (size_t)start.col, (size_t)(end.col - start.col));
                    } else {
                        s->lines[start.row].length = (size_t)start.col;
                        s->lines[start.row].text[start.col] = L'\0';
                        Line* endLine = &s->lines[end.row];
                        for (size_t i = (size_t)end.col; i < endLine->length; i++) Line_InsertChar(&s->lines[start.row], s->lines[start.row].length, endLine->text[i]);
                        for (int r = end.row; r > start.row; r--) Editor_DeleteLine(s, r);
                    }

                    s->caret = targetPos;
                    const wchar_t* p = movedText;
                    TextPos insertStart = s->caret;
                    while (*p) {
                        if (*p == L'\r' && *(p + 1) == L'\n') {
                            size_t tailLen = (s->lines[s->caret.row].length >= (size_t)s->caret.col) ? (s->lines[s->caret.row].length - (size_t)s->caret.col) : 0;
                            Editor_InsertLine(s, s->caret.row + 1);
                            Line* next = &s->lines[s->caret.row + 1];
                            for (size_t i = 0; i < tailLen; i++) Line_InsertChar(next, i, s->lines[s->caret.row].text[(size_t)s->caret.col + i]);
                            s->lines[s->caret.row].length = (size_t)s->caret.col;
                            s->lines[s->caret.row].text[s->caret.col] = L'\0';
                            s->caret.row++;
                            s->caret.col = 0;
                            p += 2;
                        } else if (*p == L'\n' || *p == L'\r') {
                            size_t tailLen = (s->lines[s->caret.row].length >= (size_t)s->caret.col) ? (s->lines[s->caret.row].length - (size_t)s->caret.col) : 0;
                            Editor_InsertLine(s, s->caret.row + 1);
                            Line* next = &s->lines[s->caret.row + 1];
                            for (size_t i = 0; i < tailLen; i++) Line_InsertChar(next, i, s->lines[s->caret.row].text[(size_t)s->caret.col + i]);
                            s->lines[s->caret.row].length = (size_t)s->caret.col;
                            s->lines[s->caret.row].text[s->caret.col] = L'\0';
                            s->caret.row++;
                            s->caret.col = 0;
                            p++;
                        } else {
                            Line_InsertChar(&s->lines[s->caret.row], (size_t)s->caret.col++, *p++);
                        }
                    }
                    TextPos insertEnd = s->caret;
                    free(movedText);

                    s->selAnchor = insertStart;
                    s->caret = insertEnd;
                    s->preferredCol = s->caret.col;
                    SetModified(s, TRUE);
                    UpdateCodeBlocks(s);
                    RecalculateMaxCols(s);
                    RecalculateGutter(s);
                    FindMatchingBrace(s);
                    EnsureCaretVisible(hwnd, s);
                    UpdateScrollbars(hwnd, s);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            return 0;
        }

        if (s->isMouseDown) {
            s->isMouseDown = FALSE;
            s->isGutterSelecting = FALSE;
            KillTimer(hwnd, IDT_AUTOSCROLL);
            ReleaseCapture();
        }
        return 0;
    }

    case WM_CONTEXTMENU: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        ShowContextMenu(hwnd, s, x, y);
        return 0;
    }

    case WM_COMMAND: {
        int cmd = LOWORD(wParam);
        if (cmd >= IDM_ENC_BASE && cmd < IDM_ENC_BASE + ENCODING_COUNT) {
            int newEncIdx = cmd - IDM_ENC_BASE;
            s->encodingIdx = newEncIdx;
            UpdateEncodingMenuCheck(s->hwndMain, s->encodingIdx);
            SetModified(s, TRUE);
            UpdateStatusBar(s);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        switch (cmd) {
        case IDM_EDIT_UNDO:      Editor_Undo(hwnd, s); break;
        case IDM_EDIT_REDO:      Editor_Redo(hwnd, s); break;
        case IDM_EDIT_CUT:       Editor_Cut(hwnd, s); break;
        case IDM_EDIT_COPY:      Editor_Copy(hwnd, s); break;
        case IDM_EDIT_PASTE:     Editor_Paste(hwnd, s); break;
        case IDM_EDIT_DELETE:
            if (HasSelection(s)) {
                Editor_DeleteSelection(s);
                UpdateCodeBlocks(s);
                FindMatchingBrace(s);
                EnsureCaretVisible(hwnd, s);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        case IDM_EDIT_SELECTALL:
            s->selAnchor.row = 0; s->selAnchor.col = 0;
            s->caret.row = s->lineCount - 1;
            s->caret.col = (int64_t)s->lines[s->caret.row].length;
            s->preferredCol = s->caret.col;
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_EDIT_UPPERCASE: Editor_MakeSelectionCase(s, TRUE); break;
        case IDM_EDIT_LOWERCASE: Editor_MakeSelectionCase(s, FALSE); break;
        case IDM_SEARCH_FIND:    ShowFindDialog(s->hwndMain, s); break;
        case IDM_SEARCH_FINDNEXT: FindNextText(s, TRUE); break;
        case IDM_SEARCH_FINDPREV: FindNextText(s, FALSE); break;
        case IDM_SEARCH_FINDINFILES: ShowFindInFilesDialog(s->hwndMain, s); break;
        case IDM_SEARCH_REPLACE: ShowReplaceDialog(s->hwndMain, s); break;
        case IDM_SEARCH_GOTO:    ShowGotoDialog(s->hwndMain, s); break;
        case IDM_VIEW_DARK:
            s->isDark = TRUE;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_DARK, MF_CHECKED);
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_LIGHT, MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            UpdateCodeBlocks(s);
            FindMatchingBrace(s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            if (g_hwndOutputPane) InvalidateRect(g_hwndOutputPane, NULL, TRUE);
            if (g_hwndOutputList) InvalidateRect(g_hwndOutputList, NULL, TRUE);
            break;
        case IDM_VIEW_LIGHT:
            s->isDark = FALSE;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_LIGHT, MF_CHECKED);
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_DARK, MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            UpdateCodeBlocks(s);
            FindMatchingBrace(s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            if (g_hwndOutputPane) InvalidateRect(g_hwndOutputPane, NULL, TRUE);
            if (g_hwndOutputList) InvalidateRect(g_hwndOutputList, NULL, TRUE);
            break;
        case IDM_VIEW_GUIDES:
            s->showIndentGuides = !s->showIndentGuides;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_GUIDES, s->showIndentGuides ? MF_CHECKED : MF_UNCHECKED);
            EnableMenuItem(GetMenu(s->hwndMain), IDM_VIEW_HIGHLIGHT_GUIDES, s->showIndentGuides ? MF_ENABLED : MF_GRAYED);
            SaveSettingsToIni(s->hwndMain, s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            break;
        case IDM_VIEW_HIGHLIGHT_GUIDES:
            s->highlightIndentGuides = !s->highlightIndentGuides;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_HIGHLIGHT_GUIDES, s->highlightIndentGuides ? MF_CHECKED : MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            break;
        case IDM_VIEW_BRACES:
            s->showBraceMatch = !s->showBraceMatch;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_BRACES, s->showBraceMatch ? MF_CHECKED : MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            FindMatchingBrace(s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            break;
        case IDM_VIEW_LINENUMBERS:
            s->showLineNumbers = !s->showLineNumbers;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_LINENUMBERS, s->showLineNumbers ? MF_CHECKED : MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            RecalculateGutter(s);
            UpdateScrollbars(hwnd, s);
            UpdateCaret(hwnd, s);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case IDM_INDENT_SIZE_2:
        case IDM_INDENT_SIZE_4:
        case IDM_INDENT_SIZE_8: {
            int newSize = (cmd == IDM_INDENT_SIZE_2) ? 2 : (cmd == IDM_INDENT_SIZE_4 ? 4 : 8);
            if (s->tabSize != newSize) {
                int oldSize = s->tabSize;
                s->tabSize = newSize;
                Editor_ConvertIndentation(s, oldSize, newSize);
                CheckMenuRadioItem(GetMenu(s->hwndMain), IDM_INDENT_SIZE_2, IDM_INDENT_SIZE_8, cmd, MF_BYCOMMAND);
                SaveSettingsToIni(s->hwndMain, s);
                EnsureCaretVisible(hwnd, s);
                UpdateScrollbars(hwnd, s);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case IDM_INDENT_SPACES: {
            s->useSpaces = !s->useSpaces;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_INDENT_SPACES, s->useSpaces ? MF_CHECKED : MF_UNCHECKED);
            SetModified(s, TRUE);
            SaveSettingsToIni(s->hwndMain, s);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_VIEW_FONT: {
            CHOOSEFONTW cf;
            ZeroMemory(&cf, sizeof(cf));
            LOGFONTW lf;
            ZeroMemory(&lf, sizeof(lf));

            HDC hdc = GetDC(hwnd);
            int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(hwnd, hdc);

            // Initialize dialog using the true base point size, not the temporary zoomed height
            lf.lfHeight = -MulDiv(s->pointSize, logPixelsY, 72);
            lf.lfWeight = s->fontBold ? FW_BOLD : FW_NORMAL;
            lf.lfCharSet = DEFAULT_CHARSET;
            lf.lfQuality = (BYTE)s->fontQuality;
            lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
            wcscpy_s(lf.lfFaceName, 64, s->fontFace);

            cf.lStructSize = sizeof(cf);
            cf.hwndOwner = hwnd;
            cf.lpLogFont = &lf;
            cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_ENABLEHOOK;
            cf.lpfnHook = FontHookProc;
            g_selectedFontQuality = s->fontQuality;

            if (ChooseFontW(&cf)) {
                wcscpy_s(s->fontFace, 64, lf.lfFaceName);
                s->pointSize = cf.iPointSize / 10;
                s->fontBold = (lf.lfWeight >= FW_BOLD);
                s->fontQuality = g_selectedFontQuality;
                s->zoomLevel = 0; // Reset zoom level so the chosen size is the clean baseline
                SaveSettingsToIni(s->hwndMain, s);
                RecreateEditorFont(hwnd, s);
            }
            break;
        }
        case IDM_VIEW_THEME_DLG:
            ShowThemeSettingsDialog(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_TOOLS_ENCRYPT: Tool_Encrypt(g_hwndEditor, s); break;
        case IDM_TOOLS_DECRYPT: Tool_Decrypt(g_hwndEditor, s); break;
        case IDM_HELP_ABOUT:    ShowAboutDialog(hwnd); break;
        case IDM_FILE_EXIT:     SendMessage(hwnd, WM_CLOSE, 0, 0); break;
        default:
            break;
        }
        return 0;
    }

    case WM_KEYUP: {
      if (wParam == VK_CONTROL) {
        // Only jump if Ctrl was tapped solo AND no text is currently selected
        if (s && s->isCtrlSolo && !HasSelection(s)) {
            s->isCtrlSolo = FALSE;
            EnsureCaretVisible(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        if (s) s->isCtrlSolo = FALSE;
      }
      return 0;
    }
    
    case WM_KEYDOWN: {
                
        if (wParam == VK_CONTROL) {
            // Only set true on initial press (ignore auto-repeat bit 30)
            if ((lParam & (1 << 30)) == 0) {
                s->isCtrlSolo = TRUE;
            }
            return 0; // <--- MUST RETURN HERE so it doesn't hit EnsureCaretVisible below!
        } else {
            // Any other key pressed (e.g. 'C', 'V', 'Z', 'F') cancels the solo Ctrl jump
            s->isCtrlSolo = FALSE;
            
            // Also keep bare Shift and Alt silent on KeyDown,
            // only bare CTRL has the function to scroll viewport to caret:
            if (wParam == VK_SHIFT || wParam == VK_MENU)
                return 0;
        }
      
        BOOL ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (ctrl) {
            if (wParam == 'Z' || wParam == 'z') {
                if (shift) SendMessage(hwnd, WM_COMMAND, IDM_EDIT_REDO, 0);
                else SendMessage(hwnd, WM_COMMAND, IDM_EDIT_UNDO, 0);
                return 0;
            } else if (wParam == 'Y' || wParam == 'y') {
                SendMessage(hwnd, WM_COMMAND, IDM_EDIT_REDO, 0);
                return 0;
            } else if (wParam == 'U' || wParam == 'u') {
                if (shift) SendMessage(hwnd, WM_COMMAND, IDM_EDIT_UPPERCASE, 0);
                else SendMessage(hwnd, WM_COMMAND, IDM_EDIT_LOWERCASE, 0);
                return 0;
            } else if (wParam == 'F' || wParam == 'f') {
                if (shift) SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_FINDINFILES, 0);
                else SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_FIND, 0);
                return 0;
            } else if (wParam == 'H' || wParam == 'h') {
                SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_REPLACE, 0);
                return 0;
            } else if (wParam == 'G' || wParam == 'g') {
                SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_GOTO, 0);
                return 0;
            } else if (wParam == '1') {
                SendMessage(hwnd, WM_COMMAND, IDM_TOOLS_ENCRYPT, 0);
                return 0;
            } else if (wParam == '2') {
                SendMessage(hwnd, WM_COMMAND, IDM_TOOLS_DECRYPT, 0);
                return 0;
            } else if (wParam == 'A' || wParam == 'a') {
                SendMessage(hwnd, WM_COMMAND, IDM_EDIT_SELECTALL, 0);
                return 0;
            } else if (wParam == 'C' || wParam == 'c') {
                SendMessage(hwnd, WM_COMMAND, IDM_EDIT_COPY, 0);
                return 0;
            } else if (wParam == 'V' || wParam == 'v') {
                SendMessage(hwnd, WM_COMMAND, IDM_EDIT_PASTE, 0);
                return 0;
            } else if (wParam == 'X' || wParam == 'x') {
                SendMessage(hwnd, WM_COMMAND, IDM_EDIT_CUT, 0);
                return 0;
            }
        }

        switch (wParam) {
        case VK_ESCAPE:
            if (HasSelection(s)) {
                ClearSelection(s);
                FindMatchingBrace(s);
                EnsureCaretVisible(hwnd, s);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case VK_F3:
            if (shift) SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_FINDPREV, 0);
            else SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_FINDNEXT, 0);
            return 0;

        case VK_PRIOR: {
            int linesToJump = s->visibleLines > 1 ? s->visibleLines : 1;
            s->caret.row -= linesToJump;
            if (s->caret.row < 0) s->caret.row = 0;
            if ((size_t)s->caret.col > s->lines[s->caret.row].length) s->caret.col = (int64_t)s->lines[s->caret.row].length;
            s->preferredCol = s->caret.col;
            s->scrollY -= linesToJump * s->charHeight;
            if (s->scrollY < 0) s->scrollY = 0;
            if (!shift) ClearSelection(s);
            break;
        }

        case VK_NEXT: {
            int linesToJump = s->visibleLines > 1 ? s->visibleLines : 1;
            s->caret.row += linesToJump;
            if (s->caret.row >= s->lineCount) s->caret.row = s->lineCount - 1;
            if ((size_t)s->caret.col > s->lines[s->caret.row].length) s->caret.col = (int64_t)s->lines[s->caret.row].length;
            s->preferredCol = s->caret.col;
            s->scrollY -= linesToJump * s->charHeight;
            int maxScroll = (s->lineCount - (s->visibleLines > 0 ? s->visibleLines : 1)) * s->charHeight;
            if (maxScroll < 0) maxScroll = 0;
            if (s->scrollY > maxScroll) s->scrollY = maxScroll;
            if (!shift) ClearSelection(s);
            break;
        }

        case VK_LEFT:
            if (ctrl) {
                Line* cur = &s->lines[s->caret.row];
                size_t c = (size_t)s->caret.col;
                if (c == 0) {
                    if (s->caret.row > 0) { s->caret.row--; s->caret.col = (int64_t)s->lines[s->caret.row].length; }
                } else {
                    while (c > 0 && (cur->text[c - 1] == L' ' || cur->text[c - 1] == L'\t')) c--;
                    if (c > 0) {
                        if (IsIdentChar(cur->text[c - 1])) { while (c > 0 && IsIdentChar(cur->text[c - 1])) c--; }
                        else { while (c > 0 && !IsIdentChar(cur->text[c - 1]) && cur->text[c - 1] != L' ' && cur->text[c - 1] != L'\t') c--; }
                    }
                    s->caret.col = (int64_t)c;
                }
            } else {
                if (s->caret.col > 0) s->caret.col--;
                else if (s->caret.row > 0) { s->caret.row--; s->caret.col = (int64_t)s->lines[s->caret.row].length; }
            }
            s->preferredCol = s->caret.col;
            if (!shift) ClearSelection(s);
            break;

        case VK_RIGHT:
            if (ctrl) {
                Line* cur = &s->lines[s->caret.row];
                size_t c = (size_t)s->caret.col;
                size_t len = cur->length;
                if (c >= len) {
                    if (s->caret.row + 1 < s->lineCount) { s->caret.row++; s->caret.col = 0; }
                } else {
                    if (IsIdentChar(cur->text[c])) { while (c < len && IsIdentChar(cur->text[c])) c++; }
                    else if (cur->text[c] != L' ' && cur->text[c] != L'\t') {
                        while (c < len && !IsIdentChar(cur->text[c]) && cur->text[c] != L' ' && cur->text[c] != L'\t') c++;
                    }
                    while (c < len && (cur->text[c] == L' ' || cur->text[c] == L'\t')) c++;
                    s->caret.col = (int64_t)c;
                }
            } else {
                if ((size_t)s->caret.col < s->lines[s->caret.row].length) s->caret.col++;
                else if (s->caret.row + 1 < s->lineCount) { s->caret.row++; s->caret.col = 0; }
            }
            s->preferredCol = s->caret.col;
            if (!shift) ClearSelection(s);
            break;

        case VK_UP:
            if (s->caret.row > 0) {
                s->caret.row--;
                s->caret.col = s->preferredCol;
                if ((size_t)s->caret.col > s->lines[s->caret.row].length) s->caret.col = (int64_t)s->lines[s->caret.row].length;
            }
            if (!shift) ClearSelection(s);
            break;

        case VK_DOWN:
            if (s->caret.row + 1 < s->lineCount) {
                s->caret.row++;
                s->caret.col = s->preferredCol;
                if ((size_t)s->caret.col > s->lines[s->caret.row].length) s->caret.col = (int64_t)s->lines[s->caret.row].length;
            }
            if (!shift) ClearSelection(s);
            break;

        case VK_HOME: {
            Line* cur = &s->lines[s->caret.row];
            size_t firstChar = 0;
            while (firstChar < cur->length && (cur->text[firstChar] == L' ' || cur->text[firstChar] == L'\t')) firstChar++;
            if (firstChar >= cur->length) firstChar = 0;
            if (s->caret.col == (int64_t)firstChar) s->caret.col = 0;
            else s->caret.col = (int64_t)firstChar;
            s->preferredCol = s->caret.col;
            if (!shift) ClearSelection(s);
            break;
        }

        case VK_END: {
            Line* cur = &s->lines[s->caret.row];
            size_t lastChar = cur->length;
            while (lastChar > 0 && (cur->text[lastChar - 1] == L' ' || cur->text[lastChar - 1] == L'\t')) lastChar--;
            if (s->caret.col == (int64_t)lastChar) s->caret.col = (int64_t)cur->length;
            else s->caret.col = (int64_t)lastChar;
            s->preferredCol = s->caret.col;
            if (!shift) ClearSelection(s);
            break;
        }

        case VK_DELETE:
            if (HasSelection(s)) {
                Editor_DeleteSelection(s);
            } else {
                Line* cur = &s->lines[s->caret.row];
                if ((size_t)s->caret.col < cur->length) {
                    PushUndo(s);
                    size_t spacesToDelete = 1;
                    int tabWidth = (s->tabSize > 0) ? s->tabSize : 2;
                    if (cur->text[s->caret.col] == L' ') {
                        size_t rem = (size_t)tabWidth - ((size_t)s->caret.col % tabWidth);
                        if (rem == 0) rem = (size_t)tabWidth;
                        BOOL allSpaces = TRUE;
                        for (size_t i = 0; i < rem; i++) {
                            if ((size_t)s->caret.col + i >= cur->length || cur->text[(size_t)s->caret.col + i] != L' ') { allSpaces = FALSE; break; }
                        }
                        if (allSpaces) spacesToDelete = rem;
                    }
                    Line_DeleteChars(cur, (size_t)s->caret.col, spacesToDelete);
                    SetModified(s, TRUE);
                } else if (s->caret.row + 1 < s->lineCount) {
                    PushUndo(s);
                    Line* next = &s->lines[s->caret.row + 1];
                    for (size_t i = 0; i < next->length; i++) Line_InsertChar(cur, cur->length, next->text[i]);
                    Editor_DeleteLine(s, s->caret.row + 1);
                    SetModified(s, TRUE);
                }
            }
            s->preferredCol = s->caret.col;
            UpdateCodeBlocks(s);
            RecalculateMaxCols(s);
            break;
        }

        FindMatchingBrace(s);
        EnsureCaretVisible(hwnd, s);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    
    case WM_CHAR: {
        wchar_t ch = (wchar_t)wParam;
        
        if (ch < 32 && ch != L'\r' && ch != L'\t' && ch != L'\b') return 0;

        if (HasSelection(s)) {
            Editor_DeleteSelection(s);
            if (ch == L'\b') {
                s->preferredCol = s->caret.col;
                UpdateCodeBlocks(s);
                RecalculateMaxCols(s);
                FindMatchingBrace(s);
                EnsureCaretVisible(hwnd, s);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        }

        int tabWidth = (s->tabSize > 0) ? s->tabSize : 2;

        if (ch == L'\r') {
            Editor_SplitLine(s);
        } else if (ch == L'}') {
            Line* cur = &s->lines[s->caret.row];
            BOOL onlySpacesBefore = TRUE;
            for (size_t k = 0; k < (size_t)s->caret.col; k++) {
                if (cur->text[k] != L' ') { onlySpacesBefore = FALSE; break; }
            }

            if (onlySpacesBefore) {
                int depth = 1;
                int matchRow = -1;
                for (int r = s->caret.row - 1; r >= 0; r--) {
                    Line* l = &s->lines[r];
                    for (int64_t c = (int64_t)l->length - 1; c >= 0; c--) {
                        if (!IsCharInQuoteOrComment(s, r, c)) {
                            if (l->text[c] == L'}') depth++;
                            else if (l->text[c] == L'{') {
                                depth--;
                                if (depth == 0) { matchRow = r; break; }
                            }
                        }
                    }
                    if (matchRow != -1) break;
                }

                if (matchRow != -1) {
                    PushUndo(s);
                    Line* matchLine = &s->lines[matchRow];
                    size_t openIndent = 0;
                    while (openIndent < matchLine->length && matchLine->text[openIndent] == L' ') openIndent++;

                    size_t tailLen = (cur->length >= (size_t)s->caret.col) ? (cur->length - (size_t)s->caret.col) : 0;
                    wchar_t* tail = NULL;
                    if (tailLen > 0) {
                        tail = (wchar_t*)malloc(tailLen * sizeof(wchar_t));
                        if (tail) memcpy(tail, &cur->text[s->caret.col], tailLen * sizeof(wchar_t));
                    }

                    cur->length = 0;
                    cur->text[0] = L'\0';
                    for (size_t k = 0; k < openIndent; k++) Line_InsertChar(cur, k, L' ');
                    Line_InsertChar(cur, openIndent, L'}');
                    if (tail) {
                        for (size_t k = 0; k < tailLen; k++) Line_InsertChar(cur, cur->length, tail[k]);
                        free(tail);
                    }

                    s->caret.col = (int64_t)openIndent + 1;
                    s->preferredCol = s->caret.col;
                    ClearSelection(s);
                    SetModified(s, TRUE);
                    UpdateCodeBlocks(s);
                    RecalculateMaxCols(s);
                    FindMatchingBrace(s);
                    EnsureCaretVisible(hwnd, s);
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
            PushUndo(s);
            Line_InsertChar(&s->lines[s->caret.row], (size_t)s->caret.col++, ch);
            SetModified(s, TRUE);
        } else if (ch == L'\b') {
            Line* cur = &s->lines[s->caret.row];
            size_t firstNonSpace = 0;
            while (firstNonSpace < cur->length && (cur->text[firstNonSpace] == L' ' || cur->text[firstNonSpace] == L'\t')) firstNonSpace++;

            if (firstNonSpace < cur->length && (size_t)s->caret.col <= firstNonSpace) {
                if (firstNonSpace > 0) {
                    PushUndo(s);
                    size_t rem = firstNonSpace % tabWidth;
                    size_t spacesToDelete = (rem == 0) ? (size_t)tabWidth : rem;
                    if (spacesToDelete > firstNonSpace) spacesToDelete = firstNonSpace;
                    Line_DeleteChars(cur, firstNonSpace - spacesToDelete, spacesToDelete);
                    s->caret.col = (int64_t)(firstNonSpace - spacesToDelete);
                    SetModified(s, TRUE);
                } else if (s->caret.row > 0) {
                    PushUndo(s);
                    Line* prev = &s->lines[s->caret.row - 1];
                    size_t oldPrevLen = prev->length;
                    for (size_t i = 0; i < cur->length; i++) Line_InsertChar(prev, prev->length, cur->text[i]);
                    Editor_DeleteLine(s, s->caret.row);
                    s->caret.row--;
                    s->caret.col = (int64_t)oldPrevLen;
                    SetModified(s, TRUE);
                }
            } else if (s->caret.col > 0) {
                PushUndo(s);
                size_t spacesToDelete = 1;
                if (s->useSpaces && cur->text[s->caret.col - 1] == L' ') {
                    size_t rem = (size_t)s->caret.col % tabWidth;
                    size_t step = (rem == 0) ? (size_t)tabWidth : rem;
                    BOOL allSpaces = TRUE;
                    for (size_t i = 1; i <= step; i++) {
                        if (cur->text[(size_t)s->caret.col - i] != L' ') { allSpaces = FALSE; break; }
                    }
                    if (allSpaces) spacesToDelete = step;
                }

                Line_DeleteChars(cur, (size_t)s->caret.col - spacesToDelete, spacesToDelete);
                s->caret.col -= (int64_t)spacesToDelete;
                SetModified(s, TRUE);
            } else if (s->caret.row > 0) {
                PushUndo(s);
                Line* prev = &s->lines[s->caret.row - 1];
                size_t oldPrevLen = prev->length;
                for (size_t i = 0; i < cur->length; i++) Line_InsertChar(prev, prev->length, cur->text[i]);
                Editor_DeleteLine(s, s->caret.row);
                s->caret.row--;
                s->caret.col = (int64_t)oldPrevLen;
                SetModified(s, TRUE);
            }
        } else if (ch == L'\t') {
            PushUndo(s);
            Line* cur = &s->lines[s->caret.row];
            int64_t curCol = s->caret.col;
            int spacesToAdd = tabWidth - (int)(curCol % tabWidth);
            if (spacesToAdd <= 0) spacesToAdd = tabWidth;

            for (int i = 0; i < spacesToAdd; i++) Line_InsertChar(cur, (size_t)s->caret.col++, L' ');
            SetModified(s, TRUE);
        } else if (ch >= 32) {
            PushUndo(s);
            Line_InsertChar(&s->lines[s->caret.row], (size_t)s->caret.col++, ch);
            SetModified(s, TRUE);
        }

        s->preferredCol = s->caret.col;
        ClearSelection(s);
        UpdateCodeBlocks(s);
        RecalculateMaxCols(s);
        FindMatchingBrace(s);
        EnsureCaretVisible(hwnd, s);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
            
            s->isCtrlSolo = FALSE; // User is zooming, do not jump on release
            
            if (HasSelection(s)) return 0;

            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0) {
                if (s->pointSize + s->zoomLevel < MAX_ZOOM_POINT_SIZE) {
                    s->zoomLevel++;
                    RecreateEditorFont(hwnd, s);
                    EnsureCaretVisible(hwnd, s);
                }
            } else if (delta < 0) {
                if (s->pointSize + s->zoomLevel > MIN_ZOOM_POINT_SIZE) {
                    s->zoomLevel--;
                    RecreateEditorFont(hwnd, s);
                    EnsureCaretVisible(hwnd, s);
                }
            }
            return 0;
        }

        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        s->scrollY -= (delta / WHEEL_DELTA) * (s->charHeight * 3);
        int maxScroll = (s->lineCount - (s->visibleLines > 0 ? s->visibleLines : 1)) * s->charHeight;
        if (maxScroll < 0) maxScroll = 0;
        if (s->scrollY < 0) s->scrollY = 0;
        if (s->scrollY > maxScroll) s->scrollY = maxScroll;

        UpdateCaret(hwnd, s);
        UpdateScrollbars(hwnd, s);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_HSCROLL: {
        SCROLLINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL | SIF_TRACKPOS;
        GetScrollInfo(hwnd, SB_HORZ, &si);

        int charW = (s->charWidth > 0) ? s->charWidth : 8;
        int64_t totalCols = s->maxCols + 5;
        int64_t curColPos = s->scrollX;

        RECT rc;
        GetClientRect(hwnd, &rc);
        int clientW = rc.right - (s->gutterWidth + TEXT_MARGIN_X);
        int64_t visCols = (clientW > 0 && charW > 0) ? (clientW / charW) : 1;
        if (visCols <= 0) visCols = 1;

        int64_t maxColPos = totalCols - visCols;
        if (maxColPos < 0) maxColPos = 0;

        switch (LOWORD(wParam)) {
        case SB_LINELEFT:
            curColPos -= 8;
            break;
        case SB_LINERIGHT:
            curColPos += 8;
            break;
        case SB_PAGELEFT:
            curColPos -= (totalCols > MAX_SCROLL_UNITS) ? (totalCols / 50) : visCols;
            break;
        case SB_PAGERIGHT:
            curColPos += (totalCols > MAX_SCROLL_UNITS) ? (totalCols / 50) : visCols;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            if (totalCols <= MAX_SCROLL_UNITS) {
                curColPos = si.nTrackPos;
            } else {
                double trackRange = (double)(MAX_SCROLL_UNITS - (int)si.nPage + 1);
                if (trackRange <= 0.0) trackRange = 1.0;
                curColPos = (int64_t)((double)si.nTrackPos * (double)maxColPos / trackRange);
            }
            break;
        }

        if (curColPos < 0) curColPos = 0;
        if (curColPos > maxColPos) curColPos = maxColPos;

        s->scrollX = curColPos;

        UpdateCaret(hwnd, s);
        UpdateScrollbars(hwnd, s);
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        return 0;
    }

    case WM_VSCROLL: {
        SCROLLINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL | SIF_TRACKPOS;
        GetScrollInfo(hwnd, SB_VERT, &si);

        int charH = (s->charHeight > 0) ? s->charHeight : 18;
        int totalLines = s->lineCount;
        int curLinePos = (charH > 0) ? (s->scrollY / charH) : 0;
        int visLines = (s->visibleLines > 0) ? s->visibleLines : 1;

        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            curLinePos--;
            break;
        case SB_LINEDOWN:
            curLinePos++;
            break;
        case SB_PAGEUP:
            curLinePos -= visLines;
            break;
        case SB_PAGEDOWN:
            curLinePos += visLines;
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            if (totalLines <= MAX_SCROLL_UNITS) {
                curLinePos = si.nTrackPos;
            } else {
                curLinePos = (int)((double)si.nTrackPos * totalLines / MAX_SCROLL_UNITS);
            }
            break;
        }

        int maxLinePos = totalLines - visLines;
        if (maxLinePos < 0) maxLinePos = 0;
        if (curLinePos < 0) curLinePos = 0;
        if (curLinePos > maxLinePos) curLinePos = maxLinePos;

        s->scrollY = curLinePos * charH;

        UpdateCaret(hwnd, s);
        UpdateScrollbars(hwnd, s);
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        return 0;
    }

    case WM_SIZE:
        UpdateScrollbars(hwnd, s);
        UpdateCaret(hwnd, s);
        return 0;

    case WM_NCDESTROY:
        UndoStack_Clear(&s->undoStack);
        UndoStack_Clear(&s->redoStack);
        if (s->blocks) {
            free(s->blocks);
            s->blocks = NULL;
        }
        if (s->lines) {
            for (int i = 0; i < s->lineCount; i++) {
                Line_FreeText(&s->lines[i]);
            }
            free(s->lines);
            s->lines = NULL;
        }
        if (s->hFont) DeleteObject(s->hFont);
        if (g_hDragCursor) {
            DestroyCursor(g_hDragCursor);
            g_hDragCursor = NULL;
        }
        free(s);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Register Control
static void RegisterCustomEditControl(HINSTANCE hInstance) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CustomEditProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CUSTOM_EDIT_CLASS;
    RegisterClassEx(&wc);

    WNDCLASSEX wco;
    ZeroMemory(&wco, sizeof(wco));
    wco.cbSize = sizeof(WNDCLASSEX);
    wco.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wco.lpfnWndProc = OutputPaneProc;
    wco.hInstance = hInstance;
    wco.hCursor = LoadCursor(NULL, IDC_ARROW);
    wco.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wco.lpszClassName = OUTPUT_PANE_CLASS;
    RegisterClassEx(&wco);

    WNDCLASSEX wcs;
    ZeroMemory(&wcs, sizeof(wcs));
    wcs.cbSize = sizeof(WNDCLASSEX);
    wcs.style = CS_HREDRAW | CS_VREDRAW;
    wcs.lpfnWndProc = SplitterProc;
    wcs.hInstance = hInstance;
    wcs.hCursor = LoadCursor(NULL, IDC_SIZENS);
    wcs.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcs.lpszClassName = SPLITTER_CLASS;
    RegisterClassEx(&wcs);
}

// -------------------------------------------------------------
// Host Window Application & Menus
// -------------------------------------------------------------
static HMENU g_hSavedMenu = NULL;

static void ToggleFullscreen(HWND hwnd) {
    EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;

    if (!g_isFullscreen) {
        g_prevStyle = (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE);
        g_prevPlacement.length = sizeof(WINDOWPLACEMENT);
        if (GetWindowPlacement(hwnd, &g_prevPlacement)) {
            MONITORINFO mi = { sizeof(MONITORINFO) };
            HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

            if (GetMonitorInfo(hMon, &mi)) {
                g_isFullscreen = TRUE;

                if (s && s->hwndStatus) ShowWindow(s->hwndStatus, SW_HIDE);

                g_hSavedMenu = GetMenu(hwnd);
                if (g_hSavedMenu) SetMenu(hwnd, NULL);

                SetWindowLongPtr(hwnd, GWL_STYLE, 
                    (g_prevStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_MAXIMIZE)) | WS_POPUP);

                int monW = mi.rcMonitor.right - mi.rcMonitor.left;
                int monH = mi.rcMonitor.bottom - mi.rcMonitor.top;

                SetWindowPos(hwnd, HWND_TOPMOST,
                             mi.rcMonitor.left,
                             mi.rcMonitor.top,
                             monW,
                             monH,
                             SWP_FRAMECHANGED);

                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rcClient.right, rcClient.bottom));

                SetForegroundWindow(hwnd);
                if (g_hwndEditor) SetFocus(g_hwndEditor);
            }
        }
    } else {
        g_isFullscreen = FALSE;
      
        KillTimer(hwnd, IDT_FULLSCREEN);

        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        SetWindowLongPtr(hwnd, GWL_STYLE, g_prevStyle);

        if (g_hSavedMenu) {
            SetMenu(hwnd, g_hSavedMenu);
            g_hSavedMenu = NULL;
        }

        SetWindowPlacement(hwnd, &g_prevPlacement);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        if (s && s->hwndStatus) {
            ShowWindow(s->hwndStatus, s->showStatusBar ? SW_SHOW : SW_HIDE);
        }

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rcClient.right, rcClient.bottom));
        if (s && s->showStatusBar) UpdateStatusBar(s);

        DrawMenuBar(hwnd);
        SetForegroundWindow(hwnd);
        if (g_hwndEditor) SetFocus(g_hwndEditor);
    }
}

static HMENU CreateAppMenu(EditorState* s) {
    HMENU hMenuBar = CreateMenu();

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_NEW, L"&New\tCtrl+N");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN, L"&Open...\tCtrl+O");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVE, L"&Save\tCtrl+S");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVEAS, L"Save &As...\tCtrl+Shift+S");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"E&xit");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");

    HMENU hEditMenu = CreatePopupMenu();
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_UNDO, L"&Undo\tCtrl+Z");
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_REDO, L"&Redo\tCtrl+Y");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_CUT, L"Cu&t\tCtrl+X");
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_COPY, L"&Copy\tCtrl+C");
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_PASTE, L"&Paste\tCtrl+V");
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_DELETE, L"&Delete\tDel");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_SELECTALL, L"Select All\tCtrl+A");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_UPPERCASE, L"Make Selection &Uppercase\tCtrl+Shift+U");
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_LOWERCASE, L"Make Selection &Lowercase\tCtrl+U");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hEditMenu, L"&Edit");

    HMENU hSearchMenu = CreatePopupMenu();
    AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FIND, L"&Find...\tCtrl+F");
    AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FINDNEXT, L"Find &Next\tF3");
    AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FINDPREV, L"Find &Previous\tShift+F3");
    AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_FINDINFILES, L"Find in Files...\tCtrl+Shift+F");
    AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_REPLACE, L"&Replace...\tCtrl+H");
    AppendMenuW(hSearchMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSearchMenu, MF_STRING, IDM_SEARCH_GOTO, L"&Goto Line...\tCtrl+G");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hSearchMenu, L"&Search");

    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_FULLSCREEN, L"&Fullscreen\tF11");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, MF_STRING | (s && s->isDark ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_DARK, L"&Dark Theme");
    AppendMenuW(hViewMenu, MF_STRING | (s && !s->isDark ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_LIGHT, L"&Light Theme");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_FONT, L"Choose &Font...");
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_THEME_DLG, L"&Theme && Syntax Settings...");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);

    HMENU hEncMenu = CreatePopupMenu();
    for (int i = 0; i < ENCODING_COUNT; i++) AppendMenuW(hEncMenu, MF_STRING, IDM_ENC_BASE + i, g_encodings[i].name);
    AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)hEncMenu, L"&Encoding");

    HMENU hIndentMenu = CreatePopupMenu();
    AppendMenuW(hIndentMenu, MF_STRING | (s && s->tabSize == 2 ? MF_CHECKED : MF_UNCHECKED), IDM_INDENT_SIZE_2, L"Tab Size: 2");
    AppendMenuW(hIndentMenu, MF_STRING | (s && s->tabSize == 4 ? MF_CHECKED : MF_UNCHECKED), IDM_INDENT_SIZE_4, L"Tab Size: 4");
    AppendMenuW(hIndentMenu, MF_STRING | (s && s->tabSize == 8 ? MF_CHECKED : MF_UNCHECKED), IDM_INDENT_SIZE_8, L"Tab Size: 8");
    AppendMenuW(hIndentMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hIndentMenu, MF_STRING | (s && s->useSpaces ? MF_CHECKED : MF_UNCHECKED), IDM_INDENT_SPACES, L"Insert Spaces (Soft Tabs)");
    AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)hIndentMenu, L"&Indentation");

    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, MF_STRING | (s && s->showIndentGuides ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_GUIDES, L"Show &Indentation Lines");
    
    UINT highlightFlags = MF_STRING;
    if (s && !s->showIndentGuides) highlightFlags |= MF_GRAYED;
    if (s && s->highlightIndentGuides) highlightFlags |= MF_CHECKED;
    else highlightFlags |= MF_UNCHECKED;
    AppendMenuW(hViewMenu, highlightFlags, IDM_VIEW_HIGHLIGHT_GUIDES, L"&Highlight Indentation Lines");

    AppendMenuW(hViewMenu, MF_STRING | (s && s->showBraceMatch ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_BRACES, L"&Highlight Matching Braces");
    AppendMenuW(hViewMenu, MF_STRING | (s && s->showLineNumbers ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_LINENUMBERS, L"&Line Numbers");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, MF_STRING | (s && s->showStatusBar ? MF_CHECKED : MF_UNCHECKED), IDM_VIEW_STATUSBAR, L"&Status Bar");

    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");

    HMENU hToolsMenu = CreatePopupMenu();
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_ENCRYPT, L"&Encrypt\tCtrl+1");
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_DECRYPT, L"&Decrypt\tCtrl+2");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hToolsMenu, L"&Tools");

    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"&About...");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");

    return hMenuBar;
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditorState* s = g_hwndEditor ? (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA) : NULL;

    switch (msg) {
    case WM_CREATE: {
        g_hwndEditor = CreateWindowEx(
            0, CUSTOM_EDIT_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
            0, 0, 0, 0, hwnd, (HMENU)100, GetModuleHandle(NULL), NULL
        );
        s = (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA);
        s->hwndMain = hwnd;

        g_hwndSplitter = CreateWindowExW(
            0, SPLITTER_CLASS, NULL,
            WS_CHILD | (g_showOutputPane ? WS_VISIBLE : 0),
            0, 0, 0, 0, hwnd, NULL, GetModuleHandle(NULL), NULL
        );

        g_hwndOutputPane = CreateWindowExW(
            0, OUTPUT_PANE_CLASS, NULL,
            WS_CHILD | (g_showOutputPane ? WS_VISIBLE : 0) | WS_CLIPCHILDREN,
            0, 0, 0, 0, hwnd, NULL, GetModuleHandle(NULL), NULL
        );

        s->hwndStatus = CreateWindowExW(
            0, STATUSCLASSNAME, NULL,
            WS_CHILD | SBARS_SIZEGRIP | (s->showStatusBar ? WS_VISIBLE : 0),
            0, 0, 0, 0, hwnd, (HMENU)IDC_STATUSBAR, GetModuleHandle(NULL), NULL
        );

        HMENU hMenu = CreateAppMenu(s);
        SetMenu(hwnd, hMenu);
        int activeTabCmd = (s->tabSize == 4) ? IDM_INDENT_SIZE_4 : ((s->tabSize == 8) ? IDM_INDENT_SIZE_8 : IDM_INDENT_SIZE_2);
        CheckMenuRadioItem(hMenu, IDM_INDENT_SIZE_2, IDM_INDENT_SIZE_8, activeTabCmd, MF_BYCOMMAND);

        DrawMenuBar(hwnd);
        UpdateEncodingMenuCheck(hwnd, s->encodingIdx);
        UpdateTitle(s);
        UpdateStatusBar(s);
        SetFocus(g_hwndEditor);
        return 0;
    }

    case WM_COMMAND: {
        if (s) s->isCtrlSolo = FALSE; // <-- Cancel solo Ctrl on any command
        int cmd = LOWORD(wParam);

        switch (cmd) {
        case IDM_FILE_NEW: {
            if (PromptSaveIfModified(hwnd, s)) {
                Editor_ClearAll(s);
                UndoStack_Clear(&s->undoStack);
                UndoStack_Clear(&s->redoStack);
                s->filePath[0] = L'\0';
                s->fileName[0] = L'\0';
                s->encodingIdx = 0;
                SetModified(s, FALSE);
                UpdateCodeBlocks(s);
                UpdateScrollbars(g_hwndEditor, s);
                UpdateCaret(g_hwndEditor, s);
                UpdateEncodingMenuCheck(hwnd, s->encodingIdx);
                UpdateTitle(s);
                UpdateStatusBar(s);
                InvalidateRect(g_hwndEditor, NULL, FALSE);
            }
            break;
        }
        case IDM_FILE_OPEN: {
            if (PromptSaveIfModified(hwnd, s)) {
                wchar_t filename[MAX_PATH] = L"";
                OPENFILENAMEW ofn;
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"All Files (*.*)\0*.*\0C/C++ Files (*.c;*.cpp;*.cxx;*.h;*.hpp)\0*.c;*.cpp;*.cxx;*.h;*.hpp\0";
                ofn.nFilterIndex = 1;
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) {
                    LoadFileContent(g_hwndEditor, s, filename);
                }
            }
            break;
        }
        case IDM_FILE_SAVE: {
            if (s->filePath[0] != L'\0') {
                SaveFileContent(g_hwndEditor, s, s->filePath);
                break;
            }
        }
        case IDM_FILE_SAVEAS:
            DoFileSaveAs(hwnd, s);
            break;
        case IDM_VIEW_FULLSCREEN:
            ToggleFullscreen(hwnd);
            break;
        case IDM_VIEW_DARK:
            s->isDark = TRUE;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_DARK, MF_CHECKED);
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_LIGHT, MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            UpdateCodeBlocks(s);
            FindMatchingBrace(s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            if (g_hwndOutputPane) InvalidateRect(g_hwndOutputPane, NULL, TRUE);
            if (g_hwndOutputList) InvalidateRect(g_hwndOutputList, NULL, TRUE);
            break;
        case IDM_VIEW_LIGHT:
            s->isDark = FALSE;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_LIGHT, MF_CHECKED);
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_DARK, MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            UpdateCodeBlocks(s);
            FindMatchingBrace(s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            if (g_hwndOutputPane) InvalidateRect(g_hwndOutputPane, NULL, TRUE);
            if (g_hwndOutputList) InvalidateRect(g_hwndOutputList, NULL, TRUE);
            break;
        case IDM_VIEW_GUIDES:
            s->showIndentGuides = !s->showIndentGuides;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_GUIDES, s->showIndentGuides ? MF_CHECKED : MF_UNCHECKED);
            EnableMenuItem(GetMenu(s->hwndMain), IDM_VIEW_HIGHLIGHT_GUIDES, s->showIndentGuides ? MF_ENABLED : MF_GRAYED);
            SaveSettingsToIni(s->hwndMain, s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            break;
        case IDM_VIEW_HIGHLIGHT_GUIDES:
            s->highlightIndentGuides = !s->highlightIndentGuides;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_HIGHLIGHT_GUIDES, s->highlightIndentGuides ? MF_CHECKED : MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            break;
        case IDM_VIEW_BRACES:
            s->showBraceMatch = !s->showBraceMatch;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_BRACES, s->showBraceMatch ? MF_CHECKED : MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            FindMatchingBrace(s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            break;
        case IDM_VIEW_LINENUMBERS:
            s->showLineNumbers = !s->showLineNumbers;
            CheckMenuItem(GetMenu(s->hwndMain), IDM_VIEW_LINENUMBERS, s->showLineNumbers ? MF_CHECKED : MF_UNCHECKED);
            SaveSettingsToIni(s->hwndMain, s);
            RecalculateGutter(s);
            UpdateScrollbars(g_hwndEditor, s);
            UpdateCaret(g_hwndEditor, s);
            InvalidateRect(g_hwndEditor, NULL, TRUE);
            break;
        case IDM_VIEW_STATUSBAR: {
            s->showStatusBar = !s->showStatusBar;
            CheckMenuItem(GetMenu(hwnd), IDM_VIEW_STATUSBAR, s->showStatusBar ? MF_CHECKED : MF_UNCHECKED);
            if (s->hwndStatus) {
                ShowWindow(s->hwndStatus, s->showStatusBar ? SW_SHOW : SW_HIDE);
            }
            SaveSettingsToIni(hwnd, s);
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rcClient.right, rcClient.bottom));
            UpdateStatusBar(s);
            break;
        }
        case IDM_INDENT_SIZE_2:
        case IDM_INDENT_SIZE_4:
        case IDM_INDENT_SIZE_8:
        case IDM_INDENT_SPACES:
            SendMessage(g_hwndEditor, WM_COMMAND, wParam, lParam);
            break;
        case IDM_VIEW_FONT:
            SendMessage(g_hwndEditor, WM_COMMAND, wParam, lParam);
            break;
        case IDM_VIEW_THEME_DLG:
            ShowThemeSettingsDialog(hwnd, s);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_TOOLS_ENCRYPT: Tool_Encrypt(g_hwndEditor, s); break;
        case IDM_TOOLS_DECRYPT: Tool_Decrypt(g_hwndEditor, s); break;
        case IDM_HELP_ABOUT:    ShowAboutDialog(hwnd); break;
        case IDM_FILE_EXIT:     SendMessage(hwnd, WM_CLOSE, 0, 0); break;
        default:
            SendMessage(g_hwndEditor, WM_COMMAND, wParam, lParam);
            break;
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;
    
    case WM_SIZE: {
        int cx = LOWORD(lParam);
        int cy = HIWORD(lParam);
        int statusHeight = 0;

        if (!g_isFullscreen && s && s->showStatusBar && s->hwndStatus && IsWindow(s->hwndStatus)) {
            SendMessage(s->hwndStatus, WM_SIZE, 0, 0);
            RECT rcStatus;
            GetWindowRect(s->hwndStatus, &rcStatus);
            statusHeight = rcStatus.bottom - rcStatus.top;

            int parts[3];
            parts[0] = 220;
            parts[1] = 520;
            parts[2] = -1;
            SendMessage(s->hwndStatus, SB_SETPARTS, 3, (LPARAM)parts);
        }

        int availH = cy - statusHeight;
        if (availH < 0) availH = 0;

        if (g_showOutputPane && g_hwndOutputPane && g_hwndSplitter) {
            int splitBarH = 4;
            int outH = g_splitterHeight;
            if (outH <= 0) outH = availH / 4;
            if (outH < 40) outH = 40;
            if (outH > availH - 50) outH = availH - 50;

            int editH = availH - outH - splitBarH;
            if (editH < 0) editH = 0;

            if (g_hwndEditor) MoveWindow(g_hwndEditor, 0, 0, cx, editH, TRUE);
            MoveWindow(g_hwndSplitter, 0, editH, cx, splitBarH, TRUE);
            MoveWindow(g_hwndOutputPane, 0, editH + splitBarH, cx, outH, TRUE);
        } else {
            if (g_hwndEditor) MoveWindow(g_hwndEditor, 0, 0, cx, availH, TRUE);
        }
        return 0;
    }
        
    case WM_SETFOCUS:
        if (g_hwndEditor && IsWindow(g_hwndEditor)) SetFocus(g_hwndEditor);
        return 0;

    case WM_CLOSE:
        if (PromptSaveIfModified(hwnd, s)) {
            SaveSettingsToIni(hwnd, s);
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_PARENTNOTIFY:
        if (g_isFullscreen && LOWORD(wParam) == WM_LBUTTONDOWN) {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetTimer(hwnd, IDT_FULLSCREEN, 50, NULL);
        }
        break;

    case WM_ACTIVATE:
        if (g_isFullscreen) {
            BOOL bActivating = (LOWORD(wParam) != WA_INACTIVE);
            if (bActivating) {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetTimer(hwnd, IDT_FULLSCREEN, 50, NULL);
            } else {
                KillTimer(hwnd, IDT_FULLSCREEN);
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
        break;

    case WM_TIMER:
        if (wParam == IDT_FULLSCREEN) {
            KillTimer(hwnd, IDT_FULLSCREEN);
            if (g_isFullscreen) {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            return 0;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_FULLSCREEN);
        if (g_hSavedMenu) {
            DestroyMenu(g_hSavedMenu);
            g_hSavedMenu = NULL;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void InitDpiAwareness(void) {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *SetProcessDPIAwareFunc)(void);
        SetProcessDPIAwareFunc setAware = (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
        if (setAware) setAware();
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrev;
    (void)lpCmdLine;
    InitDpiAwareness();

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TAB_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    RegisterCustomEditControl(hInstance);

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MainWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100));
    RegisterClassEx(&wc);

    wchar_t iniPath[MAX_PATH];
    GetIniPath(iniPath, MAX_PATH);
    int x = GetPrivateProfileIntW(L"Window", L"Left", CW_USEDEFAULT, iniPath);
    int y = GetPrivateProfileIntW(L"Window", L"Top", CW_USEDEFAULT, iniPath);
    int w = GetPrivateProfileIntW(L"Window", L"Width", 950, iniPath);
    int h = GetPrivateProfileIntW(L"Window", L"Height", 650, iniPath);
    int max = GetPrivateProfileIntW(L"Window", L"Maximized", 0, iniPath);

    HWND hwnd = CreateWindowEx(
        0, wc.lpszClassName, L"Untitled",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, x, y, w, h, NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, max ? SW_SHOWMAXIMIZED : nCmdShow);
    UpdateWindow(hwnd);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        if (argc >= 2 && g_hwndEditor) {
            EditorState* s = (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA);
            if (s) LoadFileContent(g_hwndEditor, s, argv[1]);
        }
        LocalFree(argv);
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000)) 
        {
            // Any Ctrl shortcut pressed cancels solo Ctrl
            if (msg.wParam != VK_CONTROL && g_hwndEditor) {
                EditorState* s = (EditorState*)GetWindowLongPtr(g_hwndEditor, GWLP_USERDATA);
                if (s) s->isCtrlSolo = FALSE;
            }
            if (msg.wParam == 'F' || msg.wParam == 'f') {
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_FINDINFILES, 0);
                } else {
                    SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_FIND, 0);
                }
                continue;
            }
            if (msg.wParam == 'H' || msg.wParam == 'h') {
                SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_REPLACE, 0);
                continue;
            }
            if (msg.wParam == 'G' || msg.wParam == 'g') {
                SendMessage(hwnd, WM_COMMAND, IDM_SEARCH_GOTO, 0);
                continue;
            }
        }

        if (g_hFindDlg && IsDialogMessageW(g_hFindDlg, &msg)) continue;
        if (g_hReplaceDlg && IsDialogMessageW(g_hReplaceDlg, &msg)) continue;
        if (g_hGotoDlg && IsDialogMessageW(g_hGotoDlg, &msg)) continue;
        if (g_hGrepDlg && IsDialogMessageW(g_hGrepDlg, &msg)) continue;

        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_F11) {
                ToggleFullscreen(hwnd);
                continue;
            }
            if (msg.wParam == VK_F3) {
                BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (shift) SendMessage(g_hwndEditor, WM_COMMAND, IDM_SEARCH_FINDPREV, 0);
                else SendMessage(g_hwndEditor, WM_COMMAND, IDM_SEARCH_FINDNEXT, 0);
                continue;
            }
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (msg.wParam == 'Z' || msg.wParam == 'z') {
                    if (shift) SendMessage(g_hwndEditor, WM_COMMAND, IDM_EDIT_REDO, 0);
                    else SendMessage(g_hwndEditor, WM_COMMAND, IDM_EDIT_UNDO, 0);
                    continue;
                } else if (msg.wParam == 'Y' || msg.wParam == 'y') {
                    SendMessage(g_hwndEditor, WM_COMMAND, IDM_EDIT_REDO, 0);
                    continue;
                } else if (msg.wParam == 'U' || msg.wParam == 'u') {
                    if (shift) SendMessage(g_hwndEditor, WM_COMMAND, IDM_EDIT_UPPERCASE, 0);
                    else SendMessage(g_hwndEditor, WM_COMMAND, IDM_EDIT_LOWERCASE, 0);
                    continue;
                } else if (msg.wParam == '1') {
                    SendMessage(hwnd, WM_COMMAND, IDM_TOOLS_ENCRYPT, 0);
                    continue;
                } else if (msg.wParam == '2') {
                    SendMessage(hwnd, WM_COMMAND, IDM_TOOLS_DECRYPT, 0);
                    continue;
                } else if ((msg.wParam == 'S' || msg.wParam == 's') && shift) {
                    SendMessage(hwnd, WM_COMMAND, IDM_FILE_SAVEAS, 0);
                    continue;
                } else if (msg.wParam == 'S' || msg.wParam == 's') {
                    SendMessage(hwnd, WM_COMMAND, IDM_FILE_SAVE, 0);
                    continue;
                } else if (msg.wParam == 'O' || msg.wParam == 'o') {
                    SendMessage(hwnd, WM_COMMAND, IDM_FILE_OPEN, 0);
                    continue;
                } else if (msg.wParam == 'N' || msg.wParam == 'n') {
                    SendMessage(hwnd, WM_COMMAND, IDM_FILE_NEW, 0);
                    continue;
                }
            }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

