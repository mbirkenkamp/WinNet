#include "logmanager.h"

void SetFname(const char* name)
{
	NetString tmp(name);
	if (tmp.find(CSTRING("/")) != NET_STRING_NOT_FOUND
		|| tmp.find(CSTRING("//")) != NET_STRING_NOT_FOUND
		|| tmp.find(CSTRING("\\")) != NET_STRING_NOT_FOUND
		|| tmp.find(CSTRING("\\\\")) != NET_STRING_NOT_FOUND)
	{
		if (tmp.find(CSTRING(":")) != NET_STRING_NOT_FOUND)
		{
			strcpy_s(fname, name);
			strcat_s(fname, CSTRING(".log"));
		}
		else
		{
			strcpy_s(fname, Net::manager::dirmanager::homeDir().data());
			strcpy_s(fname, name);
			strcat_s(fname, CSTRING(".log"));
			NET_DIRMANAGER::createDir(fname);
		}
	}
	else
	{
		strcpy_s(fname, Net::manager::dirmanager::homeDir().data());
		strcat_s(fname, name);
		strcat_s(fname, CSTRING(".log"));
	}
}

char* GetFname()
{
	return fname;
}

void SetAreaInUse(const bool status)
{
	AreaInUse = status;
}

bool IsAreaInUse()
{
	return AreaInUse;
}

NET_NAMESPACE_BEGIN(Net)
NET_NAMESPACE_BEGIN(Console)
static bool DisablePrintF = false;

tm TM_GetTime()
{
	auto timeinfo = tm();
#ifdef _WIN64
	auto t = _time64(nullptr);   // get time now
	_localtime64_s(&timeinfo, &t);
#else
	auto t = _time32(nullptr);   // get time now
	_localtime32_s(&timeinfo, &t);
#endif
	return timeinfo;
}

std::string GetLogStatePrefix(LogStates state)
{
	switch (static_cast<int>(state))
	{
	case (int)LogStates::debug:
		return std::string(CSTRING("DEBUG"));

	case (int)LogStates::warning:
		return std::string(CSTRING("WARNING"));

	case (int)LogStates::error:
		return std::string(CSTRING("ERROR"));

	case (int)LogStates::success:
		return std::string(CSTRING("SUCCESS"));

	case (int)LogStates::peer:
		return std::string(CSTRING("PEER"));

	default:
		return std::string(CSTRING("INFO"));
	}
}

void Log(const LogStates state, const char* func, const char* msg, ...)
{
	va_list vaArgs;
	va_start(vaArgs, msg);
	const size_t size = std::vsnprintf(nullptr, 0, msg, vaArgs);
	std::vector<char> str(size + 1);
	std::vsnprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);

	if (str.empty())
		return;

	char time[TIME_LENGTH];
	CURRENTTIME(time);

	char date[DATE_LENGTH];
	CURRENTDATE(date);

	if (strcmp(func, CSTRING("")) == 0)
	{
		const auto prefix = GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + 23;
		auto buffer = ALLOC<char>(bsize + 1);
		sprintf_s(buffer, bsize, CSTRING("[%s][%s][%s] %s\n"), date, time, prefix.data(), str.data());
		buffer[bsize] = '\0';

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), GetColorFromState(state));

		printf(CSTRING("%s"), buffer);

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		FREE(buffer);
	}
	else
	{
		const auto prefix = GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + strlen(func) + 25;
		auto buffer = ALLOC<char>(bsize + 1);
		sprintf_s(buffer, bsize, CSTRING("[%s][%s][%s][%s] %s\n"), date, time, prefix.data(), func, str.data());
		buffer[bsize] = '\0';

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), GetColorFromState(state));

		printf(CSTRING("%s"), buffer);

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		FREE(buffer);
	}
}

void Log(const LogStates state, const char* func, const wchar_t* msg, ...)
{
	va_list vaArgs;
	va_start(vaArgs, msg);
	const size_t size = std::vswprintf(nullptr, 0, msg, vaArgs);
	std::vector<wchar_t> str(size + 1);
	std::vswprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);

	if (str.empty())
		return;

	char time[TIME_LENGTH];
	CURRENTTIME(time);

	char date[DATE_LENGTH];
	CURRENTDATE(date);

	if (strcmp(func, CSTRING("")) == 0)
	{
		const auto prefix = GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + 23;
		auto buffer = ALLOC<wchar_t>(bsize + 1);
		swprintf_s(buffer, bsize, CWSTRING("[%s][%s][%s] %s\n"), date, time, prefix.data(), str.data());
		buffer[bsize] = '\0';

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), GetColorFromState(state));

		wprintf(CWSTRING("%s"), buffer);

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		FREE(buffer);
	}
	else
	{
		const auto prefix = Console::GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + strlen(func) + 25;
		auto buffer = ALLOC<wchar_t>(bsize + 1);
		swprintf_s(buffer, bsize, CWSTRING("[%s][%s][%s][%s] %s\n"), date, time, prefix.data(), func, str.data());
		buffer[bsize] = '\0';

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), GetColorFromState(state));

		wprintf(CWSTRING("%s"), buffer);

		if (!GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		FREE(buffer);
	}
}

void ChangeStdOutputColor(const int Color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Color);
}

void SetPrintF(const bool state)
{
	DisablePrintF = !state;
}

