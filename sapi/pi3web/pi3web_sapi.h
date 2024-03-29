#ifndef _PI3WEB_SAPI_H_
#define _PI3WEB_SAPI_H_

#ifdef PHP_WIN32
#	include <windows.h>
#	include <httpext.h>
#	ifdef SAPI_EXPORTS
#		define MODULE_API __declspec(dllexport) 
#	else
#		define MODULE_API __declspec(dllimport) 
#	endif
#else
#	define far
#	define MODULE_API

	typedef int BOOL;
	typedef void far *LPVOID;
	typedef LPVOID HCONN;
	typedef unsigned long DWORD;
	typedef DWORD far *LPDWORD;
	typedef char CHAR;
	typedef CHAR *LPSTR;
	typedef unsigned char BYTE;
	typedef BYTE far *LPBYTE;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PHP_MODE_STANDARD  1
#define PHP_MODE_HIGHLIGHT 2
#define PHP_MODE_INDENT    3
#define PHP_MODE_LINT	   4

//
// passed to the procedure on a new request
//
typedef struct _CONTROL_BLOCK {
	void	 *pPIHTTP;
    DWORD     cbSize;                 // size of this struct.
    HCONN     ConnID;                 // Context number not to be modified!
    DWORD     dwHttpStatusCode;       // HTTP Status code
    CHAR      lpszLogData[80];        // null terminated log info

    LPSTR     lpszMethod;             // REQUEST_METHOD
    LPSTR     lpszQueryString;        // QUERY_STRING
    LPSTR     lpszPathInfo;           // PATH_INFO
    LPSTR     lpszPathTranslated;     // PATH_TRANSLATED
	LPSTR     lpszFileName;           // FileName to PHP3 physical file
	LPSTR     lpszUri;		          // The request URI
	LPSTR     lpszReq;		          // The whole HTTP request line
	LPSTR     lpszUser;		          // The authenticated user
	LPSTR     lpszPassword;	          // The authenticated password

    DWORD     cbTotalBytes;           // Total bytes indicated from client
    DWORD     cbAvailable;            // Available number of bytes
    LPBYTE    lpbData;                // pointer to cbAvailable bytes

    LPSTR     lpszContentType;        // Content type of client data
	DWORD     dwBehavior;			  // PHP behavior (standard, highlight, intend

    BOOL (* GetServerVariable) ( HCONN       hConn,
                                 LPSTR       lpszVariableName,
                                 LPVOID      lpvBuffer,
                                 LPDWORD     lpdwSize );

    BOOL (* WriteClient)  ( HCONN      ConnID,
                            LPVOID     Buffer,
                            LPDWORD    lpdwBytes,
                            DWORD      dwReserved );

    BOOL (* ReadClient)  ( HCONN      ConnID,
                           LPVOID     lpvBuffer,
                           LPDWORD    lpdwSize );

    BOOL (* SendHeaderFunction)( HCONN      hConn,
                                 LPDWORD    lpdwSize,
                                 LPDWORD    lpdwDataType );

} CONTROL_BLOCK, *LPCONTROL_BLOCK;

DWORD PHP4_wrapper(LPCONTROL_BLOCK lpCB);
BOOL PHP4_startup();
BOOL PHP4_shutdown();

// the following type declaration is for the server side
typedef DWORD ( * PFN_WRAPPERFUNC )( CONTROL_BLOCK *pCB );



#ifdef __cplusplus
}
#endif

#endif  // end definition _PI3WEB_SAPI_H_
