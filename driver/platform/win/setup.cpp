#define POCO_NO_UNWINDOWS

#include "driver/platform/platform.h"
#include "driver/platform/win/resource.h"
#include "driver/utils/include/utils.h"
#include "driver/utils/include/conversion.h"
#include "driver/config/config.h"
#include "driver/config/ini_defines.h"

#include <Poco/UTF8String.h>

#include <odbcinst.h>
#include <Commctrl.h>

#include <algorithm>

#if defined(_win_)

#    include <strsafe.h>

/// Saved module handle.
extern HINSTANCE module_instance;

extern "C" {

INT_PTR CALLBACK ConfigDlgProc(
    HWND hdlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
);

} // extern "C"

namespace { namespace impl {

/* NOTE:  All these are used by the dialog procedures */
struct SetupDialogData {
    HWND hwnd_parent;        /// Parent window handle
    std::string driver_name; /// Driver description
    std::string dsn;         /// Original data source name
    bool is_new_dsn;         /// New data source flag
    bool is_default;         /// Default data source flag
    int source_type;         // 0: Unknown, 1: Capella Ops, 2: Capella Analytics, 3: CB Server, 4: Ent Analytics
    ConnInfo ci;

     // Member function to get LPARAM
    LPARAM GetAsLPARAM() const {
        return reinterpret_cast<LPARAM>(const_cast<SetupDialogData*>(this));
    }
};

void ConfigureScopeUI(HWND hdlg, int sourceType, const std::string& catalog) {
    // sourceType:
    // 1: Capella Ops (Scope only)
    // 2: Capella Analytics (Database + Optional Scope)
    // 3: CB Server (Scope only)
    // 4: Ent Analytics (Database + Optional Scope)

    bool isDatabaseMode = (sourceType == 2 || sourceType == 4);

    if (isDatabaseMode) {
        SetDlgItemText(hdlg, IDC_CATALOG_LABEL, TEXT("Database:"));
        ShowWindow(GetDlgItem(hdlg, IDC_SCOPE), SW_SHOW);
        ShowWindow(GetDlgItem(hdlg, IDC_SCOPE_LABEL), SW_SHOW);

        // Split catalog if needed
        size_t slashPos = catalog.find('/');
        if (slashPos != std::string::npos) {
            std::string db = catalog.substr(0, slashPos);
            std::string scope = catalog.substr(slashPos + 1);

            std::basic_string<CharTypeLPCTSTR> val;
            fromUTF8(db, val);
            SetDlgItemText(hdlg, IDC_CATALOG, val.c_str());

            fromUTF8(scope, val);
            SetDlgItemText(hdlg, IDC_SCOPE, val.c_str());
        }
        Edit_SetCueBannerText(GetDlgItem(hdlg, IDC_SCOPE), L"Optional");
    } else {
        SetDlgItemText(hdlg, IDC_CATALOG_LABEL, TEXT("Scope:"));
        ShowWindow(GetDlgItem(hdlg, IDC_SCOPE), SW_HIDE);
        ShowWindow(GetDlgItem(hdlg, IDC_SCOPE_LABEL), SW_HIDE);
    }
}

inline BOOL copyAttributes(ConnInfo * ci, LPCTSTR attribute, LPCTSTR value) {
    const auto attribute_str = toUTF8(attribute);

#define COPY_ATTR_IF(NAME, INI_NAME)                          \
    if (Poco::UTF8::icompare(attribute_str, INI_NAME) == 0) { \
        ci->NAME = toUTF8(value);                             \
        return TRUE;                                          \
    }

    COPY_ATTR_IF(drivername,            INI_DRIVER);
    COPY_ATTR_IF(dsn,                   INI_DSN);
    COPY_ATTR_IF(desc,                  INI_DESC);
    COPY_ATTR_IF(url,                   INI_URL);
    COPY_ATTR_IF(server,                INI_SERVER);
    COPY_ATTR_IF(username,              INI_USERNAME);
    COPY_ATTR_IF(username,              INI_UID);
    COPY_ATTR_IF(password,              INI_PASSWORD);
    COPY_ATTR_IF(password,              INI_PWD);
    COPY_ATTR_IF(timeout,               INI_TIMEOUT);
    COPY_ATTR_IF(sslmode,               INI_SSLMODE);
    COPY_ATTR_IF(client_key_password,   INI_CLIENT_KEY_PASSWORD);
    COPY_ATTR_IF(database,              INI_DATABASE);
    COPY_ATTR_IF(scope,                 INI_SCOPE);
    COPY_ATTR_IF(sid,                   INI_SOURCE_ID);
    COPY_ATTR_IF(login_timeout,         INI_LOGIN_TIMEOUT);
    COPY_ATTR_IF(query_timeout,         INI_QUERY_TIMEOUT);

#undef COPY_ATTR_IF

    return FALSE;
}

inline void parseAttributes(LPCTSTR lpszAttributes, SetupDialogData * lpsetupdlg) {
    LPCTSTR lpsz;
    LPCTSTR lpszStart;
    TCHAR aszKey[MAX_DSN_KEY_LEN];
    int cbKey;
    TCHAR value[MAX_DSN_VALUE_LEN];

    for (lpsz = lpszAttributes; *lpsz; lpsz++) {
        /*
        * Extract key name (e.g., DSN), it must be terminated by an
        * equals
        */
        lpszStart = lpsz;
        for (;; lpsz++) {
            if (!*lpsz)
                return; /* No key was found */
            else if (*lpsz == '=')
                break; /* Valid key found */
        }

        /* Determine the key's index in the key table (-1 if not found) */
        cbKey = lpsz - lpszStart;
        if (cbKey < sizeof(aszKey)) {
            memcpy(aszKey, lpszStart, cbKey * sizeof(TCHAR));
            aszKey[cbKey] = '\0';
        }

        /* Locate end of key value */
        lpszStart = ++lpsz;
        for (; *lpsz; lpsz++)
            ;

        /* lpsetupdlg->aAttr[iElement].fSupplied = TRUE; */
        memcpy(value, lpszStart, std::min<size_t>(lpsz - lpszStart + 1, MAX_DSN_VALUE_LEN) * sizeof(TCHAR));

        /* Copy the appropriate value to the conninfo  */
        copyAttributes(&lpsetupdlg->ci, aszKey, value);

        //if (copyAttributes(&lpsetupdlg->ci, aszKey, value) != TRUE)
        //    copyCommonAttributes(&lpsetupdlg->ci, aszKey, value);
    }
}

inline BOOL setDSNAttributes(HWND hwndParent, SetupDialogData * lpsetupdlg, DWORD * errcode) {
    std::basic_string<CharTypeLPCTSTR> Driver;
    fromUTF8(lpsetupdlg->driver_name, Driver);

    std::basic_string<CharTypeLPCTSTR> DSN;
    fromUTF8(lpsetupdlg->ci.dsn, DSN);

    if (errcode)
        *errcode = 0;

    /* Validate arguments */
    if (lpsetupdlg->is_new_dsn && lpsetupdlg->ci.dsn.empty())
        return FALSE;

    if (!SQLValidDSN(DSN.c_str()))
        return FALSE;

    /* Write the data source name */
    if (!SQLWriteDSNToIni(DSN.c_str(), Driver.c_str())) {
        RETCODE ret = SQL_ERROR;
        DWORD err = SQL_ERROR;
        TCHAR szMsg[SQL_MAX_MESSAGE_LENGTH];

        ret = SQLInstallerError(1, &err, szMsg, sizeof(szMsg), NULL);
        if (hwndParent) {
            if (SQL_SUCCESS != ret)
                MessageBox(hwndParent, szMsg, TEXT("Bad DSN configuration"), MB_ICONEXCLAMATION | MB_OK);
        }

        if (errcode)
            *errcode = err;

        return FALSE;
    }

    /* Update ODBC.INI */
    writeDSNinfo(&lpsetupdlg->ci);

    /* If the data source name has changed, remove the old name */
    if (Poco::UTF8::icompare(lpsetupdlg->dsn, lpsetupdlg->ci.dsn) != 0) {
        fromUTF8(lpsetupdlg->dsn, DSN);
        SQLRemoveDSNFromIni(DSN.c_str());
    }

    return TRUE;
}

inline void CenterDialog(HWND hdlg) {
    HWND hwndFrame;
    RECT rcDlg;
    RECT rcScr;
    RECT rcFrame;
    int cx;
    int cy;

    hwndFrame = GetParent(hdlg);

    GetWindowRect(hdlg, &rcDlg);
    cx = rcDlg.right - rcDlg.left;
    cy = rcDlg.bottom - rcDlg.top;

    GetClientRect(hwndFrame, &rcFrame);
    ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.left));
    ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.right));
    rcDlg.top = rcFrame.top + (((rcFrame.bottom - rcFrame.top) - cy) >> 1);
    rcDlg.left = rcFrame.left + (((rcFrame.right - rcFrame.left) - cx) >> 1);
    rcDlg.bottom = rcDlg.top + cy;
    rcDlg.right = rcDlg.left + cx;

    GetWindowRect(GetDesktopWindow(), &rcScr);
    if (rcDlg.bottom > rcScr.bottom) {
        rcDlg.bottom = rcScr.bottom;
        rcDlg.top = rcDlg.bottom - cy;
    }
    if (rcDlg.right > rcScr.right) {
        rcDlg.right = rcScr.right;
        rcDlg.left = rcDlg.right - cx;
    }

    if (rcDlg.left < 0)
        rcDlg.left = 0;
    if (rcDlg.top < 0)
        rcDlg.top = 0;

    MoveWindow(hdlg, rcDlg.left, rcDlg.top, cx, cy, TRUE);
    return;
}

