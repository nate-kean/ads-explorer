
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


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


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ADSExplorer_h_h__
#define __ADSExplorer_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef __DECLSPEC_ALIGN_EX
#define __DECLSPEC_ALIGN_EX(x)   __declspec(align(x))
#endif

/* Forward Declarations */ 

#ifndef __IADSExplorerShellFolder_FWD_DEFINED__
#define __IADSExplorerShellFolder_FWD_DEFINED__
typedef interface IADSExplorerShellFolder IADSExplorerShellFolder;

#endif 	/* __IADSExplorerShellFolder_FWD_DEFINED__ */


#ifndef __IADSXContextMenuEntry_FWD_DEFINED__
#define __IADSXContextMenuEntry_FWD_DEFINED__
typedef interface IADSXContextMenuEntry IADSXContextMenuEntry;

#endif 	/* __IADSXContextMenuEntry_FWD_DEFINED__ */


#ifndef __ADSExplorerShellFolder_FWD_DEFINED__
#define __ADSExplorerShellFolder_FWD_DEFINED__

#ifdef __cplusplus
typedef class ADSExplorerShellFolder ADSExplorerShellFolder;
#else
typedef struct ADSExplorerShellFolder ADSExplorerShellFolder;
#endif /* __cplusplus */

#endif 	/* __ADSExplorerShellFolder_FWD_DEFINED__ */


#ifndef __ADSXContextMenuEntry_FWD_DEFINED__
#define __ADSXContextMenuEntry_FWD_DEFINED__

#ifdef __cplusplus
typedef class ADSXContextMenuEntry ADSXContextMenuEntry;
#else
typedef struct ADSXContextMenuEntry ADSXContextMenuEntry;
#endif /* __cplusplus */

#endif 	/* __ADSXContextMenuEntry_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IADSExplorerShellFolder_INTERFACE_DEFINED__
#define __IADSExplorerShellFolder_INTERFACE_DEFINED__

/* interface IADSExplorerShellFolder */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IADSExplorerShellFolder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0281CB6-CF26-4053-9936-96D7EE602E5E")
    IADSExplorerShellFolder : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IADSExplorerShellFolderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADSExplorerShellFolder * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADSExplorerShellFolder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADSExplorerShellFolder * This);
        
        END_INTERFACE
    } IADSExplorerShellFolderVtbl;

    interface IADSExplorerShellFolder
    {
        CONST_VTBL struct IADSExplorerShellFolderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADSExplorerShellFolder_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IADSExplorerShellFolder_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) )

#define IADSExplorerShellFolder_Release(This)	\
    ( (This)->lpVtbl -> Release(This) )


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IADSExplorerShellFolder_INTERFACE_DEFINED__ */


#ifndef __IADSXContextMenuEntry_INTERFACE_DEFINED__
#define __IADSXContextMenuEntry_INTERFACE_DEFINED__

/* interface IADSXContextMenuEntry */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IADSXContextMenuEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E4C5A47C-D60F-4E81-A418-06C382F65D30")
    IADSXContextMenuEntry : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IADSXContextMenuEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADSXContextMenuEntry * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADSXContextMenuEntry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADSXContextMenuEntry * This);
        
        END_INTERFACE
    } IADSXContextMenuEntryVtbl;

    interface IADSXContextMenuEntry
    {
        CONST_VTBL struct IADSXContextMenuEntryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADSXContextMenuEntry_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define IADSXContextMenuEntry_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) )

#define IADSXContextMenuEntry_Release(This)	\
    ( (This)->lpVtbl -> Release(This) )


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IADSXContextMenuEntry_INTERFACE_DEFINED__ */



#ifndef __ADSEXPLORERLib_LIBRARY_DEFINED__
#define __ADSEXPLORERLib_LIBRARY_DEFINED__

/* library ADSEXPLORERLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ADSEXPLORERLib;

#ifndef __ADSExplorerShellFolder_CLASS_DEFINED__
#define __ADSExplorerShellFolder_CLASS_DEFINED__

#ifdef __cplusplus

class DECLSPEC_UUID("ED383D11-6797-4103-85EF-CBDB8DEB50E2")
ADSExplorerShellFolder;
#endif /* __cplusplus */

#endif 	/* __ADSExplorerShellFolder_CLASS_DEFINED__ */

#ifndef __ADSXContextMenuEntry_CLASS_DEFINED__
#define __ADSXContextMenuEntry_CLASS_DEFINED__

#ifdef __cplusplus

class DECLSPEC_UUID("D8AECA1A-7E1D-44C2-ABB0-F0558AB00092")
ADSXContextMenuEntry;
#endif /* __cplusplus */

#endif 	/* __ADSXContextMenuEntry_CLASS_DEFINED__ */

#endif /* __ADSEXPLORERLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif