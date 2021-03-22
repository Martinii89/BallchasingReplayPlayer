#pragma once
#include <string>

class RegisterySettingsManager
{
private:
	static HKEY GetKey(const std::wstring& sub_key, HKEY key = HKEY_CURRENT_USER);
public:

	static void SaveSetting(const std::wstring& key, const std::wstring& setting, const std::wstring& sub_key, HKEY root = HKEY_CURRENT_USER);
	static std::wstring GetStringSetting(const std::wstring& key, const std::wstring& sub_key);
	static void SaveSetting(const std::wstring& key, int setting, const std::wstring& sub_key);
	static int GetIntSetting(const std::wstring& key, const std::wstring& sub_key);
	static void DeleteSetting(const std::wstring& key, const std::wstring& subKey);
};
