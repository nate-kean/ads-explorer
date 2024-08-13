#pragma once
#include "stdafx.h"
#include <map>
#include <string>


    {L"{93F81976-6A0D-42C3-94DD-AA258A155470}", L"IItemRealizer"},
    {L"{000214F9-0000-0000-C000-000000000046}", L"IShellLinkW"},
    {L"{000214FA-0000-0000-c000-000000000046}", L"IExtractIconW"},
const std::map<const std::wstring, LPCWSTR> iids{
    {L"{000214EB-0000-0000-C000-000000000046}", L"IExtractIconA"},
    {L"{64961751-0835-43C0-8FFE-D57686530E64}", L"IExplorerCommandProvider"},
    {L"{886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99}", L"IPropertyStore"},
    {L"{BC110B6D-57E8-4148-A9C6-91015AB2F3A5}", L"IPropertyStoreFactory"},
    {L"{3017056D-9A91-4E90-937D-746C72ABBF4F}", L"IPropertyStoreCache"},
    {L"{8279FEB8-5CA4-45C4-BE27-770DCDEA1DEB}", L"ITopViewAwareItem"},
};
