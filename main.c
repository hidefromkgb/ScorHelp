#define _WIN32_IE 0x501
#define _WIN32_WINNT 0x501

#define BIF_NONEWFOLDERBUTTON 0x200
#define BIF_FLGS BIF_BROWSEINCLUDEFILES \
               | BIF_NONEWFOLDERBUTTON  \
               | BIF_RETURNFSANCESTORS  \
               | BIF_DONTGOBELOWDOMAIN  \
               | BIF_RETURNONLYFSDIRS   \
               | BIF_NEWDIALOGSTYLE     \
               | BIF_STATUSTEXT

#include <stdio.h>
#include <shlobj.h>
#include <windows.h>
#include <commctrl.h>

#include "res\resource.h"



#define swap32(i) (((BYTE*)&(i))[0] << 24 | ((BYTE*)&(i))[1] << 16 | ((BYTE*)&(i))[2] << 8 | ((BYTE*)&(i))[3])
#define fourcc(a, b, c, d) ((a) + 0x100 * (b) + 0x10000 * (c) + 0x1000000 * (d))
#define arrsize(a) (sizeof(a)/sizeof(*(a)))

#define IMG_FLDC 0
#define IMG_FLDO 1
#define IMG_FILE 2
#define IMG_TEXT 3
#define IMG_SCOR 4
#define IMG_PICT 5
#define IMG_BEEP 6
#define IMG_MESH 7

#define RCM_SAVE 101
#define RCM_LOAD 102

#define DEF_EXES 776379
#define DEF_CRCO 0xC495C3BA
#define DEF_CRCP 0xCF361257
#define DEF_CRCL 0xEDB88320

#define DEF_EXEP "scorwin.exe"
#define DEF_RESP "tagden.bin"
#define DEF_CFGP "scor95.bin"
#define DEF_SCRP "hiscores.bin"
#define DEF_REGP "Software\\Scorcher Tools"
#define DEF_INSP "Install Path"

#define PTR_WVDM 32*4
#define PTR_FVDM 16*4
#define PTR_SCRN 14*4
#define PTR_POLY 28*4

#define EXE_UNKN (DWORD)~0
#define EXE_ORIG 1
#define EXE_PTCH 0



BOOL CALLBACK GraphicsProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK HiscoresProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK ResourceProc(HWND, UINT, WPARAM, LPARAM);



/** Pseudo-filesystem node **/
typedef struct _PFSN {
    struct _PFSN *next;
    struct {
        DWORD fhsz, fpos, ffsz, fnsz;
    } head;
    LPSTR name, file;
} PFSN;



/** Video mode data **/
typedef struct _TDVM {
    DWORD dwid, dhei, dbit;
} TDVM;



const TDVM lres[] = {
    {320, 200,~ 8}, {320, 200, 15}, {320, 200,~16}, {320, 200, 24},
    {320, 200, 32}, {320, 240,~ 8}, {320, 240, 15}, {320, 240,~16},
    {320, 240, 24}, {320, 240, 32}, {320, 350,  8}, {320, 350, 15},
    {320, 350, 16}, {320, 350, 24}, {320, 350, 32}, {320, 400,  8},
    {320, 400, 15}, {320, 400, 16}, {320, 400, 24}, {320, 400, 32},
    {320, 480,  8}, {320, 480, 15}, {320, 480, 16}, {320, 480, 24},
    {320, 480, 32}, {360, 200,  8}, {360, 200, 15}, {360, 200, 16},
    {360, 200, 24}, {360, 200, 32}, {360, 240,  8}, {360, 240, 15},
    {360, 240, 16}, {360, 240, 24}, {360, 240, 32}, {360, 350,  8},
    {360, 350, 15}, {360, 350, 16}, {360, 350, 24}, {360, 350, 32},
    {360, 400,  8}, {360, 400, 15}, {360, 400, 16}, {360, 400, 24},
    {360, 400, 32}, {360, 480,  8}, {360, 480, 15}, {360, 480, 16},
    {360, 480, 24}, {360, 480, 32}, {400, 300,~ 8}, {400, 300, 15},
    {400, 300,~16}, {400, 300, 24}, {400, 300, 32}, {512, 384,~ 8},
    {512, 384, 15}, {512, 384,~16}, {512, 384, 24}, {512, 384, 32},
    {640, 400,~ 8}, {640, 400,~16}, {640, 480,~ 8}, {640, 480,~16}
};
const DWORD NUM_LRES = arrsize(lres);



const union {
    struct {
        DWORD dpos, dlen;
        LPSTR fmod, forg;
    } pstr;
    LPSTR parr[4];
} ptch[] = {
    {{118394,   4, "\xE9\x3D\xBC\x01", "\xBE\xE8\xFB\x43"}},
    {{118673,   4, "\xE9\xED\xBA\x01", "\xB9\x98\x00\x00"}},
    {{177350,   2, "\x00\x80", "\x80\x0A"}},
    {{187234,   1, "\x00", "\x04"}},
    {{189454,   9, "\xB8\x01\x00\x00\x00\x90\x90\x90\x90", "\x6A\x20\x2E\xFF\x15\xC0\xA1\x4B\x00"}},
    {{189470,   9, "\xB8\x01\x00\x00\x00\x90\x90\x90\x90", "\x6A\x21\x2E\xFF\x15\xC0\xA1\x4B\x00"}},
    {{193411,   2, "\xA0\x8C", "\x7F\x0A"}},
    {{193483,   9, "\xB8\x01\x00\x00\x00\x90\x90\x90\x90", "\x6A\x20\x2E\xFF\x15\xC0\xA1\x4B\x00"}},
    {{193500,   9, "\xB8\x01\x00\x00\x00\x90\x90\x90\x90", "\x6A\x21\x2E\xFF\x15\xC0\xA1\x4B\x00"}},
    {{203460,   2, "\xEB\x57", "\x75\x49"}},
    {{232067, 114, "\x60\x6A\x20\x68\x52\xC6\x43\x00\x2E\xFF\x15\x94\xA2\x4B\x00\x83\xF8\xFF\x74\x1A\x50\x68\x98\x00\x00\x00\x68\xDC\xEA\x43\x00\x50\x2E\xFF\x15\x98\xA2\x4B\x00\x2E\xFF\x15\x8C\xA2\x4B\x00\x61\xB9\x98\x00\x00\x00\xE9\xDA\x44\xFE\xFF\x60\x6A\x20\x68\xAE\xB4\x43\x00\x2E\xFF\x15\x94\xA2\x4B\x00\x83\xF8\xFF\x74\x1A\x50\x68\x6C\x03\x00\x00\x68\xE8\xFB\x43\x00\x50\x2E\xFF\x15\x98\xA2\x4B\x00\x2E\xFF\x15\x8C\xA2\x4B\x00\x61\xBE\xE8\xFB\x43\x00\xE9\x8A\x43\xFE\xFF", "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"}},
    {{251676,  12, "\x80\x02\x00\x00\xE0\x01\x00\x00\x10\x00\x00\x00", "\x40\x01\x00\x00\xC8\x00\x00\x00\x08\x00\x00\x00"}}
};
const DWORD NUM_TFPD = arrsize(ptch);



