/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 21:14:07 2038
 */
/* Compiler settings for ADSExplorer.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C __declspec(selectany) const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IADSExplorerShellFolder,0xF0281CB6,0xCF26,0x4053,0x99,0x36,0x96,0xD7,0xEE,0x60,0x2E,0x5E);


MIDL_DEFINE_GUID(IID, IID_IADSXContextMenuEntry,0xE4C5A47C,0xD60F,0x4E81,0xA4,0x18,0x06,0xC3,0x82,0xF6,0x5D,0x30);


MIDL_DEFINE_GUID(IID, LIBID_ADSEXPLORERLib,0x72B8106C,0x7E36,0x4E68,0xB1,0x25,0x53,0xC5,0x35,0xBC,0x4A,0x9F);


MIDL_DEFINE_GUID(CLSID, CLSID_ADSExplorerShellFolder,0xED383D11,0x6797,0x4103,0x85,0xEF,0xCB,0xDB,0x8D,0xEB,0x50,0xE2);


MIDL_DEFINE_GUID(CLSID, CLSID_ADSXContextMenuEntry,0xD8AECA1A,0x7E1D,0x44C2,0xAB,0xB0,0xF0,0x55,0x8A,0xB0,0x00,0x92);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif