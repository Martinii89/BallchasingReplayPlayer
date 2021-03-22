#pragma once
#include <string>

#include <map>
#include <memory>
#include <windows.h>

//class SettingsManager
//{
//public:
//	virtual void SaveSetting(std::wstring key, std::wstring setting, std::wstring subKey);
//	virtual std::wstring GetStringSetting(std::wstring key, bool createIfNew);
//	virtual void SaveSetting(std::wstring key, int setting);
//	virtual int GetIntSetting(std::wstring key, bool createIfNew);
//};

class RegisterySettingsManager
{
public:
	const static std::wstring REGISTRY_DIR;
	const static std::wstring REGISTRY_DIR_APPPATH;
	const static std::wstring REGISTRY_DIR_RUN;
private:
	HKEY GetKey(std::wstring subKey, HKEY root = HKEY_CURRENT_USER);
public:

	void SaveSetting(std::wstring key, std::wstring setting, std::wstring subKey = REGISTRY_DIR, HKEY root = HKEY_CURRENT_USER);
	std::wstring GetStringSetting(std::wstring key, std::wstring subKey = REGISTRY_DIR);
	void SaveSetting(std::wstring key, int setting, std::wstring subKey = REGISTRY_DIR);
	int GetIntSetting(std::wstring key, std::wstring subKey = REGISTRY_DIR);
	void DeleteSetting(std::wstring key, std::wstring subKey = REGISTRY_DIR);
};
