#include "EthernetScannerSDKDefine.h"

#if !defined(ETHERNETSCANNERHEADER)
#define ETHERNETSCANNERHEADER

#if !defined (_MSC_VER)
#define _stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//DLL-Functions
/*
char *chIP: "192.168.100.1\0"
char *chPort: "32001"
int uiTimeOut: TimeOut in [ms] for the Connection if no data will be transfered during this Time

return Value: 
the Handle for the Scanner: NULL is non valid Value, all other Values are valid

Note:
This Handle is to use for all communications with the Scanner.
This Handle is activate till the "EthernetScanner_Disconnect"-function will be called. 
call the "EthernetScanner_Disconnect"-function to close the Connection to the Scanner and to get the System-Resources free.
*/
extern void* _stdcall EthernetScanner_Connect(char *chIP, char *chPort, int uiTimeOut);

/*
void* vHandle: the Handle for the Scanner: NULL is non valid Value, all other Values are valid
char *chIP: "192.168.100.1\0"
char *chPort: "32001"
int uiTimeOut: TimeOut in [ms] for the Connection if no data will be transfered during this Time

Note:
please see the description of the "EthernetScanner_Connect"-function
*/
extern void _stdcall EthernetScanner_ConnectEx(void* vHandle, char *chIP, char *chPort, int uiTimeOut);

/*
void *pEthernetScanner: Handle of the Scanner

Note:
the Connection to the Scanner will be closed. 
the Handle of the Scanner will be closed and set to NULL.
The Handle of the Scanner is not more valid.
*/
extern void* _stdcall EthernetScanner_Disconnect(void *pEthernetScanner);

/*
void *pEthernetScanner: the Handle of the Scanner
int *uiConnectStatus: returned Value of the current Connection state.

Note:
Non blocking function. during the time out the connection should be established, otherwise no valid connection.
if no valid connection can be established, the "EthernetScanner_Disconnect"-function need to be called.
*/
extern void _stdcall EthernetScanner_GetConnectStatus(void *pEthernetScanner, int *iConnectStatus);

/*
void *pEthernetScanner: the Handle of the Scanner
double *pdoX: Array of doubles. X-Coordinates. already linerized in [mm]
double *pdoZ: Array of doubles. Z-Coordinates. already linerized in [mm]
int *piIntensity: Array of Integers. Corresponded Intensity-Values of the Point. 0..1023
int *piPeakWidth: Array of Integers. Corresponded PeakWidth-Values of the Point. 0..31
int iBuffer: the size of the Buffer()
unsigned int *puiEncoder: Variable for the Encoder-Value
unsigned char *pucUSRIO: Variable for the User I/O
int dwTimeOut: not in used: non blocking function
unsigned char *ucBufferRaw: the Input-Buffer for the RawData(non linearized)
int iBufferRaw: the size of the Input-Buffer: ETHERNETSCANNER_BUFFERSIZEMAX
int *iPicCnt = NULL : Note: the PicCounter is 16 Bit long: 0...65535

return value: 
amount of all points written in the Buffer.

Note:
for details please s. the manual(using of the DLL)
if any buffers dont needed by the application it possible to provide the NULL-Pointers instead of real Buffers
possible values:
pdoX=NULL
pdoZ=NULL
piIntensity=NULL
piPeakWidth=NULL
ucBufferRaw=NULL
*/
extern int _stdcall EthernetScanner_GetXZIExtended(void *pEthernetScanner, double *pdoX, double *pdoZ, int *piIntensity, int *piPeakWidth, int iBuffer, unsigned int *puiEncoder, unsigned char *pucUSRIO, int dwTimeOut, unsigned char *ucBufferRaw, int iBufferRaw, int *iPicCnt);

/*
void *pEthernetScanner: the Handle of the Scanner
char *chInfo: String-Buffer for the general Information's(eds with \0)
int iBuffer: Size of the String Buffer
char *chMode: resesrved for future: "xml"
*/
extern int _stdcall EthernetScanner_GetInfo(void *pEthernetScanner, char *chInfo, int iBuffer, char *chMode);

/*
void *pEthernetScanner: the Handle of the Scanner
return a Dll-FiFo state in percent
*/
extern int _stdcall EthernetScanner_GetDllFiFoState(void *pEthernetScanner);

/*
void *pEthernetScanner: the Handle of the Scanner
the Dll-FiFo will be reset. 
*/
extern int _stdcall EthernetScanner_ResetDllFiFo(void *pEthernetScanner);

/*
void *pEthernetScanner: the Handle of the Scanner
unsigned char *ucBuffer: Buffer with the Commands
int uiBuffer: Size of the Byte-Buffer
*/
extern int _stdcall EthernetScanner_WriteData(void *pEthernetScanner, char *cBuffer, int uiBuffer);

/*
void *pEthernetScanner: the Handle of the Scanner
char * chPropertyName: Buffer with the property name
char * chRetBuf: Buffer where the property value will be written
int iRetBuf: the length of the chRetBuf buffer
int iCashTime: the old of the cach in the DLL 
*/
extern int _stdcall EthernetScanner_ReadData(void* pEthernetScanner, char * chPropertyName, char * chRetBuf, int iRetBuf, int iCashTime);


/*
unsigned char *ucBuffer: Input-Buffer for the Dll-Version as Text, end with \0
int uiBuffer: Size of the Input-Buffer
*/
extern int _stdcall EthernetScanner_GetVersion(unsigned char *ucBuffer, int uiBuffer);

#ifdef __cplusplus
};
#endif  /* __cplusplus */

#endif
