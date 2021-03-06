// TotalSurveillance.cpp: Defines the entry point for the console application.
//

#include "stdafx.h"

#include <ctime>

#include <Windows.h>

#include <TlHelp32.h>
#include <atlbase.h>
#include <UIAutomation.h>

#include <vector>
#include <string>
#include <iostream>
#include <regex>
#include <fstream>


namespace ts {
	std::vector<std::wstring> illegalProcesses = {
	};

	std::vector<std::wstring> illegalURLs = {
	};

	std::string illegalProcessesPath = "illegalProcessesConfig.txt";
	std::string illegalWebsitesPath = "illegalWebsitesConfig.txt";

	bool fileExists(const std::string &path) {
		std::ifstream f(path);
		bool ex = f.good();
		f.close();
		return ex;
	}

	void createConfigIfMissing(std::string &path, std::vector<std::wstring> &data) {
		if (!fileExists(path)) {
			std::wofstream outfile(path);
			for (int i = 0; i < data.size(); i++) {
				outfile << data[i] << std::endl;
			}
			outfile.close();
		}
	}

	void loadConfig(const std::string &path, std::vector<std::wstring> &data) {
		data.clear();
		std::wstring line;
		std::wifstream file(path);
		if (file) {
			while (std::getline(file, line)) {
				if (line != L"") {
					data.push_back(line);
				}
			}
			file.close();
		}
	}

	time_t getFileEditTime(const std::string &path) {
		struct _stat result;
		if (_stat(path.data(), &result) == 0) {
			time_t modTime = result.st_mtime;
			return modTime;
		}
		return 0;
	}

	void loadConfigIfNeeded() {
		static time_t illegalProcessesLastEditTime = 0;
		static time_t illegalURLsLastEditTime = 0;

		time_t illegalProcessesEditTime = getFileEditTime(illegalProcessesPath);
		if (illegalProcessesEditTime != illegalProcessesLastEditTime) {
			loadConfig(illegalProcessesPath, illegalProcesses);
			illegalProcessesLastEditTime = illegalProcessesEditTime;
		}

		time_t illegalURLsEditTime = getFileEditTime(illegalWebsitesPath);
		if (illegalURLsEditTime != illegalURLsLastEditTime) {
			loadConfig(illegalWebsitesPath, illegalURLs);
			illegalURLsLastEditTime = illegalURLsEditTime;
		}
	}

	void createConfigsIfMissing() {
		createConfigIfMissing(illegalProcessesPath, illegalProcesses);
		createConfigIfMissing(illegalWebsitesPath, illegalURLs);
	}

	void killProcessesByName(const wchar_t *processName) {
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(entry);
		BOOL hasEntry = Process32First(snapshot, &entry);
		while (hasEntry) {
			if (wcscmp(entry.szExeFile, processName) == 0) {
				HANDLE process = OpenProcess(PROCESS_TERMINATE, 0, entry.th32ProcessID);
				if (process != NULL) {
					TerminateProcess(process, 1000);
					CloseHandle(process);
				}
			}

			hasEntry = Process32Next(snapshot, &entry);
		}

		CloseHandle(snapshot);
	}

	void killProcesses() {
		for (int i = 0; i < illegalProcesses.size(); i++) {
			killProcessesByName(illegalProcesses[i].data());
		}
	}

