#pragma once

#include "stdafx.h"

#include <map>
#include <string>


const std::map<const std::wstring, LPCWSTR> iids{
    {L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}", L"CLSID_MyComputer"},
    {L"{ED383D11-6797-4103-85EF-CBDB8DEB50E2}", L"CLSID_ADSExplorerRootShellFolder"},
    {L"{00000122-0000-0000-C000-000000000046}", L"IDropTarget"},
    {L"{000214EB-0000-0000-C000-000000000046}", L"IExtractIconA"},
    {L"{000214FA-0000-0000-C000-000000000046}", L"IExtractIconW"},
    {L"{000214F9-0000-0000-C000-000000000046}", L"IShellLinkW"},
    {L"{000214E3-0000-0000-C000-000000000046}", L"IShellView"},
    {L"{6FE2B64C-5012-4B88-BB9D-7CE4F45E3751}", L"IConnectionFactory"},
    {L"{64961751-0835-43C0-8FFE-D57686530E64}", L"IExplorerCommandProvider"},
    {L"{176C11B1-4302-4164-8430-D5A9F0EEACDB}", L"IFrameLayoutDefinition"},
    {L"{7E734121-F3B4-45F9-AD43-2FBE39E533E2}", L"IFrameLayoutDefinitionFactory"},
    {L"{7D903FCA-D6F9-4810-8332-946C0177E247}", L"IIdentityName"},
    {L"{93F81976-6A0D-42C3-94DD-AA258A155470}", L"IItemRealizer"},
    {L"{32AE3A1F-D90E-4417-9DD9-23B0DFA4621D}", L"IItemSetOperations"},
    {L"{886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99}", L"IPropertyStore"},
    {L"{BC110B6D-57E8-4148-A9C6-91015AB2F3A5}", L"IPropertyStoreFactory"},
    {L"{3017056D-9A91-4E90-937D-746C72ABBF4F}", L"IPropertyStoreCache"},
    {L"{8279FEB8-5CA4-45C4-BE27-770DCDEA1DEB}", L"ITopViewAwareItem"},
    {L"{A39EE748-6A27-4817-A6F2-13914BEF5890}", L"IUri"},
};
