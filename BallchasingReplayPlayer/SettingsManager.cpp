#include "pch.h"
#include "SettingsManager.h"
#include <windows.h>
#include <tchar.h>
#include <iostream>


HKEY RegisterySettingsManager::GetKey(const std::wstring& sub_key, HKEY key)
{
	HKEY h_key;
	const auto current_user_open = RegOpenKeyExW(key, sub_key.c_str(), 0, KEY_ALL_ACCESS, &h_key);
	if (current_user_open != ERROR_SUCCESS)
	{
		const auto set_value_res = RegCreateKeyW(key, sub_key.c_str(), &h_key);
		std::cout << set_value_res;
	}
	return h_key;
}


void RegisterySettingsManager::SaveSetting(const std::wstring& key, const std::wstring& setting, const std::wstring&
                                           sub_key, HKEY root_key)
{
	auto* const h_key = GetKey(sub_key, root_key);

	const auto set_value_res = RegSetValueExW(h_key, key.c_str(), 0, REG_SZ, (LPBYTE)setting.c_str(),
		(setting.size() + 1) * sizeof(wchar_t));
	if (set_value_res != ERROR_SUCCESS)
		return;
	RegCloseKey(h_key);
}

std::wstring RegisterySettingsManager::GetStringSetting(const std::wstring& key, const std::wstring& sub_key)
{
	auto* const h_key = GetKey(sub_key);

	WCHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	const ULONG n_error = RegQueryValueExW(h_key, key.c_str(), nullptr, nullptr, (LPBYTE)szBuffer, &dwBufferSize);
	RegCloseKey(h_key);
	if (ERROR_SUCCESS == n_error)
	{
		return szBuffer;
	}
	return std::wstring();
}

void RegisterySettingsManager::SaveSetting(const std::wstring& key, int setting, const std::wstring& sub_key)
{
	auto* const h_key = GetKey(sub_key);

	const auto set_value_res = RegSetValueExW(h_key, key.c_str(), 0, REG_DWORD, (const BYTE*)&setting, sizeof(setting));
	if (set_value_res != ERROR_SUCCESS)
		return;
	RegCloseKey(h_key);
}

int RegisterySettingsManager::GetIntSetting(const std::wstring& key, const std::wstring& sub_key)
{
	auto* const h_key = GetKey(sub_key);

	DWORD dwBufferSize(sizeof(DWORD));
	DWORD n_result(0);
	auto nError = ::RegQueryValueExW(h_key,
		key.c_str(),
		nullptr,
		nullptr,
		reinterpret_cast<LPBYTE>(&n_result),
		&dwBufferSize);
	RegCloseKey(h_key);
	return n_result;
}

void RegisterySettingsManager::DeleteSetting(const std::wstring& key, const std::wstring& subKey)
{
	auto* const h_key = GetKey(subKey);
	const auto del_value_res = RegDeleteValueW(h_key, key.c_str());

	if (del_value_res != ERROR_SUCCESS)
		return;
	RegCloseKey(h_key);
}
