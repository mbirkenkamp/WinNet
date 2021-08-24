#include "logmanager.h"

#include <mutex>
#include <Net/Import/Kernel32.h>

// Color codes
CONSTEXPR auto BLACK = 0;
CONSTEXPR auto BLUE = 1;
CONSTEXPR auto GREEN = 2;
CONSTEXPR auto CYAN = 3;
CONSTEXPR auto RED = 4;
CONSTEXPR auto MAGENTA = 5;
CONSTEXPR auto BROWN = 6;
CONSTEXPR auto LIGHTGRAY = 7;
CONSTEXPR auto DARKGRAY = 8;
CONSTEXPR auto LIGHTBLUE = 9;
CONSTEXPR auto LIGHTGREEN = 10;
CONSTEXPR auto LIGHTCYAN = 11;
CONSTEXPR auto LIGHTRED = 12;
CONSTEXPR auto LIGHTMAGENTA = 13;
CONSTEXPR auto YELLOW = 14;
CONSTEXPR auto WHITE = 15;

static char __net_output_fname[NET_MAX_PATH];
static bool __net_output_used()
{
	return strcmp(__net_output_fname, CSTRING(""));
}

// global override able callback
static void (*OnLogA)(int state, const char* buffer) = nullptr;
static void (*OnLogW)(int state, const wchar_t* buffer) = nullptr;

static void SetConsoleOutputColor(const int color)
{
#ifndef BUILD_LINUX
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
}

/* MAIN Thread Invoker */
struct [[nodiscard]] __net_logmanager_array_entry_A_t
{
	Net::Console::LogStates state;
	std::string func;
	std::string msg;
	bool save;
};

struct [[nodiscard]] __net_logmanager_array_entry_W_t
{
	Net::Console::LogStates state;
	std::wstring func;
	std::wstring msg;
	bool save;
};

static std::recursive_mutex __net_logmanager_critical;
static std::vector<__net_logmanager_array_entry_A_t> __net_logmanager_array_a_queue_main;
static std::vector<__net_logmanager_array_entry_W_t> __net_logmanager_array_w_queue_main;

static void __net_logmanager_push_queue_a(__net_logmanager_array_entry_A_t data)
{
	std::lock_guard<std::recursive_mutex> guard(__net_logmanager_critical);
	__net_logmanager_array_a_queue_main.emplace_back(data);
}

static void __net_logmanager_push_queue_w(__net_logmanager_array_entry_W_t data)
{
	std::lock_guard<std::recursive_mutex> guard(__net_logmanager_critical);
	__net_logmanager_array_w_queue_main.emplace_back(data);
}