struct {
    LPSTR capt;
    DWORD indx;
    DLGPROC proc;
    HWND htab;
} tabs[] = {
    {"  Graphics  ",    TAB_GRPH, GraphicsProc},
    {"  High scores  ", TAB_SCOR, HiscoresProc},
    {"  Resources  ",   TAB_RSRC, ResourceProc}
};
const DWORD NUM_TABS = arrsize(tabs);



CHAR path[MAX_PATH + 1];
DWORD tcrc[256];
WNDPROC eprc, lprc;
PFSN *root, *iter;
HINSTANCE base;
HTREEITEM rtci;
HMENU rtcm;
TDVM *allm;



void Terminate(HWND hDlg, BOOL rdel, BOOL exec) {
    CHAR path[MAX_PATH + 3], temp[MAX_PATH + 3];

    if (rdel) RegDeleteKey(HKEY_CURRENT_USER, DEF_REGP);
    if (exec) {
        GetModuleFileName(NULL, temp, MAX_PATH);
        if (temp[0] != '"') {
            sprintf(path, "\"%s\"", temp);
            ShellExecute(hDlg, NULL, path, NULL, NULL, SW_SHOW);
        }
        else
            ShellExecute(hDlg, NULL, temp, NULL, NULL, SW_SHOW);
    }
    EndDialog(hDlg, 0);
}



DWORD CRC32(DWORD tcrc[256], LPSTR pstr, DWORD size) {
    DWORD gcrc, indx;

    gcrc = 0xFFFFFFFF;
    for (indx = 0; indx < size; indx++)
        gcrc = ((gcrc >> 8) & 0x00FFFFFF) ^ tcrc[(gcrc ^ *pstr++) & 0xFF];
    return gcrc ^ 0xFFFFFFFF;
}



void GenCRC32(DWORD tcrc[256], DWORD poly) {
    DWORD indx, byte, gcrc;

    for (indx = 0; indx < 256; indx++) {
        gcrc = indx;
        for (byte = 0; byte < 8; byte++) {
            if (gcrc & 1)
                gcrc = (gcrc >> 1) ^ poly;
            else
                gcrc = (gcrc >> 1);
        }
        tcrc[indx] = gcrc;
    }
}



DWORD ConvPathToDir(LPSTR path) {
    LPSTR dlmt;
    DWORD retn;

    while (!(GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY))
        if ((dlmt = strrchr(path, '\\')))
            *dlmt = 0;

    retn = strlen(path);
    if (path[retn - 1] != '\\') {
        path[retn++] = '\\';
        path[retn] = 0;
    }
    return retn;
}



BOOL CheckPathValidity(LPSTR path) {
    CHAR test[MAX_PATH + 1];
    HANDLE retn;

    strcpy(test, path);
    ConvPathToDir(test);
    strcat(test, DEF_EXEP);
    CloseHandle(retn = CreateFile(test, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));

    return (retn != INVALID_HANDLE_VALUE);
}



DWORD CheckEXEValidity(LPSTR path, DWORD exes, DWORD tcrc[128], DWORD crco, DWORD crcp) {
    CHAR *fptr, test[MAX_PATH + 1];
    DWORD size, read;
    HANDLE retn;

    strcpy(test, path);
    strcat(test, DEF_EXEP);
    retn = CreateFile(test, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    size = SetFilePointer(retn, 0, NULL, FILE_END);
    SetFilePointer(retn, 0, NULL, FILE_BEGIN);

    if (size != exes) {
        CloseHandle(retn);
        return EXE_UNKN;
    }
    fptr = (LPSTR)malloc(size);
    ReadFile(retn, fptr, size, &read, NULL);
    CloseHandle(retn);
    read = CRC32(tcrc, fptr, size);
    free(fptr);

    return (read - crco)? (read - crcp)? EXE_UNKN : EXE_PTCH : EXE_ORIG;
}



DWORD MessageBoxIcon(HWND hWnd, LPSTR text, LPSTR capt, UINT styl, LPSTR icon) {
    MSGBOXPARAMS tmbp;

    tmbp.cbSize = sizeof(tmbp);
    tmbp.hwndOwner = hWnd;
    tmbp.hInstance = base;
    tmbp.lpszText = text;
    tmbp.lpszCaption = capt;
    tmbp.dwStyle = styl | MB_USERICON;
    tmbp.lpszIcon = icon;
    tmbp.dwContextHelpId = 0;
    tmbp.lpfnMsgBoxCallback = NULL;
    tmbp.dwLanguageId = LANG_NEUTRAL;

    return MessageBoxIndirect(&tmbp);
}



int CALLBACK BrowseProc(HWND hWnd, UINT uMsg, LPARAM lPrm, LPARAM data) {
    CHAR path[MAX_PATH + 1];
    LPSTR oldp = (LPSTR)data;

    switch (uMsg) {
        case BFFM_INITIALIZED:
            SendMessage(hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)oldp);
            break;

        case BFFM_SELCHANGED:
            path[0] = 0;
            SHGetPathFromIDList((LPITEMIDLIST)lPrm, path);
            oldp = (LPSTR)path;
            break;
    }
    SendMessage(hWnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)oldp);
    SendMessage(hWnd, BFFM_ENABLEOK, 0, CheckPathValidity(oldp));
    return 0;
}



