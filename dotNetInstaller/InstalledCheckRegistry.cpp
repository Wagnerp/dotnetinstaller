#include "StdAfx.h"
#include "InstalledCheckRegistry.h"

InstalledCheckRegistry::InstalledCheckRegistry(void)
{

}

HKEY InstalledCheckRegistry::GetRootKey() const
{
    if(rootkey == "HKEY_CLASSES_ROOT")   return HKEY_CLASSES_ROOT;
    if(rootkey == "HKEY_CURRENT_USER")   return HKEY_CURRENT_USER;
    if(rootkey == "HKEY_USERS")          return HKEY_USERS;
    if(rootkey == "HKEY_CURRENT_CONFIG") return HKEY_CURRENT_CONFIG;
    return HKEY_LOCAL_MACHINE;
}

void InstalledCheckRegistry::Load(TiXmlElement * l_Node, InstallerSetting& p_Setting)
{
    ApplicationLog.Write(TEXT("----Reading REGISTRY installed check: "), l_Node->AttributeT("path").data());
    fieldname = l_Node->AttributeT("fieldname").data();
    fieldtype = l_Node->AttributeT("fieldtype").data();
    fieldvalue = l_Node->AttributeT("fieldvalue").data();
    path = l_Node->AttributeT("path").data();
    comparison = l_Node->AttributeT("comparison").data();
    rootkey = l_Node->AttributeT("rootkey").data();
    wowoption = l_Node->AttributeT("wowoption").data();
}

