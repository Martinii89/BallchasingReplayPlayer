#include "pch.h"
#include "SettingsManager.h"

#include <stdio.h>
#include <tchar.h>
#include <iostream>


const std::wstring RegisterySettingsManager::REGISTRY_DIR = std::wstring(L"SOFTWARE\\BakkesMod");
const std::wstring RegisterySettingsManager::REGISTRY_DIR_APPPATH = std::wstring(L"SOFTWARE\\BakkesMod\\AppPath");
const std::wstring RegisterySettingsManager::REGISTRY_DIR_RUN = std::wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");

HKEY RegisterySettingsManager::GetKey(std::wstring subKey, HKEY key)
{
	HKEY hKey;
	LONG currentUserOpen = RegOpenKeyExW(key, subKey.c_str(), 0, KEY_ALL_ACCESS, &hKey);
	if (currentUserOpen != ERROR_SUCCESS) {
		LONG setValueRes = RegCreateKeyW(key, subKey.c_str(), &hKey);// (hKey, key.c_str(), 0, REG_SZ, (LPBYTE)setting.c_str(), (setting.size() + 1) * sizeof(wchar_t));
		std::cout << setValueRes;
	}
	return hKey;
}


void RegisterySettingsManager::SaveSetting(std::wstring key, std::wstring setting, std::wstring subKey, HKEY rootkey)
{
	HKEY hKey = GetKey(subKey, rootkey);

	LONG setValueRes = RegSetValueExW(hKey, key.c_str(), 0, REG_SZ, (LPBYTE)setting.c_str(), (setting.size() + 1) * sizeof(wchar_t));
	if (setValueRes != ERROR_SUCCESS)
		return;
	RegCloseKey(hKey);
}

std::wstring RegisterySettingsManager::GetStringSetting(std::wstring key, std::wstring subKey)
{
	HKEY hKey = GetKey(subKey);

	WCHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExW(hKey, key.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	RegCloseKey(hKey);
	if (ERROR_SUCCESS == nError)
	{
		return szBuffer;
	}
	return std::wstring();
}

void RegisterySettingsManager::SaveSetting(std::wstring key, int setting, std::wstring subKey)
{
	HKEY hKey = GetKey(subKey);


	LONG setValueRes = RegSetValueExW(hKey, key.c_str(), 0, REG_DWORD, (const BYTE*)&setting, sizeof(setting));
	if (setValueRes != ERROR_SUCCESS)
		return;
	RegCloseKey(hKey);
}

int RegisterySettingsManager::GetIntSetting(std::wstring key, std::wstring subKey)
{
	HKEY hKey = GetKey(subKey);

	DWORD dwBufferSize(sizeof(DWORD));
	DWORD nResult(0);
	LONG nError = ::RegQueryValueExW(hKey,
		key.c_str(),
		0,
		NULL,
		reinterpret_cast<LPBYTE>(&nResult),
		&dwBufferSize);
	RegCloseKey(hKey);
	return nResult;
}

void RegisterySettingsManager::DeleteSetting(std::wstring key, std::wstring subKey)
{
	HKEY hKey = GetKey(subKey);
	LONG delValueRes = RegDeleteValueW(hKey, key.c_str());

	if (delValueRes != ERROR_SUCCESS)
		return;
	RegCloseKey(hKey);
}