BOOL ShFolderOpen(HWND hDlg, LPSTR capt, LPSTR path) {
    BROWSEINFO binf = {hDlg, NULL, NULL, capt, BIF_FLGS, BrowseProc};
    CHAR oldp[MAX_PATH + 1];
    BOOL retn = FALSE;
    LPITEMIDLIST pidl;

    ExpandEnvironmentStrings(path, oldp, MAX_PATH);
    binf.lParam = (LPARAM)oldp;
    do {
        pidl = SHBrowseForFolder(&binf);

        if (pidl != NULL) {
            if (SHGetPathFromIDList(pidl, oldp)) {
                ConvPathToDir(oldp);
                lstrcpy(path, oldp);
                retn = TRUE;
            }
            CoTaskMemFree(pidl);
            pidl = NULL;

            if (!CheckPathValidity(path)) {
                if (MessageBox(hDlg, "Selected folder does not seem to contain Scorcher files.\nTry again?", "Whoops...", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                    pidl = (LPITEMIDLIST)INVALID_HANDLE_VALUE;
                else
                    retn = FALSE;
            }
        }
    } while (pidl);

    return retn;
}



HANDLE TryOpenFile(LPSTR path, LPSTR name, DWORD nres, DWORD attr, HWND hDlg) {
    CHAR stmp[MAX_PATH + 1];
    HANDLE file;
    HRSRC rsrc;
    DWORD size;

    strcpy(stmp, path);
    strcat(stmp, name);
    SetFileAttributes(stmp, FILE_ATTRIBUTE_NORMAL);
    if ((file = CreateFile(stmp, attr, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        if (nres)
            Terminate(hDlg, TRUE, TRUE);
    }
    else if (GetLastError() != ERROR_ALREADY_EXISTS) {
        if (nres) {
            rsrc = FindResource(base, MAKEINTRESOURCE(nres), RT_RCDATA);
            WriteFile(file, LockResource(LoadResource(base, rsrc)), SizeofResource(base, rsrc), &size, NULL);
            SetFilePointer(file, 0, NULL, FILE_BEGIN);
        }
        else {
            CloseHandle(file);
            file = INVALID_HANDLE_VALUE;
        }
    }
    return file;
}



void ReadChunk(LPSTR *data, HANDLE file, DWORD indx, DWORD size, DWORD offs) {
    *data = (LPSTR)malloc(size);
    SetFilePointer(file, offs + size * indx, NULL, FILE_BEGIN);
    ReadFile(file, *data, size, &indx, NULL);
}



void WriteChunk(LPSTR *data, HANDLE file, DWORD indx, DWORD size, DWORD offs) {
    SetFilePointer(file, offs + size * indx, NULL, FILE_BEGIN);
    WriteFile(file, *data, size, &indx, NULL);
}



void PatchControl(LPSTR path, DWORD need, HWND hDlg, HWND hBtn) {
    CHAR test[MAX_PATH + 1];
    DWORD indx, size;
    HANDLE file;

    switch (indx = CheckEXEValidity(path, DEF_EXES, tcrc, DEF_CRCO, DEF_CRCP)) {
        case EXE_ORIG:
        case EXE_PTCH:
            EnableWindow(hBtn, need == EXE_ORIG);
            if (indx == need) {
                indx = (indx - EXE_ORIG)? 3 : 2;
                strcpy(test, path);
                strcat(test, DEF_EXEP);
                file = CreateFile(test, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                for (need = 0; need < NUM_TFPD; need++) {
                    SetFilePointer(file, ptch[need].pstr.dpos, NULL, FILE_BEGIN);
                    WriteFile(file, ptch[need].parr[indx], ptch[need].pstr.dlen, &size, NULL);
                }
                CloseHandle(file);
            }
            break;

        default:
            EnableWindow(hBtn, FALSE);
            MessageBoxIcon(hDlg, "You seem to own a release of Scorcher that the current version of this program can not patch.\nPlease email it to hidefromkgb@gmail.com for further improvement of the algorithm.\nThank you.", "CRC32 mismatch!", MB_OK, MAKEINTRESOURCE(ICN_MAIN));
    }
}



int CompareTDVM(const void *a, const void *b) {
    if (((TDVM*)a)->dbit != ((TDVM*)b)->dbit)
        return ((TDVM*)a)->dbit - ((TDVM*)b)->dbit;
    if (((TDVM*)a)->dwid != ((TDVM*)b)->dwid)
        return ((TDVM*)a)->dwid - ((TDVM*)b)->dwid;
    if (((TDVM*)a)->dhei != ((TDVM*)b)->dhei)
        return ((TDVM*)a)->dhei - ((TDVM*)b)->dhei;
    return 0;
}



void FillHiscores(HWND tree, HANDLE file) {
    LPSTR nsec[] = {"[Lap timer data]", "Best track times", "Best lap times", NULL};
    LPSTR ntrk[] = {"The Dump", "The Suburbs", "Tunnels", "Radioact. Waste", "Downtown", "The Spiral", NULL};
    TV_INSERTSTRUCT tvis = {NULL, 0, {{TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE}}};
    UINT sect, indx;

    for (sect = 0; nsec[sect]; sect++) {
        tvis.hParent = NULL;
        tvis.item.iImage = IMG_FLDC;
        tvis.item.iSelectedImage = IMG_FLDO;
        tvis.hInsertAfter = TVI_ROOT;
        tvis.item.pszText = nsec[sect];
        tvis.item.lParam = sect;

        tvis.hParent = (HTREEITEM)SendMessage(tree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        tvis.item.iImage = tvis.item.iSelectedImage = IMG_SCOR;
        tvis.hInsertAfter = TVI_LAST;
        for (indx = 0; (tvis.item.pszText = ntrk[indx]); indx++) {
            ReadChunk((LPSTR*)&tvis.item.lParam, file, 6 * sect + indx - ((sect)? 5 : 0), (sect)? 60 : 16, (sect)? 96 : 0);
            SendMessage(tree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        }
    }
    tvis.hParent = NULL;
    tvis.hInsertAfter = TVI_ROOT;
    tvis.item.pszText = "Championship";
    ReadChunk((LPSTR*)&tvis.item.lParam, file, 0, 60, 96);
    SendMessage(tree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
}



void FreeHiscores(HWND tree, HTREEITEM item) {
    HTREEITEM temp;
    TV_ITEM tcur;

    if ((temp = item)) {
        tcur.mask = TVIF_PARAM;
        while ((tcur.hItem = temp)) {
            SendMessage(tree, TVM_GETITEM, 0, (LPARAM)&tcur);
            if (HIWORD(tcur.lParam))
                free((VOID*)tcur.lParam);
            FreeHiscores(tree, (HTREEITEM)SendMessage(tree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)tcur.hItem));
            temp = (HTREEITEM)SendMessage(tree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)temp);
            SendMessage(tree, TVM_DELETEITEM, 0, (LPARAM)tcur.hItem);
        }
    }
}



void FillResource(HWND tree, PFSN *curr) {
    TV_INSERTSTRUCT tvis = {NULL, 0, {{TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE}}};
    CHAR temp[MAX_PATH + 1];
    LPSTR tbgn, tend;
    WORD hash, tlvl;
    TV_ITEM tcur;

    tvis.hParent = NULL;
    tvis.item.iImage = IMG_FLDC;
    tvis.item.iSelectedImage = IMG_FLDO;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.pszText = temp;

    tcur.mask = TVIF_PARAM;
    tcur.hItem = NULL;

    tlvl = TVGN_ROOT;
    tbgn = curr->name + 3;
    while ((tend = strchr(tbgn, '\\'))) {
        strncpy(temp, tbgn, tend - tbgn);
        temp[tend - tbgn] = 0;

        hash = 0;
        while (tbgn < tend)
            hash = hash * 0xFBC5 + *tbgn++;

        if (tcur.hItem || (tlvl == TVGN_ROOT)) {
            do {
                SendMessage(tree, TVM_GETITEM, 0, (LPARAM)&tcur);
                if (LOWORD(tcur.lParam) == hash)
                    break;
            } while ((tcur.hItem = (HTREEITEM)SendMessage(tree, TVM_GETNEXTITEM, tlvl, (LPARAM)tcur.hItem)));
            tlvl = TVGN_NEXT;
        }
        if (tcur.hItem)
            tcur.hItem = (HTREEITEM)SendMessage(tree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)(tvis.hParent = tcur.hItem));
        else {
            tvis.item.lParam = hash;
            tvis.hParent = (HTREEITEM)SendMessage(tree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        }
        tbgn = tend + 1;
    }

    strcpy(temp, tbgn);
    *(DWORD*)(temp + strlen(temp)) = 0;
    CharUpper(temp);
    if ((tbgn = strchr(temp, '.'))) {
        switch (*(DWORD*)(tbgn + 1)) {
            case fourcc('R', '0', 'V', 0):
            case fourcc('F', 'M', 'Z', 0):
            case fourcc('F', 'Z', 'P', 0):
            case fourcc('0',  0,   0,  0):
            case fourcc('1',  0,   0,  0):
                tvis.item.iImage = tvis.item.iSelectedImage = IMG_PICT;
                break;

            case fourcc('T', 'X', 'T', 0):
                tvis.item.iImage = tvis.item.iSelectedImage = IMG_TEXT;
                break;

            case fourcc('W', 'L', '3', 0):
                tvis.item.iImage = tvis.item.iSelectedImage = IMG_MESH;
                break;

            case fourcc('B', 'I', 'N', 0):
                tvis.item.iImage = tvis.item.iSelectedImage = IMG_BEEP;
                if (!strcmp(temp, "SCOR1.BIN"))
                    break;

            default:
                tvis.item.iImage = tvis.item.iSelectedImage = IMG_FILE;
                break;
        }
        tvis.item.lParam = (LPARAM)curr;
        SendMessage(tree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
    }
}



LONG CALLBACK EditProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_KEYDOWN:
            if (wPrm == VK_ESCAPE) {
                ShowWindow(hWnd, SW_HIDE);
                return 0;
            }
            else if (wPrm == VK_RETURN)

        case WM_KILLFOCUS: {
            if (uMsg != WM_KILLFOCUS) {
                HWND lWnd = GetParent(hWnd);
                LPSTR text = NULL;
                RECT rect;

                GetWindowRect(hWnd, &rect);
                LV_HITTESTINFO iclk = {{(rect.right + rect.left) >> 1, (rect.bottom + rect.top) >> 1}};

                ScreenToClient(lWnd, &iclk.pt);
                SendMessage(lWnd, LVM_SUBITEMHITTEST, 0, (LPARAM)&iclk);
                LV_DISPINFO disp = {{hWnd, GetDlgCtrlID(hWnd), LVN_ENDLABELEDIT}, {LVIF_TEXT | LVIF_PARAM, iclk.iItem, iclk.iSubItem}};

                wPrm = 1 + SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
                text = (LPSTR)malloc(wPrm);
                SendMessage(hWnd, WM_GETTEXT, wPrm, (LPARAM)text);
                disp.item.cchTextMax = wPrm - 1;
                disp.item.pszText = text;
                SendMessage(GetParent(lWnd), WM_NOTIFY, GetDlgCtrlID(lWnd), (LPARAM)&disp);
                free(text);
            }
            ShowWindow(hWnd, SW_HIDE);
        }

        default:
            return CallWindowProc(eprc, hWnd, uMsg, wPrm, lPrm) | ((uMsg == WM_GETDLGCODE)? DLGC_WANTALLKEYS : 0);
    }
}



LONG CALLBACK ListProc(HWND hWnd, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            LV_HITTESTINFO iclk = {{LOWORD(lPrm), HIWORD(lPrm)}};
            HWND tree = GetDlgItem(GetParent(hWnd), STV_TREE);
            LV_ITEM lcur = {};
            TV_ITEM tcur = {};
            CHAR text[256];
            RECT rect;

            tcur.hItem = (HTREEITEM)SendMessage(tree, TVM_GETNEXTITEM, TVGN_PARENT, SendMessage(tree, TVM_GETNEXTITEM, TVGN_CARET, 0));
            if (!tcur.hItem)
                tcur.lParam = -1;
            else {
                tcur.mask = TVIF_PARAM;
                SendMessage(tree, TVM_GETITEM, 0, (LPARAM)&tcur);
            }

            uMsg = 1 + SendMessage(hWnd, LVM_SUBITEMHITTEST, 0, (LPARAM)&iclk);
            if (!uMsg || (!iclk.iSubItem && !tcur.lParam))
                SendDlgItemMessage(hWnd, SED_LIST, WM_KILLFOCUS, 0, 0);
            else {
                rect.left = LVIR_BOUNDS;
                rect.top = iclk.iSubItem;
                SendMessage(hWnd, LVM_GETSUBITEMRECT, (WPARAM)iclk.iItem, (LPARAM)&rect);
                if (!iclk.iSubItem)
                    rect.right = rect.left + SendMessage(hWnd, LVM_GETCOLUMNWIDTH, 0, 0);
                lcur.iItem = iclk.iItem;
                lcur.iSubItem = iclk.iSubItem;
                lcur.cchTextMax = 256;
                lcur.pszText = text;
                SendMessage(hWnd, LVM_GETITEMTEXT, iclk.iItem, (LPARAM)&lcur);
                SendMessage(hWnd = GetDlgItem(hWnd, SED_LIST), WM_SETTEXT, 0, (LPARAM)lcur.pszText);
                MoveWindow(hWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
                ShowWindow(hWnd, SW_SHOW);
                SetFocus(hWnd);
                SendMessage(hWnd, EM_SETSEL, 0, -1);
            }
            return 0;
        }

        default:
            return CallWindowProc(lprc, hWnd, uMsg, wPrm, lPrm);
    }
}



BOOL CALLBACK GraphicsProc(HWND hDlg, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            DWORD indx, icnt, temp;
            CHAR mstr[32];
            DEVMODE mode;
            HANDLE file;
            TDVM dvmt;

            SendDlgItemMessage(hDlg, GLB_SCRN, CB_ADDSTRING, 0, (LPARAM)"Full screen");
            SendDlgItemMessage(hDlg, GLB_SCRN, CB_ADDSTRING, 0, (LPARAM)"Wide screen");
            SendDlgItemMessage(hDlg, GLB_SCRN, CB_ADDSTRING, 0, (LPARAM)"Window");

            SendDlgItemMessage(hDlg, GLB_POLY, CB_ADDSTRING, 0, (LPARAM)"Texture");
            SendDlgItemMessage(hDlg, GLB_POLY, CB_ADDSTRING, 0, (LPARAM)"Flat");
            SendDlgItemMessage(hDlg, GLB_POLY, CB_ADDSTRING, 0, (LPARAM)"Wireframe");

            icnt = 0;
            while (EnumDisplaySettings(NULL, icnt++, &mode));
            allm = (TDVM*)calloc(icnt + NUM_LRES + 1, sizeof(*allm));

            icnt = 0;
            while (EnumDisplaySettings(NULL, icnt++, &mode)) {
                allm[icnt - 1].dbit = mode.dmBitsPerPel;
                allm[icnt - 1].dhei = mode.dmPelsHeight;
                allm[icnt - 1].dwid = mode.dmPelsWidth;
            }

            for (icnt--, indx = 0; indx < NUM_LRES; indx++) {
                mode.dmBitsPerPel = lres[indx].dbit;
                mode.dmPelsHeight = lres[indx].dhei;
                mode.dmPelsWidth  = lres[indx].dwid;
                mode.dmFields     = DM_BITSPERPEL | DM_PELSHEIGHT | DM_PELSWIDTH;
                if (mode.dmBitsPerPel & ~((DWORD)~0 >> 1)) mode.dmBitsPerPel = ~mode.dmBitsPerPel;
                else if (ChangeDisplaySettings(&mode, CDS_TEST | CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
                    continue;

                allm[icnt].dbit = mode.dmBitsPerPel;
                allm[icnt].dhei = mode.dmPelsHeight;
                allm[icnt].dwid = mode.dmPelsWidth;
                icnt++;
            }
            for (qsort(allm, temp = icnt, sizeof(*allm), CompareTDVM), indx = icnt = 0; indx < temp; indx++)
                if (CompareTDVM(&allm[indx], &allm[indx + 1])) {
                    sprintf(mstr, "%lu x %lu [%lu bit]", allm[indx].dwid, allm[indx].dhei, allm[indx].dbit);
                    SendDlgItemMessage(hDlg, GLB_TDVM, CB_ADDSTRING, 0, (LPARAM)mstr);
                    allm[icnt++] = allm[indx];
                }

            if ((file = TryOpenFile(path, DEF_CFGP, RCD_CONF, GENERIC_READ, hDlg)) == INVALID_HANDLE_VALUE)
                return FALSE;

            SetFilePointer(file, PTR_SCRN, NULL, FILE_BEGIN);
            ReadFile(file, &dvmt, sizeof(dvmt), &indx, NULL);
            SendDlgItemMessage(hDlg, GLB_SCRN, CB_SETCURSEL, (dvmt.dwid)? (dvmt.dhei)? 1 : 0 : 2, 0);

            SetFilePointer(file, PTR_FVDM, NULL, FILE_BEGIN);
            ReadFile(file, &dvmt, sizeof(dvmt), &indx, NULL);
            for (indx = 0; indx < icnt; indx++)
                if (allm[indx].dbit == dvmt.dbit && allm[indx].dhei == dvmt.dhei && allm[indx].dwid == dvmt.dwid)
                    break;
            if (indx >= icnt) indx = 0;
            SendDlgItemMessage(hDlg, GLB_TDVM, CB_SETCURSEL, indx, 0);

            SetFilePointer(file, PTR_POLY, NULL, FILE_BEGIN);
            ReadFile(file, &dvmt, sizeof(dvmt), &indx, NULL);
            if (dvmt.dwid > 2) dvmt.dwid = 0;
            CloseHandle(file);

            SendDlgItemMessage(hDlg, GLB_POLY, CB_SETCURSEL, dvmt.dwid, 0);
            SendDlgItemMessage(hDlg, GCB_2DBK, BM_SETCHECK, dvmt.dhei, 0);
            SendDlgItemMessage(hDlg, GCB_3DBK, BM_SETCHECK, dvmt.dbit, 0);
            EnableWindow(GetDlgItem(hDlg, GBT_REVP), CheckEXEValidity(path, DEF_EXES, tcrc, DEF_CRCO, DEF_CRCP) == EXE_PTCH);
            return TRUE;
        }


        case WM_COMMAND: {
            HWND hBtn = GetDlgItem(hDlg, GBT_REVP);
            DWORD temp, size;
            HANDLE file;

            switch (LOWORD(wPrm)) {
                case GBT_REVP:
                    PatchControl(path, EXE_PTCH, hDlg, hBtn);
                    return TRUE;

                case GBT_TEST: {
                    if ((file = TryOpenFile(path, DEF_CFGP, RCD_CONF, GENERIC_WRITE, hDlg)) == INVALID_HANDLE_VALUE)
                        return TRUE;
                    if ((temp = SendDlgItemMessage(hDlg, GLB_TDVM, CB_GETCURSEL, 0, 0)) == CB_ERR) temp = 0;
                    SetFilePointer(file, PTR_FVDM, NULL, FILE_BEGIN);
                    WriteFile(file, &allm[temp], sizeof(allm[temp]), &size, NULL);
                    SetFilePointer(file, PTR_WVDM, NULL, FILE_BEGIN);

                    RECT wrec = {(GetSystemMetrics(SM_CXSCREEN) - allm[temp].dwid) >> 1,
                                 (GetSystemMetrics(SM_CYSCREEN) - allm[temp].dhei) >> 1,
                                 allm[temp].dwid, allm[temp].dhei};
                    WriteFile(file, &wrec, sizeof(wrec), &size, NULL);

                    SetFilePointer(file, PTR_SCRN, NULL, FILE_BEGIN);
                    switch (SendDlgItemMessage(hDlg, GLB_SCRN, CB_GETCURSEL, 0, 0)) {
                        case 0:
                            temp = TRUE;
                            WriteFile(file, &temp, sizeof(temp), &size, NULL);
                            temp = FALSE;
                            WriteFile(file, &temp, sizeof(temp), &size, NULL);
                            break;

                        case 1:
                            temp = TRUE;
                            WriteFile(file, &temp, sizeof(temp), &size, NULL);
                            WriteFile(file, &temp, sizeof(temp), &size, NULL);
                            break;

                        default:
                            temp = FALSE;
                            WriteFile(file, &temp, sizeof(temp), &size, NULL);
                            WriteFile(file, &temp, sizeof(temp), &size, NULL);
                            break;
                    }

                    SetFilePointer(file, PTR_POLY, NULL, FILE_BEGIN);
                    if ((temp = SendDlgItemMessage(hDlg, GLB_POLY, CB_GETCURSEL, 0, 0)) == CB_ERR) temp = 0;
                    WriteFile(file, &temp, sizeof(temp), &size, NULL);
                    temp = SendDlgItemMessage(hDlg, GCB_2DBK, BM_GETCHECK, 0, 0);
                    WriteFile(file, &temp, sizeof(temp), &size, NULL);
                    temp = SendDlgItemMessage(hDlg, GCB_3DBK, BM_GETCHECK, 0, 0);
                    WriteFile(file, &temp, sizeof(temp), &size, NULL);

                    CloseHandle(file);
                    PatchControl(path, EXE_ORIG, hDlg, hBtn);
                    return TRUE;
                }

                default:
                    return FALSE;
            }
        }


        case WM_DESTROY:


        default:
            return FALSE;
    }
}



BOOL CALLBACK HiscoresProc(HWND hDlg, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            LV_COLUMN llvc = {LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM};
            HIMAGELIST list;
            HBITMAP hbmp;
            HANDLE file;
            RECT rect;
            HWND tree;

            tree = GetDlgItem(hDlg, SLV_LIST);
            GetClientRect(tree, &rect);
            llvc.cx = (FLOAT)(rect.right - rect.left) * 0.35;
            llvc.pszText = "Name";
            SendMessage(tree, LVM_INSERTCOLUMN, 0, (LPARAM)&llvc);
            llvc.pszText = "Time";
            llvc.cx = rect.right - rect.left - llvc.cx;
            SendMessage(tree, LVM_INSERTCOLUMN, 1, (LPARAM)&llvc);
            SendMessage(tree, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

            eprc = (WNDPROC)SetWindowLong(file = GetDlgItem(hDlg, SED_LIST), GWL_WNDPROC, (LONG)EditProc);
            lprc = (WNDPROC)SetWindowLong(tree, GWL_WNDPROC, (LONG)ListProc);
            SetParent(file, tree);

            tree = GetDlgItem(hDlg, STV_TREE);
            ImageList_AddMasked(list = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 0),
                                hbmp = LoadBitmap(base, MAKEINTRESOURCE(BMP_TREE)),
                                0xFF00FF);
            DeleteObject(hbmp);
            SendMessage(tree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)list);
            if ((file = TryOpenFile(path, DEF_SCRP, RCD_SCOR, GENERIC_READ, hDlg)) == INVALID_HANDLE_VALUE)
                return FALSE;

            FillHiscores(tree, file);
            CloseHandle(file);
            return TRUE;
        }

        case WM_NOTIFY: {
            LV_ITEM lcur = {};
            TV_ITEM tcur = {};
            HTREEITEM oldp, newp;
            switch (LOWORD(wPrm)) {
                case STV_TREE:
                    switch (((LPNMHDR)lPrm)->code) {
                        case TVN_SELCHANGED:
                            #define lnmt ((LPNMTREEVIEW)lPrm)
                            oldp = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)lnmt->itemOld.hItem);
                            newp = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)lnmt->itemNew.hItem);
                            if (!newp) {
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)((oldp)? oldp : lnmt->itemOld.hItem));
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_EXPAND, TVE_EXPAND, (LPARAM)lnmt->itemNew.hItem);
                            }
                            if (oldp != newp) {
                                tcur.mask = TVIF_IMAGE;
                                tcur.hItem = oldp;
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);
                                tcur.iImage = tcur.iSelectedImage = (tcur.iImage - IMG_FLDO)? (tcur.iImage - IMG_FLDC)? tcur.iImage : IMG_FLDO : IMG_FLDC;
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_SETITEM, 0, (LPARAM)&tcur);
                                tcur.hItem = newp;
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);
                                tcur.iImage = tcur.iSelectedImage = (tcur.iImage - IMG_FLDO)? (tcur.iImage - IMG_FLDC)? tcur.iImage : IMG_FLDO : IMG_FLDC;
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_SETITEM, 0, (LPARAM)&tcur);
                            }
                            SendDlgItemMessage(hDlg, SLV_LIST, LVM_DELETEALLITEMS, 0, 0);
                            if (HIWORD(lnmt->itemNew.lParam)) {
                                CHAR stmp[256];

                                tcur.mask = TVIF_PARAM;
                                if ((tcur.hItem = newp))
                                    SendDlgItemMessage(hDlg, STV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);
                                else
                                    tcur.lParam = -1;

                                lcur.cchTextMax = 256;
                                lcur.mask = LVIF_TEXT;
                                for (wPrm = 0; wPrm < ((tcur.lParam)? 5 : 4); wPrm++) {
                                    lcur.iItem = wPrm;
                                    lcur.iSubItem = 0;
                                    if (tcur.lParam)
                                        lcur.pszText = (LPSTR)(lnmt->itemNew.lParam + 12 * lcur.iItem);
                                    else {
                                        sprintf(stmp, "Lap %u", wPrm + 1);
                                        lcur.pszText = stmp;
                                    }
                                    SendDlgItemMessage(hDlg, SLV_LIST, LVM_INSERTITEM, 0, (LPARAM)&lcur);

                                    lcur.iSubItem = 1;
                                    lcur.pszText = stmp;

                                    if (tcur.lParam)
                                        uMsg = *((DWORD*)(lnmt->itemNew.lParam + 12 * lcur.iItem + 4)) + 327;
                                    else
                                        uMsg = *((DWORD*)(lnmt->itemNew.lParam + 4 * lcur.iItem)) + 327;

                                    if (!newp)
                                        sprintf(stmp, "%lu %02u:%02u:%02u", *((DWORD*)(lnmt->itemNew.lParam + 12 * lcur.iItem + 8)), HIWORD(uMsg) / 60, HIWORD(uMsg) % 60, HIWORD(LOWORD(uMsg) * 100));
                                    else
                                        sprintf(stmp, "%02u:%02u:%02u", HIWORD(uMsg) / 60, HIWORD(uMsg) % 60, HIWORD(LOWORD(uMsg) * 100));

                                    SendDlgItemMessage(hDlg, SLV_LIST, LVM_SETITEM, 0, (LPARAM)&lcur);
                                }
                            }
                            #undef lnmt
                            break;
                    }
                    break;

                case SLV_LIST:
                    switch (((LPNMHDR)lPrm)->code) {
                        case LVN_ENDLABELEDIT:
                            #define lnmt ((LPNMLVDISPINFO)lPrm)
                            tcur.hItem = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)(newp = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_CARET, 0)));
                            tcur.mask = TVIF_PARAM;
                            oldp = (HTREEITEM)-1;
                            if (tcur.hItem) {
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);
                                if (!tcur.lParam)
                                    oldp = NULL;
                            }
                            tcur.hItem = newp;
                            SendDlgItemMessage(hDlg, STV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);

                            lcur = lnmt->item;
                            lcur.mask = LVIF_TEXT;
                            if (lcur.iSubItem) {
                                LPSTR test = lcur.pszText;
                                if (SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)tcur.hItem))
                                    uMsg = 1000;
                                else {
                                    uMsg = strtoul(test, &test, 10);
                                    if (*test++ != ' ') uMsg = 0;
                                }
                                if (uMsg && ((wPrm = 60 * strtoul(test, &test, 10)) < 6000))
                                    if (*test == ':')
                                        if ((lPrm = strtoul(++test, &test, 10)) < 60) {
                                            wPrm += lPrm;
                                            if (*test == ':')
                                                if ((lPrm = 655 * strtoul(++test, &test, 10)/* + <overhead: [0; 363)> */) < 65500) {
                                                    SendDlgItemMessage(hDlg, SLV_LIST, LVM_SETITEM, 0, (LPARAM)&lcur);
                                                    if (!oldp)
                                                        *((DWORD*)(tcur.lParam + 4 * lcur.iItem)) = MAKELONG(LOWORD(lPrm), LOWORD(wPrm));
                                                    else {
                                                        *((DWORD*)(tcur.lParam + 12 * lcur.iItem + 4)) = MAKELONG(LOWORD(lPrm), LOWORD(wPrm));
                                                        *((DWORD*)(tcur.lParam + 12 * lcur.iItem + 8)) = uMsg;
                                                    }
                                                }
                                        }
                            }
                            else if (!(*((DWORD*)lcur.pszText) & 0xFF808080)) {
                                CharUpper(lcur.pszText);
                                SendDlgItemMessage(hDlg, SLV_LIST, LVM_SETITEM, 0, (LPARAM)&lcur);
                                *((DWORD*)(tcur.lParam + 12 * lcur.iItem)) = *((DWORD*)lcur.pszText);
                            }
                            #undef lnmt
                            break;
                    }
                    break;

                default:
                    switch (((LPNMHDR)lPrm)->code) {
                        case HDN_ITEMCHANGINGA:
                        case HDN_ITEMCHANGINGW:
                            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
                            return TRUE;
                    }
                    break;
            }
            return FALSE;
        }


        case WM_COMMAND: {
            TV_ITEM tcur = {};
            HTREEITEM temp;
            HANDLE file;
            BOOL laps;

            switch (LOWORD(wPrm)) {
                case SBT_SAVE:
                    tcur.mask = TVIF_PARAM;
                    tcur.hItem = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_ROOT, 0);
                    if ((file = TryOpenFile(path, DEF_SCRP, RCD_SCOR, GENERIC_WRITE, hDlg)) == INVALID_HANDLE_VALUE)
                        break;

                    while ((temp = tcur.hItem)) {
                        SendDlgItemMessage(hDlg, STV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);
                        if (HIWORD(tcur.lParam))
                            WriteChunk((LPSTR*)&tcur.lParam, file, 0, 60, 96);
                        else {
                            tcur.hItem = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)tcur.hItem);
                            laps = tcur.lParam;
                            for (uMsg = (laps)? 6 * tcur.lParam - 5 : 0; tcur.hItem; uMsg++) {
                                SendDlgItemMessage(hDlg, STV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);
                                WriteChunk((LPSTR*)&tcur.lParam, file, uMsg, (laps)? 60 : 16, (laps)? 96 : 0);
                                tcur.hItem = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)tcur.hItem);
                            }
                        }
                        tcur.hItem = (HTREEITEM)SendDlgItemMessage(hDlg, STV_TREE, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)temp);
                    }
                    CloseHandle(file);
                    break;

                case SBT_LOAD:
                    if ((file = TryOpenFile(path, DEF_SCRP, RCD_SCOR, GENERIC_READ, hDlg)) == INVALID_HANDLE_VALUE)
                        break;
                    hDlg = GetDlgItem(hDlg, STV_TREE);
                    FreeHiscores(hDlg, (HTREEITEM)SendMessage(hDlg, TVM_GETNEXTITEM, TVGN_ROOT, 0));
                    FillHiscores(hDlg, file);
                    CloseHandle(file);
                    break;
            }
            return FALSE;
        }


        case WM_DESTROY:
            hDlg = GetDlgItem(hDlg, STV_TREE);
            FreeHiscores(hDlg, (HTREEITEM)SendMessage(hDlg, TVM_GETNEXTITEM, TVGN_ROOT, 0));
            ImageList_Destroy((HIMAGELIST)SendMessage(hDlg, TVM_GETIMAGELIST, TVSIL_NORMAL, 0));


        default:
            return FALSE;
    }
}