	std::wstring getTabName() {
		CComQIPtr<IUIAutomation> uia;
		if (FAILED(uia.CoCreateInstance(CLSID_CUIAutomation)) || !uia) {
			return L"ERROR_IUIAUTOMATION_OBJECT_CREATION_FAILED";
		}

		CComPtr<IUIAutomationCondition> cond;
		uia->CreatePropertyCondition(UIA_ControlTypePropertyId, CComVariant(0xC354), &cond);	//For Opera-Webbrowser: 0xC36E; For Firefox-Webbrowser: 0xC354

		HWND hwnd = NULL;
		while (true) {
			hwnd = FindWindowEx(NULL, hwnd, L"Chrome_WidgetWin_1", NULL);	//For Opera-Webbbrowser: L"Chrome_WidgetWin_1"; For Firefox-Webbrowser: No Access!
			if (!hwnd) {
				return L"ERROR_WINDOW_NOT_FOUND";
			}

			if (!IsWindowVisible(hwnd)) {
				continue;
			}

			CComPtr<IUIAutomationElement> root;
			if (FAILED(uia->ElementFromHandle(hwnd, &root)) || !root) {
				return L"ERROR_IUIAUTOMATIONELEMENT";
			}

			CComPtr<IUIAutomationElement> urlBox;
			if (FAILED(root->FindFirst(TreeScope_Descendants, cond, &urlBox)) || !urlBox) {
				continue;
			}

			CComVariant url;
			urlBox->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &url);
			return url.bstrVal;
		}
	}

	void killURLs() {
		std::wstring tabName = getTabName();
		for (int i = 0; i < illegalURLs.size(); i++) {
			std::wregex regex(illegalURLs[i]);
			if (std::regex_search(tabName, regex)) {
				killProcessesByName(L"opera.exe");
				killProcessesByName(L"chrome.exe");
				killProcessesByName(L"firefox.exe");
				ShowWindow(GetConsoleWindow(), SW_SHOW);
				std::wcout << "Bye bye browsers! Have a nice day!" << std::endl;
				std::wcout << "Found illegal website:	" << tabName << std::endl;
				Sleep(10 * 1000);
				ShowWindow(GetConsoleWindow(), SW_HIDE);
			}
		}
	}

	[[noreturn]] void start() {
		CoInitialize(NULL);
		createConfigsIfMissing();

		HWND hwnd = GetConsoleWindow();
		HMENU hmenu = GetSystemMenu(hwnd, FALSE);
		EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);
		std::cout << "THIS WINDOW WILL ONLY BE AROUND FOR 1 MINUTE SO BE QUICK! " << std::endl << "(Or you can just read the ReadMe.txt in C:\Program Files\TotalSurveillance\ReadMe.txt)" << std::endl << std::endl;
		std::cout << "Welcome to TotalSurveillance!" << std::endl;
		std::cout << "This program will allow you to observe yourself and prevent you from going to websites you shouldn't" << std::endl << "(Your browser will crash, unless it's Mozilla Firefox or Internet Explorer)" << std::endl << "(eg. at work) or not letting you start programs you are not allowed to use" << std::endl;
		std::cout << "To configure the websites you want to block and the programs you dont want to use go " << std::endl << "to C:\Program Files\TotalSurveillance" << std::endl;
		std::cout << "Search for illegalWebsites.txt and illegalProcesses.txt" << std::endl;
		std::cout << "To add a website you'd like to block, open illegalWebsites.txt. On a new line, type the website you want to add." << std::endl;
		std::cout << "If you have multiple websites to add, put each website to a new line" << std::endl;
		std::cout << "To add a program you'd like to block open Task Manager. Click on More Details. Search for" << std::endl << " your program, right-click it and go to details. Read and remember the name of" << std::endl << "the process it shows you (eg. notepad.exe)." << std::endl;
		std::cout << "Add the name of the process on a new line and you are done" << std::endl;
		std::cout << "It's Hot-Config-Reload, which means, you do not have to restart the program for " << std::endl << "changes made in the config files to take effect!" << std::endl;
		Sleep(60 * 1000);
		ShowWindow(hwnd, SW_HIDE);

		while (true) {
			time_t rawTime;
			time(&rawTime);

			tm timeInfo;
			localtime_s(&timeInfo, &rawTime);

			int wday = timeInfo.tm_wday;
			int tmhour = timeInfo.tm_hour;
			int tmSeconds = timeInfo.tm_sec;

			if (wday >= 1 && wday <= 5 && tmhour >= 1 && tmhour < 17) {
				loadConfigIfNeeded();
				killURLs();
				killProcesses();
			}

			if (tmhour >= 1 && tmhour < 6) {
				if (tmSeconds == 0) {
					HWND hwnd = FindWindow(L"Shell_TrayWnd", NULL);
					SendMessage(hwnd, WM_COMMAND, (WPARAM)419, 0);
					Sleep(1000);
					ShowWindow(GetConsoleWindow(), SW_SHOW);
					std::cout << "Go to sleep!" << std::endl;
					Sleep(10 * 1000);
					ShowWindow(GetConsoleWindow(), SW_HIDE);
				}
			}
			Sleep(250);
		}
	}
}

int main()
{
	ts::start();
	return 0;
}