void ToggleCertificateVisibility(HWND hdlg, bool show) {
    int cmdShow = show ? SW_SHOW : SW_HIDE;
    // Hide/Show the Label ("Certificate File Path:")
    ShowWindow(GetDlgItem(hdlg, IDC_CERTIFICATEFILE_LABEL), cmdShow);
    // Hide/Show the Text Box
    ShowWindow(GetDlgItem(hdlg, IDC_CERTIFICATEFILE), cmdShow);
}

void UpdateAuthVisibility(HWND hdlg, int authMode) {
    // authMode:
    // 0 = Basic
    // 1 = LDAP
    // 2 = Client Cert
    // 3 = Client Cert + Encrypted Key

    bool showBasic = (authMode == 0 || authMode == 1);
    bool showCert  = (authMode == 2 || authMode == 3);
    bool showPass  = (authMode == 3);

    int basicCmd = showBasic ? SW_SHOW : SW_HIDE;
    int certCmd  = showCert  ? SW_SHOW : SW_HIDE;
    int passCmd  = showPass  ? SW_SHOW : SW_HIDE;

    // Basic Auth Fields
    ShowWindow(GetDlgItem(hdlg, IDC_USER), basicCmd);
    ShowWindow(GetDlgItem(hdlg, IDC_USER_LABEL), basicCmd);
    ShowWindow(GetDlgItem(hdlg, IDC_PASSWORD), basicCmd);
    ShowWindow(GetDlgItem(hdlg, IDC_PASSWORD_LABEL), basicCmd);

    // Client Cert Fields
    ShowWindow(GetDlgItem(hdlg, IDC_CLIENT_CERT), certCmd);
    ShowWindow(GetDlgItem(hdlg, IDC_CLIENT_CERT_LABEL), certCmd);
    ShowWindow(GetDlgItem(hdlg, IDC_CLIENT_KEY), certCmd);
    ShowWindow(GetDlgItem(hdlg, IDC_CLIENT_KEY_LABEL), certCmd);

    // Encrypted Key Password Field
    ShowWindow(GetDlgItem(hdlg, IDC_CLIENT_KEY_PASSWORD), passCmd);
    ShowWindow(GetDlgItem(hdlg, IDC_CLIENT_KEY_PASSWORD_LABEL), passCmd);
}