BOOL CALLBACK ResourceProc(HWND hDlg, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            HWND tree = GetDlgItem(hDlg, RTV_TREE);
            DWORD read, fcnt, temp;
            HIMAGELIST list;
            HANDLE file;

            rtcm = CreatePopupMenu();
            AppendMenu(rtcm, MF_STRING, RCM_SAVE, "&Save to file...");
            AppendMenu(rtcm, MF_STRING, RCM_LOAD, "&Load from file...");
            ImageList_AddMasked(list = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 0),
                                file = LoadBitmap(base, MAKEINTRESOURCE(BMP_TREE)),
                                0xFF00FF);
            DeleteObject(file);
            SendMessage(tree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)list);
            if ((file = TryOpenFile(path, DEF_RESP, 0, GENERIC_READ, hDlg)) == INVALID_HANDLE_VALUE)
                return FALSE;

            root = iter = (PFSN*)calloc(1, sizeof(*iter));
            for (fcnt = 0, ReadFile(file, &iter->head, sizeof(iter->head), &read, NULL); iter->head.fhsz + 1;
                 fcnt++,   ReadFile(file, &iter->head, sizeof(iter->head), &read, NULL)) {
                iter->head.fnsz = swap32(iter->head.fnsz);
                iter->head.ffsz = swap32(iter->head.ffsz);
                iter->head.fpos = swap32(iter->head.fpos);

                ReadFile(file, iter->name = (LPSTR)malloc(iter->head.fnsz - sizeof(iter->head)), iter->head.fnsz - sizeof(iter->head), &read, NULL);
                temp = SetFilePointer(file, 0, NULL, FILE_CURRENT);
                SetFilePointer(file, iter->head.fpos, NULL, FILE_BEGIN);
                ReadFile(file, iter->file = malloc(iter->head.ffsz), iter->head.ffsz, &read, NULL);
                SetFilePointer(file, temp, NULL, FILE_BEGIN);

                FillResource(tree, iter);
                iter->next = (PFSN*)calloc(1, sizeof(*iter->next));
                iter = iter->next;
            }
            CloseHandle(file);

            iter = root;
            temp = sizeof(iter->head.fhsz);
            while (iter->head.fhsz + 1) {
                iter->head.fhsz = sizeof(iter->head);
                temp += iter->head.fnsz;
                iter = iter->next;
            }

            iter = root;
            fcnt = temp;
            while (iter->head.fhsz + 1) {
                iter->head.fpos = fcnt;
                fcnt += iter->head.ffsz;
                iter = iter->next;
            }