static void __net_logmanager_output_log_a(__net_logmanager_array_entry_A_t entry)
{
	if (entry.msg.empty())
		return;

	char time[TIME_LENGTH];
	Net::Clock::GetTimeA(time);

	char date[DATE_LENGTH];
	Net::Clock::GetDateA(date);

	// display date & time
	auto buffer = ALLOC<char>(TIME_LENGTH + DATE_LENGTH + 5);
	sprintf(buffer, CSTRING("[%s][%s]"), date, time);
	buffer[TIME_LENGTH + DATE_LENGTH + 4] = '\0';
	printf(buffer);
	FREE(buffer);

	// display prefix in it specific color
	const auto prefix = Net::Console::GetLogStatePrefix(entry.state);
	buffer = ALLOC<char>(prefix.size() + 4);
	sprintf(buffer, CSTRING("[%s]"), prefix.data());
	buffer[prefix.size() + 3] = '\0';

	if (!Net::Console::GetPrintFState())
		SetConsoleOutputColor(Net::Console::GetColorFromState(entry.state));

	printf(buffer);

	if (OnLogA)
		(*OnLogA)((int)entry.state, buffer);

	FREE(buffer);

	if (!Net::Console::GetPrintFState())
		SetConsoleOutputColor(WHITE);

	// output functionname
	if (!entry.func.empty())
		printf(CSTRING("[%s]"), entry.func.data());

	printf(CSTRING(" "));
	printf(entry.msg.data());
	printf(CSTRING("\n"));

	// create an entire buffer to save to file
	if (entry.save)
	{
		if (!entry.func.empty())
		{
			const auto prefix = Net::Console::GetLogStatePrefix(entry.state);
			const auto bsize = entry.msg.size() + prefix.size() + 23;
			buffer = ALLOC<char>(bsize + 1);
			sprintf(buffer, CSTRING("[%s][%s][%s] %s\n"), date, time, prefix.data(), entry.msg.data());
			buffer[bsize] = '\0';

			if (OnLogA)
				(*OnLogA)((int)entry.state, buffer);

			auto file = NET_FILEMANAGER(__net_output_fname, NET_FILE_WRITE | NET_FILE_APPAND);
			file.write(buffer);

			FREE(buffer);
		}
		else
		{
			const auto prefix = Net::Console::GetLogStatePrefix(entry.state);
			const auto bsize = entry.msg.size() + prefix.size() + entry.func.size() + 25;
			auto buffer = ALLOC<char>(bsize + 1);
			sprintf(buffer, CSTRING("[%s][%s][%s][%s] %s\n"), date, time, prefix.data(), entry.func.data(), entry.msg.data());
			buffer[bsize] = '\0';

			if (OnLogA)
				(*OnLogA)((int)entry.state, buffer);

			auto file = NET_FILEMANAGER(__net_output_fname, NET_FILE_WRITE | NET_FILE_APPAND);
			file.write(buffer);

			FREE(buffer);
		}
	}
}

static void __net_logmanager_output_log_w(__net_logmanager_array_entry_W_t entry)
{
	if (entry.msg.empty())
		return;

	wchar_t time[TIME_LENGTH];
	Net::Clock::GetTimeW(time);

	wchar_t date[DATE_LENGTH];
	Net::Clock::GetDateW(date);

	// display date & time
	auto buffer = ALLOC<wchar_t>(TIME_LENGTH + DATE_LENGTH + 5);
	swprintf(buffer, TIME_LENGTH + DATE_LENGTH + 4, CWSTRING("[%s][%s]"), date, time);
	buffer[TIME_LENGTH + DATE_LENGTH + 4] = '\0';
	wprintf(buffer);
	FREE(buffer);

	// display prefix in it specific color
	const auto prefix = Net::Console::GetLogStatePrefix(entry.state);
	buffer = ALLOC<wchar_t>(prefix.size() + 4);
	swprintf(buffer, prefix.size() + 3, CWSTRING("[%s]"), prefix.data());
	buffer[prefix.size() + 3] = '\0';

	if (!Net::Console::GetPrintFState())
		SetConsoleOutputColor(Net::Console::GetColorFromState(entry.state));

	wprintf(buffer);

	if (OnLogW)
		(*OnLogW)((int)entry.state, buffer);

	FREE(buffer);

	if (!Net::Console::GetPrintFState())
		SetConsoleOutputColor(WHITE);

	// output functionname
	if (!entry.func.empty())
		wprintf(CWSTRING("[%s]"), entry.func.data());

	wprintf(CWSTRING(" "));
	wprintf(entry.msg.data());
	wprintf(CWSTRING("\n"));

	// create an entire buffer to save to file
	if (entry.save)
	{
		if (!entry.func.empty())
		{
			const auto prefix = Net::Console::GetLogStatePrefix(entry.state);
			const auto bsize = entry.msg.size() + prefix.size() + 23;
			auto buffer = ALLOC<wchar_t>(bsize + 1);
			swprintf(buffer, bsize, CWSTRING("[%s][%s][%s] %s\n"), date, time, prefix.data(), entry.msg.data());
			buffer[bsize] = '\0';

			if (OnLogW)
				(*OnLogW)((int)entry.state, buffer);

			auto file = NET_FILEMANAGER(__net_output_fname, NET_FILE_WRITE | NET_FILE_APPAND);
			file.write(buffer);

			FREE(buffer);
		}
		else
		{
			const auto prefix = Net::Console::GetLogStatePrefix(entry.state);
			const auto bsize = entry.msg.size() + prefix.size() + entry.func.size() + 25;
			auto buffer = ALLOC<wchar_t>(bsize + 1);
			swprintf(buffer, bsize, CWSTRING("[%s][%s][%s][%s] %s\n"), date, time, prefix.data(), entry.func.data(), entry.msg.data());
			buffer[bsize] = '\0';

			if (OnLogW)
				(*OnLogW)((int)entry.state, buffer);

			auto file = NET_FILEMANAGER(__net_output_fname, NET_FILE_WRITE | NET_FILE_APPAND);
			file.write(buffer);

			FREE(buffer);
		}
	}
}