inline INT_PTR ConfigDlgProc_(
    HWND hdlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
) noexcept {
    switch (wMsg) {
        case WM_INITDIALOG: {
            auto & lpsetupdlg = *(SetupDialogData *)lParam;
            auto & ci = lpsetupdlg.ci;

            SetWindowLongPtr(hdlg, DWLP_USER, lParam);
            CenterDialog(hdlg); /* Center dialog */
            readDSNinfo(&ci, false);

            std::basic_string<CharTypeLPCTSTR> value;

            HWND hSSLMode = GetDlgItem(hdlg, IDC_SSLMODE);
            SendMessage(hSSLMode, CB_ADDSTRING, 0, (LPARAM)TEXT("Disable")); // Index 0
            SendMessage(hSSLMode, CB_ADDSTRING, 0, (LPARAM)TEXT("Enable"));  // Index 1

            bool sslEnabled = (ci.sslmode == "1" || Poco::UTF8::icompare(ci.sslmode, "require") == 0);
            SendMessage(hSSLMode, CB_SETCURSEL, sslEnabled ? 1 : 0, 0);
            // [CALL 1] Set initial visibility based on loaded config
            ToggleCertificateVisibility(hdlg, sslEnabled);


            // Setup Auth Dropdown
            HWND hAuth = GetDlgItem(hdlg, IDC_AUTH_MODE);
            SendMessage(hAuth, CB_ADDSTRING, 0, (LPARAM)TEXT("Basic")); // Index 0
            SendMessage(hAuth, CB_ADDSTRING, 0, (LPARAM)TEXT("LDAP")); // Index 1
            SendMessage(hAuth, CB_ADDSTRING, 0, (LPARAM)TEXT("Client Certificate")); // Index 2
            SendMessage(hAuth, CB_ADDSTRING, 0, (LPARAM)TEXT("Client Certificate (Encrypted Key)")); // Index 3

            // Determine initial selection based on config
            int initialAuth = 0;
            if (Poco::UTF8::icompare(ci.auth_mode, "certificate") == 0) {
                if (!ci.client_key_password.empty()) {
                    initialAuth = 3;
                } else {
                    initialAuth = 2;
                }
            } else if (Poco::UTF8::icompare(ci.auth_mode, "ldap") == 0) {
                initialAuth = 1;
            }
            SendMessage(hAuth, CB_SETCURSEL, initialAuth, 0);

            UpdateAuthVisibility(hdlg, initialAuth);

#define SET_DLG_ITEM(NAME, ID)                                    \
    {                                                             \
        value.clear();                                            \
        fromUTF8(ci.NAME, value);                                 \
        const auto res = SetDlgItemText(hdlg, ID, value.c_str()); \
    }

            SET_DLG_ITEM(dsn,                   IDC_DSN_NAME);
            SET_DLG_ITEM(desc,                  IDC_DESCRIPTION);
            SET_DLG_ITEM(url,                   IDC_URL);
            SET_DLG_ITEM(server,                IDC_SERVER_Address);
            SET_DLG_ITEM(username,              IDC_USER);
            SET_DLG_ITEM(password,              IDC_PASSWORD);
            SET_DLG_ITEM(sslmode,               IDC_SSLMODE);
            SET_DLG_ITEM(certificate_file,      IDC_CERTIFICATEFILE);
            SET_DLG_ITEM(client_cert,           IDC_CLIENT_CERT);
            SET_DLG_ITEM(client_key,            IDC_CLIENT_KEY);
            SET_DLG_ITEM(advanced_params,       IDC_ADVANCED_PARAMS);
            SET_DLG_ITEM(client_key_password,   IDC_CLIENT_KEY_PASSWORD);

#undef SET_DLG_ITEM

            bool isDatabaseMode = (lpsetupdlg.source_type == 2 || lpsetupdlg.source_type == 4);
            if (isDatabaseMode) {
                SetDlgItemText(hdlg, IDC_CATALOG_LABEL, TEXT("Database:"));
                ShowWindow(GetDlgItem(hdlg, IDC_SCOPE), SW_SHOW);
                ShowWindow(GetDlgItem(hdlg, IDC_SCOPE_LABEL), SW_SHOW);

                std::basic_string<CharTypeLPCTSTR> val;
                fromUTF8(ci.database, val);
                SetDlgItemText(hdlg, IDC_CATALOG, val.c_str());

                fromUTF8(ci.scope, val);
                SetDlgItemText(hdlg, IDC_SCOPE, val.c_str());

                Edit_SetCueBannerText(GetDlgItem(hdlg, IDC_SCOPE), L"Optional");
            } else {
                SetDlgItemText(hdlg, IDC_CATALOG_LABEL, TEXT("Scope:"));
                ShowWindow(GetDlgItem(hdlg, IDC_SCOPE), SW_HIDE);
                ShowWindow(GetDlgItem(hdlg, IDC_SCOPE_LABEL), SW_HIDE);

                std::basic_string<CharTypeLPCTSTR> val;
                fromUTF8(ci.scope, val);
                SetDlgItemText(hdlg, IDC_CATALOG, val.c_str());
            }

            LPCWSTR placeholder = L"<n/a for Power BI>";
            Edit_SetCueBannerText(GetDlgItem(hdlg, IDC_USER), placeholder);
            Edit_SetCueBannerText(GetDlgItem(hdlg, IDC_PASSWORD), placeholder);
            Edit_SetCueBannerText(GetDlgItem(hdlg, IDC_CLIENT_KEY_PASSWORD), placeholder);
            return TRUE;
        }

        case WM_COMMAND: {
            // [CALL 2] Handle Dropdown Change Event
            if (LOWORD(wParam) == IDC_SSLMODE && HIWORD(wParam) == CBN_SELCHANGE) {
                HWND hSSLMode = GetDlgItem(hdlg, IDC_SSLMODE);
                int selectedIndex = SendMessage(hSSLMode, CB_GETCURSEL, 0, 0);
                // Index 1 is "Enable"
                ToggleCertificateVisibility(hdlg, selectedIndex == 1);
                return TRUE;
            }
            switch (const DWORD cmd = LOWORD(wParam)) {
                case IDC_AUTH_MODE: {
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        HWND hAuth = GetDlgItem(hdlg, IDC_AUTH_MODE);
                        int idx = SendMessage(hAuth, CB_GETCURSEL, 0, 0);
                        UpdateAuthVisibility(hdlg, idx);
                    }
                    break;
                }
                case IDOK: {
                    auto & lpsetupdlg = *(SetupDialogData *)GetWindowLongPtr(hdlg, DWLP_USER);
                    auto & ci = lpsetupdlg.ci;
                    // Handle SSL Mode ComboBox
                    HWND hSSLMode = GetDlgItem(hdlg, IDC_SSLMODE);
                    int selectedIndex = SendMessage(hSSLMode, CB_GETCURSEL, 0, 0);
                    if (selectedIndex == 1) {
                        ci.sslmode = "1"; // or "yes"
                    } else {
                        ci.sslmode = "0"; // or "no"
                    }

                    // Save Auth Mode
                    HWND hAuth = GetDlgItem(hdlg, IDC_AUTH_MODE);
                    int authIdx = SendMessage(hAuth, CB_GETCURSEL, 0, 0);
                    if (authIdx == 2 || authIdx == 3) {
                        ci.auth_mode = "certificate";
                    } else if (authIdx == 1) {
                        ci.auth_mode = "ldap";
                    } else {
                        ci.auth_mode = "basic";
                    }

                    std::basic_string<CharTypeLPCTSTR> value;

#define GET_DLG_ITEM(NAME, ID)                                                   \
    {                                                                            \
        value.clear();                                                           \
        value.resize(MAX_DSN_VALUE_LEN);                                         \
        const auto read = GetDlgItemText(hdlg, ID, const_cast<CharTypeLPCTSTR *>(value.data()), value.size()); \
        value.resize((read <= 0 || read > value.size()) ? 0 : read);             \
        ci.NAME = toUTF8(value);                                                 \
    }

                    GET_DLG_ITEM(dsn,                   IDC_DSN_NAME);
                    GET_DLG_ITEM(desc,                  IDC_DESCRIPTION);
                    GET_DLG_ITEM(url,                   IDC_URL);
                    GET_DLG_ITEM(server,                IDC_SERVER_Address);
                    GET_DLG_ITEM(username,              IDC_USER);
                    GET_DLG_ITEM(password,              IDC_PASSWORD);
                    GET_DLG_ITEM(certificate_file,      IDC_CERTIFICATEFILE);
                    GET_DLG_ITEM(client_cert,           IDC_CLIENT_CERT);
                    GET_DLG_ITEM(client_key,            IDC_CLIENT_KEY);
                    GET_DLG_ITEM(advanced_params,       IDC_ADVANCED_PARAMS);
                    GET_DLG_ITEM(client_key_password,   IDC_CLIENT_KEY_PASSWORD);

#undef GET_DLG_ITEM

                    // Handle Scope/Database saving
                    ci.database.clear(); // Clear legacy database
                    ci.scope.clear();

                    std::basic_string<CharTypeLPCTSTR> catalog_val;
                    catalog_val.resize(MAX_DSN_VALUE_LEN);
                    auto read = GetDlgItemText(hdlg, IDC_CATALOG, const_cast<CharTypeLPCTSTR *>(catalog_val.data()), catalog_val.size());
                    catalog_val.resize((read <= 0 || read > catalog_val.size()) ? 0 : read);
                    std::string catalog_str = toUTF8(catalog_val);

                    if (lpsetupdlg.source_type == 2 || lpsetupdlg.source_type == 4) {
                         // Database Mode
                         ci.database = catalog_str;

                         std::basic_string<CharTypeLPCTSTR> scope_val;
                         scope_val.resize(MAX_DSN_VALUE_LEN);
                         read = GetDlgItemText(hdlg, IDC_SCOPE, const_cast<CharTypeLPCTSTR *>(scope_val.data()), scope_val.size());
                         scope_val.resize((read <= 0 || read > scope_val.size()) ? 0 : read);
                         ci.scope = toUTF8(scope_val);
                    } else {
                         // Scope Only Mode
                         ci.scope = catalog_str;
                         ci.database.clear();
                    }

                    /* Return to caller */
                }

                case IDCANCEL: {
                    EndDialog(hdlg, cmd);
                    return TRUE;
                }
                case IDCONTINUE: {
                    auto &lpsetupdlg = *(SetupDialogData *)GetWindowLongPtr(hdlg, DWLP_USER);
                    auto &ci = lpsetupdlg.ci;

                    std::basic_string<CharTypeLPCTSTR> value;

#define SET_CHECKBOX_STRING(NAME, ID, TRUE_VALUE, FALSE_VALUE)                    \
    {                                                                             \
        const BOOL isChecked  = IsDlgButtonChecked(hdlg, ID) == BST_CHECKED;      \
        ci.NAME = (isChecked == BST_CHECKED) ? TRUE_VALUE : FALSE_VALUE;          \
    }

                    bool capella_ops = IsDlgButtonChecked(hdlg, IDC_RADIO_CAPELLA_OPS) == BST_CHECKED;
                    bool capella_analytics = IsDlgButtonChecked(hdlg, IDC_RADIO_CAPELLA_ANALYTICS) == BST_CHECKED;
                    bool cb_server = IsDlgButtonChecked(hdlg, IDC_RADIO_CB_SERVER) == BST_CHECKED;
                    bool ent_analytics = IsDlgButtonChecked(hdlg, IDC_RADIO_ENT_ANALYTICS) == BST_CHECKED;

                    int MAKEINTRESOURCE_VALUE = IDD_NONE_DIALOG;

                    SET_CHECKBOX_STRING(collect_logs, IDC_CHECKBOX_3, "yes", "no");

                    if (capella_ops) {
                        MAKEINTRESOURCE_VALUE = IDD_CAPELLA_DIALOG;
                        ci.connect_to_capella = "yes";
                        lpsetupdlg.source_type = 1;
                    } else if (capella_analytics) {
                        MAKEINTRESOURCE_VALUE = IDD_CAPELLA_DIALOG;
                        ci.connect_to_capella = "yes";
                        lpsetupdlg.source_type = 2;
                    } else if (cb_server) {
                        MAKEINTRESOURCE_VALUE = IDD_COUCHBASE_SERVER_DIALOG;
                        ci.connect_to_capella = "no";
                        lpsetupdlg.source_type = 3;
                    } else if (ent_analytics) {
                        MAKEINTRESOURCE_VALUE = IDD_COUCHBASE_SERVER_DIALOG;
                        ci.connect_to_capella = "no";
                        lpsetupdlg.source_type = 4;
                    }

#undef SET_CHECKBOX_STRING

                    auto ret = DialogBoxParam(module_instance, MAKEINTRESOURCE(MAKEINTRESOURCE_VALUE), hdlg, ConfigDlgProc, lpsetupdlg.GetAsLPARAM());
                    if(ret == IDOK){
                        EndDialog(hdlg,IDOK);
                        return setDSNAttributes(hdlg, &lpsetupdlg, NULL);
                    }
                    else if (ret != IDCANCEL) {
                        auto err = GetLastError();
                        LPVOID lpMsgBuf;
                        LPVOID lpDisplayBuf;

                        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            err,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR)&lpMsgBuf,
                            0,
                            NULL);

                        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR));
                        StringCchPrintf(
                            (LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("failed with error %d: %s"), err, lpMsgBuf);
                        MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

                        LocalFree(lpMsgBuf);
                        LocalFree(lpDisplayBuf);
                    }
                }
            }
            break;
        }
    }

    /* Message not processed */
    return FALSE;
}