ПРАВИТЬ ЗДЕСЬ

            file = TryOpenFile(path, DEF_RESP, 0, GENERIC_WRITE, hDlg);
            iter = root;
            while (iter->head.fhsz + 1) {
                iter->head.fhsz = swap32(iter->head.fhsz);
                iter->head.fpos = swap32(iter->head.fpos);
                iter->head.ffsz = swap32(iter->head.ffsz);
                iter->head.fnsz = swap32(iter->head.fnsz);
                WriteFile(file, &iter->head, sizeof(iter->head), &read, NULL);
                iter->head.fhsz = swap32(iter->head.fhsz);
                iter->head.fpos = swap32(iter->head.fpos);
                iter->head.ffsz = swap32(iter->head.ffsz);
                iter->head.fnsz = swap32(iter->head.fnsz);
                WriteFile(file, iter->name, iter->head.fnsz - sizeof(iter->head), &read, NULL);
                iter = iter->next;
            }
            WriteFile(file, &iter->head.fhsz, sizeof(iter->head.fhsz), &read, NULL);
            iter = root;
            while (iter->head.fhsz + 1) {
                WriteFile(file, iter->file, iter->head.ffsz, &read, NULL);
                iter = iter->next;
            }
            SetEndOfFile(file);
            CloseHandle(file);

            return TRUE;
        }


        case WM_CONTEXTMENU:
            if (GetDlgItem(hDlg, RTV_TREE) == (HWND)wPrm) {
                TV_HITTESTINFO tvhi = {{LOWORD(lPrm), HIWORD(lPrm)}};
                ScreenToClient((HWND)wPrm, &tvhi.pt);
                SendMessage((HWND)wPrm, TVM_HITTEST, 0, (LPARAM)&tvhi);
                if (tvhi.flags & TVHT_ONITEM) {
                    SendMessage((HWND)wPrm, TVM_SELECTITEM, TVGN_CARET, (LPARAM)(rtci = tvhi.hItem));
                    TrackPopupMenuEx(rtcm, TPM_LEFTALIGN | TPM_RIGHTBUTTON, LOWORD(lPrm), HIWORD(lPrm), hDlg, NULL);
                }
            }
            return FALSE;


        case WM_COMMAND: {
            TV_ITEM tcur = {TVIF_PARAM, rtci};
            SendDlgItemMessage(hDlg, RTV_TREE, TVM_GETITEM, 0, (LPARAM)&tcur);
            if (tcur.lParam & 0xFFFF0000) {
                CHAR name[MAX_PATH + 1] = {};
                OPENFILENAME open = {sizeof(OPENFILENAME), hDlg, base, NULL, NULL, 0, 0, (LPSTR)&name, MAX_PATH, NULL, 0, NULL, NULL, OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY};

                wPrm = LOWORD(wPrm) - RCM_SAVE;
                open.nFilterIndex = 1;
                open.lpstrFilter = "All Files\0*.*\0\0";
                if (wPrm) {
                    open.lpstrTitle = "Open...";
                    open.Flags |= OFN_FILEMUSTEXIST;
                }
                else {
                    open.lpstrTitle = "Save...";
                }

                if (((wPrm)? GetOpenFileName : GetSaveFileName)(&open)) {
                    HANDLE file;
                    if ((file = TryOpenFile(name, "", 0, (wPrm)? GENERIC_READ : GENERIC_WRITE, hDlg)) != INVALID_HANDLE_VALUE) {
                        if (wPrm) {
                            free(((PFSN)tcur.lPrm).file);
                            ((PFSN)tcur.lPrm).head.ffsz = SetFilePointer(file, 0, NULL, FILE_END);
                        }
                        ((wPrm)? ReadChunk : WriteChunk)((LPSTR*)&((PFSN)tcur.lPrm).file, file, 0, ((PFSN)tcur.lPrm).head.ffsz, 0);
                        CloseHandle(file);
                    }
                }
            }
            else {
            }
            return FALSE;
        }


        case WM_DESTROY:
            DestroyMenu(rtcm);
            ImageList_Destroy((HIMAGELIST)SendMessage(GetDlgItem(hDlg, STV_TREE), TVM_GETIMAGELIST, TVSIL_NORMAL, 0));


        default:
            return FALSE;
    }
}