static void __net_logmanager_invoke_main()
{
	while (true)
	{
		std::lock_guard<std::recursive_mutex> guard(__net_logmanager_critical);
		for (std::vector< __net_logmanager_array_entry_A_t>::iterator i = __net_logmanager_array_a_queue_main.begin(); i != __net_logmanager_array_a_queue_main.end();)
		{
			__net_logmanager_output_log_a(*i);
			i = __net_logmanager_array_a_queue_main.erase(i);
		}

		// qeue wide-strings after ascii-strings displayed
		for (std::vector< __net_logmanager_array_entry_W_t>::iterator i = __net_logmanager_array_w_queue_main.begin(); i != __net_logmanager_array_w_queue_main.end();)
		{
			__net_logmanager_output_log_w(*i);
			i = __net_logmanager_array_w_queue_main.erase(i);
		}

#ifdef BUILD_LINUX
		usleep(1);
#else
		if (Net::Kernel32::IsInitialized())
			Net::Kernel32::Sleep(1);
		else
			Sleep(1);
#endif
	}
}

class __net_logmanager_main_invoke_wrapper
{
public:
	__net_logmanager_main_invoke_wrapper()
	{
		std::thread(__net_logmanager_invoke_main).detach();
	}
};

// auto start thread on program start
static auto __net_logmanager_main_invoke_thread_wrapper = __net_logmanager_main_invoke_wrapper();

NET_NAMESPACE_BEGIN(Net)
NET_NAMESPACE_BEGIN(Console)
static bool DisablePrintF = false;

tm TM_GetTime()
{
	auto timeinfo = tm();
#ifdef BUILD_LINUX
	time_t tm = time(nullptr);
	auto p_timeinfo = localtime(&tm);
	timeinfo = *p_timeinfo;
#else
#ifdef _WIN64
	auto t = _time64(nullptr);   // get time now
	_localtime64_s(&timeinfo, &t);
#else
	auto t = _time32(nullptr);   // get time now
	_localtime32_s(&timeinfo, &t);
#endif
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

#ifdef BUILD_LINUX
	va_start(vaArgs, msg);
	const size_t size = std::vsnprintf(nullptr, 0, msg, vaArgs);
	va_end(vaArgs);

	va_start(vaArgs, msg);
	std::vector<char> str(size + 1);
	std::vsnprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#else
	va_start(vaArgs, msg);
	const size_t size = std::vsnprintf(nullptr, 0, msg, vaArgs);
	std::vector<char> str(size + 1);
	std::vsnprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#endif

	__net_logmanager_array_entry_A_t data;
	data.state = state;
	data.func = std::string(func);
	data.msg = std::string(str.data());
	data.save = false;
	__net_logmanager_push_queue_a(data);
}