inline BOOL ConfigDSN_(
    HWND hwnd,
    WORD fRequest,
    LPCTSTR lpszDriver,
    LPCTSTR lpszAttributes
) noexcept {
    BOOL fSuccess = FALSE;

    SetupDialogData setupdlg;
    SetupDialogData * lpsetupdlg = &setupdlg;

    /* Parse attribute string */
    if (lpszAttributes)
        parseAttributes(lpszAttributes, lpsetupdlg);

    /* Save original data source name */
    lpsetupdlg->dsn = lpsetupdlg->ci.dsn;

    /* Remove data source */
    if (ODBC_REMOVE_DSN == fRequest) {
        /* Fail if no data source name was supplied */
        if (lpsetupdlg->ci.dsn.empty())
            fSuccess = FALSE;

        /* Otherwise remove data source from ODBC.INI */
        else {
            std::basic_string<CharTypeLPCTSTR> DSN;
            fromUTF8(lpsetupdlg->ci.dsn, DSN);
            fSuccess = SQLRemoveDSNFromIni(DSN.c_str());
        }
    }
    /* Add or Configure data source */
    else {
        /* Save passed variables for global access (e.g., dialog access) */
        lpsetupdlg->hwnd_parent = hwnd;
        lpsetupdlg->driver_name = toUTF8(lpszDriver);
        lpsetupdlg->is_new_dsn = (ODBC_ADD_DSN == fRequest);
        lpsetupdlg->is_default = (Poco::UTF8::icompare(lpsetupdlg->ci.dsn, INI_DSN_DEFAULT) == 0);

        /*
        * Display the appropriate dialog (if parent window handle
        * supplied)
        */
        if (hwnd) {
            /* Display dialog(s) */
            auto ret = DialogBoxParam(module_instance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, ConfigDlgProc, (LPARAM)lpsetupdlg);
            if(ret == TRUE){
                fSuccess = TRUE;
            }
        }
        else if (!lpsetupdlg->ci.dsn.empty())
            fSuccess = setDSNAttributes(hwnd, lpsetupdlg, NULL);
        else
            fSuccess = TRUE;
    }

    return fSuccess;
}