BOOL CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wPrm, LPARAM lPrm) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            HRESULT CALLBACK (*ETDT)(HWND, DWORD) = (HRESULT CALLBACK (*)(HWND, DWORD))GetProcAddress(LoadLibrary("uxtheme.dll"), "EnableThemeDialogTexture");
            INITCOMMONCONTROLSEX ccex = {sizeof(ccex), ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES};
            HKEY rkey = NULL;
            TCITEM mtci;
            DWORD temp;
            HWND hTab;

            GenCRC32(tcrc, DEF_CRCL);
            InitCommonControlsEx(&ccex);
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(base, MAKEINTRESOURCE(ICN_MAIN)));

            do {
                RegCreateKeyEx(HKEY_CURRENT_USER, DEF_REGP, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &rkey, &temp);
                if (temp == REG_CREATED_NEW_KEY) {
                    GetCurrentDirectory(MAX_PATH, path);
                    if (!ShFolderOpen(hDlg, "Choose your Scorcher directory:", path)) {
                        RegCloseKey(rkey);
                        Terminate(hDlg, TRUE, FALSE);
                        return FALSE;
                    }
                    RegSetValueEx(rkey, DEF_INSP, 0, REG_SZ, (LPBYTE)path, strlen(path) + 1);
                }
                temp = MAX_PATH;
                RegQueryValueEx(rkey, DEF_INSP, 0, NULL, (LPBYTE)path, &temp);
                RegCloseKey(rkey);

                if (!(temp = strlen(path)))
                    RegDeleteKey(HKEY_CURRENT_USER, DEF_REGP);
            } while (!temp);

            hTab = GetDlgItem(hDlg, MTC_TABS);

            mtci.mask = TCIF_TEXT;
            for (temp = NUM_TABS; temp > 0; temp--) {
                mtci.pszText = tabs[temp - 1].capt;
                tabs[temp - 1].htab = CreateDialogParam(base, MAKEINTRESOURCE(tabs[temp - 1].indx), hTab, tabs[temp - 1].proc, 0);
                if (ETDT) ETDT(tabs[temp - 1].htab, 0x06);
                SendMessage(hTab, TCM_INSERTITEM, 0, (LPARAM)&mtci);
            }
            temp = 0;
            ShowWindow(tabs[temp].htab, TRUE);
            SendMessage(hTab, TCM_SETCURSEL, temp, 0);
            return TRUE;
        }


        case WM_NOTIFY:
            #define nmhd ((NMHDR*)lPrm)
            if ((nmhd->idFrom == MTC_TABS) && (nmhd->code == TCN_SELCHANGE)) {
                wPrm = SendMessage(nmhd->hwndFrom, TCM_GETCURSEL, 0, 0);
                for (lPrm = 0; lPrm < NUM_TABS; lPrm++)
                    ShowWindow(tabs[lPrm].htab, lPrm == wPrm);
            }
            return FALSE;
            #undef nmhd


        case WM_COMMAND:
            switch (LOWORD(wPrm)) {
                case MBT_EXEC: {
                    CHAR name[MAX_PATH + 3];
                    sprintf(name, "\"%s%s\"", path, DEF_EXEP);
                    ShellExecute(hDlg, NULL, name, NULL, path, SW_SHOW);
                    return TRUE;
                }

                case MBT_QUIT:
                    Terminate(hDlg, FALSE, FALSE);
                    return TRUE;

                case MBT_LOAD:
                    if (MessageBoxIcon(hDlg, "Are you sure?", "Confirmation", MB_YESNO, MAKEINTRESOURCE(ICN_MAIN)) == IDYES)
                        Terminate(hDlg, TRUE, TRUE);
                    return TRUE;

                case MBT_INFO:
                    MessageBoxIcon(hDlg, "Scorcher© helper tool, v0.2\nGreetz to CTPAX-X team!\n\nhidefromkgb, 2013\nhidefromkgb@gmail.com", "A bit of info =)", MB_OK, MAKEINTRESOURCE(ICN_MAIN));
                    return TRUE;

                default:
                    return FALSE;
            }


        case WM_CLOSE:
            Terminate(hDlg, FALSE, FALSE);
            return FALSE;


        default:
            return FALSE;
    }
}



int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdl, int show) {
//    AllocConsole();
//    freopen("CONOUT$", "wb", stdout);
    show = DialogBox(base = inst, MAKEINTRESOURCE(DLG_MAIN), NULL, DialogProc);
//    fclose(stdout);
//    FreeConsole();
    return show;
}