void Log(const LogStates state, const char* funcA, const wchar_t* msg, ...)
{
	const size_t lenfunc = strlen(funcA) + 1;
	wchar_t* func = new wchar_t[lenfunc];
	mbstowcs(func, funcA, lenfunc);

	va_list vaArgs;

#ifdef BUILD_LINUX
	va_start(vaArgs, msg);
	const size_t size = std::vswprintf(nullptr, 0, msg, vaArgs);
	va_end(vaArgs);

	va_start(vaArgs, msg);
	std::vector<wchar_t> str(size + 1);
	std::vswprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#else
	va_start(vaArgs, msg);
	const size_t size = std::vswprintf(nullptr, 0, msg, vaArgs);
	std::vector<wchar_t> str(size + 1);
	std::vswprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#endif

	__net_logmanager_array_entry_W_t data;
	data.state = state;
	data.func = std::wstring(func);
	data.msg = std::wstring(str.data());
	data.save = false;
	__net_logmanager_push_queue_w(data);

	FREE(func);
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

NET_NAMESPACE_BEGIN(Manager)
NET_NAMESPACE_BEGIN(Log)
void SetOutputName(const char* name)
{
	Net::String tmp(name);
	if (tmp.find(CSTRING("/")) != NET_STRING_NOT_FOUND
		|| tmp.find(CSTRING("//")) != NET_STRING_NOT_FOUND
		|| tmp.find(CSTRING("\\")) != NET_STRING_NOT_FOUND
		|| tmp.find(CSTRING("\\\\")) != NET_STRING_NOT_FOUND)
	{
		if (tmp.find(CSTRING(":")) != NET_STRING_NOT_FOUND)
		{
			strcpy(__net_output_fname, name);
			strcat(__net_output_fname, CSTRING(".log"));
		}
		else
		{
			strcpy(__net_output_fname, Net::Manager::Directory::homeDir().data());
			strcpy(__net_output_fname, name);
			strcat(__net_output_fname, CSTRING(".log"));
			NET_DIRMANAGER::createDir(__net_output_fname);
		}
	}
	else
	{
		strcpy(__net_output_fname, Net::Manager::Directory::homeDir().data());
		strcat(__net_output_fname, name);
		strcat(__net_output_fname, CSTRING(".log"));
	}
}

void SetLogCallbackA(OnLogA_t callback)
{
	OnLogA = callback;
}

void SetLogCallbackW(OnLogW_t callback)
{
	OnLogW = callback;
}

void Log(const Console::LogStates state, const char* func, const char* msg, ...)
{
	va_list vaArgs;

#ifdef BUILD_LINUX
	va_start(vaArgs, msg);
	const size_t size = std::vsnprintf(nullptr, 0, msg, vaArgs);
	va_end(vaArgs);

	va_start(vaArgs, msg);
	std::vector<char> str(size + 1);
	std::vsnprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#else
	va_start(vaArgs, msg);
	const size_t size = std::vsnprintf(nullptr, 0, msg, vaArgs);
	std::vector<char> str(size + 1);
	std::vsnprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#endif

	__net_logmanager_array_entry_A_t data;
	data.state = state;
	data.func = std::string(func);
	data.msg = std::string(str.data());
	data.save = true;
	__net_logmanager_push_queue_a(data);
}

void Log(const Console::LogStates state, const char* funcA, const wchar_t* msg, ...)
{
	const size_t lenfunc = strlen(funcA) + 1;
	wchar_t* func = new wchar_t[lenfunc];
	mbstowcs(func, funcA, lenfunc);

	va_list vaArgs;

#ifdef BUILD_LINUX
	va_start(vaArgs, msg);
	const size_t size = std::vswprintf(nullptr, 0, msg, vaArgs);
	va_end(vaArgs);

	va_start(vaArgs, msg);
	std::vector<wchar_t> str(size + 1);
	std::vswprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#else
	va_start(vaArgs, msg);
	const size_t size = std::vswprintf(nullptr, 0, msg, vaArgs);
	std::vector<wchar_t> str(size + 1);
	std::vswprintf(str.data(), str.size(), msg, vaArgs);
	va_end(vaArgs);
#endif

	__net_logmanager_array_entry_W_t data;
	data.state = state;
	data.func = std::wstring(func);
	data.msg = std::wstring(str.data());
	data.save = true;
	__net_logmanager_push_queue_w(data);

	FREE(func);
}
NET_NAMESPACE_END
NET_NAMESPACE_END
NET_NAMESPACE_END