inline BOOL ConfigDriver_(
    HWND hwnd,
    WORD fRequest,
    LPCTSTR lpszDriver,
    LPCTSTR lpszArgs,
    LPTSTR lpszMsg,
    WORD cbMsgMax,
    WORD * pcbMsgOut
) noexcept {
    MessageBox(hwnd, TEXT("ConfigDriver"), TEXT("Debug"), MB_OK);
    return TRUE;
}

} } // namespace impl

extern "C" {

INT_PTR CALLBACK ConfigDlgProc(
    HWND hdlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
) {
    return impl::ConfigDlgProc_(
        hdlg,
        wMsg,
        wParam,
        lParam
    );
}

BOOL INSTAPI EXPORTED_FUNCTION_MAYBE_W(ConfigDSN)(
    HWND hwnd,
    WORD fRequest,
    LPCTSTR lpszDriver,
    LPCTSTR lpszAttributes
) {
    return impl::ConfigDSN_(
        hwnd,
        fRequest,
        lpszDriver,
        lpszAttributes
    );
}

BOOL INSTAPI EXPORTED_FUNCTION_MAYBE_W(ConfigDriver)(
    HWND hwnd,
    WORD fRequest,
    LPCTSTR lpszDriver,
    LPCTSTR lpszArgs,
    LPTSTR lpszMsg,
    WORD cbMsgMax,
    WORD * pcbMsgOut
) {
    return impl::ConfigDriver_(
        hwnd,
        fRequest,
        lpszDriver,
        lpszArgs,
        lpszMsg,
        cbMsgMax,
        pcbMsgOut
    );
}

} // extern "C"

#endif