bool InstalledCheckRegistry::IsInstalled()
{
	try
	{
		//http://msdn.microsoft.com/en-us/library/aa384129(VS.85).aspx
		
		CString keypath(path);
		keypath.Append(L"\\");
		keypath.Append(fieldname);

        ApplicationLog.Write( TEXT("Reading Registry: "), keypath);

		HKEY l_HKey;
		LONG l_result = 0;
		
		OperatingSystem type = GetOsVersion();
		DWORD dwKeyOption = KEY_READ;

		//Alternate registry view is available from Windows XP onwards for 64 bit systems.
		if (type >= winXP)
		{
			//Indicates that an application on 64-bit Windows should operate on the 64-bit registry view.
			if ("WOW64_64" == (wowoption.MakeUpper()))
			{	
				ApplicationLog.Write( TEXT("Opening 64-bit registry view (KEY_WOW64_64KEY)"));
				dwKeyOption |= KEY_WOW64_64KEY;
			}
			//Indicates that an application on 64-bit Windows should operate on the 32-bit registry view.
			else if ("WOW64_32" == (wowoption.MakeUpper()))
			{
				ApplicationLog.Write( TEXT("Opening 32-bit registry view (KEY_WOW64_32KEY)"));
				dwKeyOption |= KEY_WOW64_32KEY;
			}
		}

		l_result = RegOpenKeyEx(GetRootKey(), path, 0, dwKeyOption, &l_HKey);

		if (l_result != ERROR_SUCCESS)
		{
			ApplicationLog.Write( TEXT("***No Registry Entry Found: "), keypath);
			return false;
		}

		ApplicationLog.Write( TEXT("Registry Entry Found: "), keypath);
		if (fieldtype == TEXT("REG_DWORD"))
		{
			DWORD wordValue;
			DWORD l_dwordLen = sizeof(DWORD);
			DWORD l_type = REG_DWORD;
			l_result = RegQueryValueEx(l_HKey,fieldname, NULL, &l_type, (LPBYTE)&wordValue, &l_dwordLen);
			if (l_result != ERROR_SUCCESS)
			{
				RegCloseKey(l_HKey);
				return false;
			}

			RegCloseKey(l_HKey);

			DWORD l_checkValue;
			if (_stscanf(fieldvalue, TEXT("%d"), &l_checkValue) != 1)
			{
				DniSilentMessageBox(TEXT("Invalid registry value to check expected DWORD."), MB_OK|MB_ICONSTOP);
				return false;
			}

            ApplicationLog.Write( TEXT("Value: "), DVLib::FormatNumber(wordValue));

			if (comparison == TEXT("match"))
			{
				if (l_checkValue == wordValue)
					return true;
				else
					return false;
			}
			else if (comparison == TEXT("version"))
			{
				if (l_checkValue <= wordValue)
					return true;
				else
					return false;
			}
			else if (comparison == TEXT("exists"))
			{
				return true;  
			}
			else
			{
				DniSilentMessageBox(TEXT("Invalid comparison type; expected match, version, or exists."), MB_OK|MB_ICONSTOP);
				return false;
			}
		}
		else if (fieldtype == TEXT("REG_SZ"))
		{
			DWORD l_dwordLen = 0;//number of bytes
			DWORD l_type = REG_SZ;
			l_result = RegQueryValueEx(l_HKey,fieldname, NULL, &l_type, NULL, &l_dwordLen);
			if (l_result != ERROR_SUCCESS)
			{
				RegCloseKey(l_HKey);
				return false;
			}

			size_t numberOfChars = l_dwordLen / sizeof(TCHAR);
			TCHAR * charsRegValue = new TCHAR[ numberOfChars + 1 ];
			ZeroMemory(charsRegValue, numberOfChars+1);

			l_result = RegQueryValueEx(l_HKey,fieldname, NULL, &l_type, (LPBYTE)charsRegValue, &l_dwordLen);
			if (l_result != ERROR_SUCCESS)
			{
				RegCloseKey(l_HKey);
				return false;
			}

			RegCloseKey(l_HKey);

			CString registryValue = charsRegValue;
			delete [] charsRegValue;

            ApplicationLog.Write( TEXT("Value: "), registryValue);

			if (comparison == TEXT("match"))
			{
				if (fieldvalue == registryValue)
					return true;
				else
					return false;
			}
			else if (comparison == TEXT("version"))
			{
				if ( DVLib::stringVersionCompare(fieldvalue, registryValue) <= 0 )
					return true;
				else
					return false;
			}
			else if (comparison == TEXT("exists"))
			{
				return true;
			}
			else if (comparison == TEXT("contains"))
			{
				if (registryValue.Find(fieldvalue) >= 0)
					return true;
				else
					return false;
			}
			else
			{
				DniSilentMessageBox(TEXT("Invalid comparison type; expected match, version, exists, or contains."), MB_OK|MB_ICONSTOP);
				return false;
			}
		}
		else if (fieldtype == TEXT("REG_MULTI_SZ"))
		{
			DWORD l_dwordLen = 0;//number of bytes
			DWORD l_type = REG_MULTI_SZ;
			l_result = RegQueryValueEx(l_HKey,fieldname, NULL, &l_type, NULL, &l_dwordLen);
			if (l_result != ERROR_SUCCESS)
			{
				RegCloseKey(l_HKey);
				return false;
			}


			size_t numberOfChars = l_dwordLen / sizeof(TCHAR);
			TCHAR * charsRegValue = new TCHAR[ numberOfChars + 1 ];
			ZeroMemory(charsRegValue, numberOfChars+1);

			l_result = RegQueryValueEx(l_HKey,fieldname, NULL, &l_type, (LPBYTE)charsRegValue, &l_dwordLen);
			if (l_result != ERROR_SUCCESS)
			{
				RegCloseKey(l_HKey);
				return false;
			}

			RegCloseKey(l_HKey);
			// Check for null values and replace them with spaces.  Because the return value from the 
			// registry can be a series of null terminated strings, it's easiest to replace the null values 
			// at the end of each string with a space in order to make it easier to deal with.
			// There's probably a better way to do this, but this gets the job done.
			for (int ii = 0; ii < (int)numberOfChars; ii++)
			{
				if (charsRegValue[ii] == 0)
				{
					charsRegValue[ii] = ' ';
				}
			}
			CString registryValue = charsRegValue;
			delete [] charsRegValue;

			if (comparison == TEXT("match"))
			{
				if (fieldvalue == registryValue)
					return true;
				else
					return false;
			}
			else if (comparison == TEXT("version"))
			{
				if ( DVLib::stringVersionCompare(fieldvalue, registryValue) <= 0 )
					return true;
				else
					return false;
			}
			else if (comparison == TEXT("exists"))
			{
				return true;
			}
			else if (comparison == TEXT("contains"))
			{
				if (registryValue.Find(fieldvalue) >= 0)
					return true;
				else
					return false;
			}

			else
			{
				DniSilentMessageBox(TEXT("Invalid comparison type; expected match, version, exists, or contains."), MB_OK|MB_ICONSTOP);
				return false;
			}
		}
		else
		{
			RegCloseKey(l_HKey);

			DniSilentMessageBox(TEXT("Invalid registry type"), MB_OK|MB_ICONSTOP);
			return false;
		}
	}
	catch(...)
	{
		return false;
	}
}