bool GetPrintFState()
{
	return DisablePrintF;
}

WORD GetColorFromState(const LogStates state)
{
	switch (state)
	{
	case LogStates::normal:
		return WHITE;

	case LogStates::debug:
		return LIGHTBLUE;

	case LogStates::warning:
		return MAGENTA;

	case LogStates::error:
		return LIGHTRED;

	case LogStates::success:
		return LIGHTGREEN;

	case LogStates::peer:
		return YELLOW;

	default:
		return WHITE;
	}
}
NET_NAMESPACE_END

NET_NAMESPACE_BEGIN(manager)
Log::Log()
{
	file = new NET_FILEMANAGER(GetFname(), NET_FILE_WRITE | NET_FILE_APPAND);
}

Log::~Log()
{
	if (file)
	{
		delete file;
		file = nullptr;
	}
}

void Log::doLog(const Console::LogStates state, const char* func, const char* msg, ...) const
{
	va_list vaArgs;
	va_start(vaArgs, msg);
	const size_t size = std::vsnprintf(nullptr, 0, msg, vaArgs);
	std::vector<char> str(size + 1);
	std::vsnprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);

	if (str.empty())
		return;

	char time[TIME_LENGTH];
	Clock::GetTimeA(time);

	char date[DATE_LENGTH];
	Clock::GetDateA(date);

	if (strcmp(func, CSTRING("")) == 0)
	{
		const auto prefix = Console::GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + 23;
		auto buffer = ALLOC<char>(bsize + 1);
		sprintf_s(buffer, bsize, CSTRING("[%s][%s][%s] %s\n"), date, time, prefix.data(), str.data());
		buffer[bsize] = '\0';

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Console::GetColorFromState(state));

		printf(CSTRING("%s"), buffer);

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		if (!WriteToFile(buffer))
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
			printf(CSTRING("%s"), CSTRING("[FILE SYSTEM] Unable to write to file ('%s')\n"), GetFname());
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
		}

		FREE(buffer);
	}
	else
	{
		const auto prefix = Console::GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + strlen(func) + 25;
		auto buffer = ALLOC<char>(bsize + 1);
		sprintf_s(buffer, bsize, CSTRING("[%s][%s][%s][%s] %s\n"), date, time, prefix.data(), func, str.data());
		buffer[bsize] = '\0';

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Console::GetColorFromState(state));

		printf(CSTRING("%s"), buffer);

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		if (!WriteToFile(buffer))
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
			printf(CSTRING("%s"), CSTRING("[FILE SYSTEM] Unable to write to file ('%s')\n"), GetFname());
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
		}

		FREE(buffer);
	}
}

void Log::doLog(const Console::LogStates state, const char* funcA, const wchar_t* msg, ...) const
{
	wchar_t func[2048];
	swprintf(func, sizeof(func), CWSTRING("%s"), funcA);
	
	va_list vaArgs;
	va_start(vaArgs, msg);
	const size_t size = std::vswprintf(nullptr, 0, msg, vaArgs);
	std::vector<wchar_t> str(size + 1);
	std::vswprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);

	if (str.empty())
		return;

	wchar_t time[TIME_LENGTH];
	Clock::GetTimeW(time);

	wchar_t date[DATE_LENGTH];
	Clock::GetDateW(date);

	if (wcscmp(func, CWSTRING("")) == 0)
	{
		const auto prefix = Console::GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + 23;
		auto buffer = ALLOC<wchar_t>(bsize + 1);
		swprintf_s(buffer, bsize, CWSTRING("[%s][%s][%s] %s\n"), date, time, prefix.data(), str.data());
		buffer[bsize] = '\0';

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Console::GetColorFromState(state));

		wprintf(CWSTRING("%s"), buffer);

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		if (!WriteToFile(buffer))
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
			printf(CSTRING("%s"), CSTRING("[FILE SYSTEM] Unable to write to file ('%s')\n"), GetFname());
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
		}

		FREE(buffer);
	}
	else
	{
		const auto prefix = Console::GetLogStatePrefix(state);
		const auto bsize = str.size() + prefix.size() + wcslen(func) + 25;
		auto buffer = ALLOC<wchar_t>(bsize + 1);
		swprintf_s(buffer, bsize, CWSTRING("[%s][%s][%s][%s] %s\n"), date, time, prefix.data(), func, str.data());
		buffer[bsize] = '\0';

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Console::GetColorFromState(state));

		wprintf(CWSTRING("%s"), buffer);

		if (!Console::GetPrintFState())
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);

		if (!WriteToFile(buffer))
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), RED);
			printf(CSTRING("%s"), CSTRING("[FILE SYSTEM] Unable to write to file ('%s')\n"), GetFname());
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), WHITE);
		}

		FREE(buffer);
	}
}

bool Log::WriteToFile(const char* msg) const
{
	size_t attemps = NULL;
	while (!file->write(msg))
	{
		++attemps;
		if (attemps == 1000)
			return false;
	}

	return true;
}

bool Log::WriteToFile(const wchar_t* msg) const
{
	size_t attemps = NULL;
	while(!file->write(msg))
	{
		++attemps;
		if (attemps == 1000)
			return false;
	}

	return true;
}
NET_NAMESPACE_END
NET_NAMESPACE_END