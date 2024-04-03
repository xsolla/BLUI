#include "BluManager.h"
#include "include/cef_version.h"

BluManager::BluManager()
{
	// set user agent
	auto chrome_version_major = cef_version_info(4);
	auto chrome_version_minor = cef_version_info(5);
	auto chrome_version_build = cef_version_info(6);
	auto chrome_version_patch = cef_version_info(7);


	FString ChromeVersion = FString::Printf(TEXT("Chrome/%d.%d.%d.%d"), chrome_version_major, chrome_version_minor, chrome_version_build, chrome_version_patch);
	auto ProductVersion = FString::Printf(TEXT("%s/%s UnrealEngine/%s Mozilla/5.0 (Linux; Android 10; Redmi Note 7) AppleWebKit/537.36 (KHTML, like Gecko) %s Safari/537.36"), FApp::GetProjectName(), FApp::GetBuildVersion(), *FEngineVersion::Current().ToString(), *ChromeVersion);

	cef_string_t user_agent = { 0 };
	CefString(&user_agent).FromString(TCHAR_TO_UTF8(*ProductVersion));
	Settings.user_agent = user_agent;
}

void BluManager::OnBeforeCommandLineProcessing(const CefString& process_type,
	CefRefPtr< CefCommandLine > CommandLine)
{

	/////////////////
	/**
	* Used to pick command line switches
	* If set to "true": CEF will use less CPU, but rendering performance will be lower. CSS3 and WebGL are not be usable
	* If set to "false": CEF will use more CPU, but rendering will be better, CSS3 and WebGL will also be usable
	*/
	BluManager::CPURenderSettings = true;
	/////////////////

	CommandLine->AppendSwitch("off-screen-rendering-enabled");
	CommandLine->AppendSwitchWithValue("off-screen-frame-rate", "60");
	CommandLine->AppendSwitch("enable-font-antialiasing");
	CommandLine->AppendSwitch("enable-media-stream");

	// Should we use the render settings that use less CPU?
	if (CPURenderSettings)
	{
		CommandLine->AppendSwitch("disable-software-rasterizer");
		CommandLine->AppendSwitch("disable-gpu");
		CommandLine->AppendSwitch("disable-gpu-compositing");
		CommandLine->AppendSwitch("enable-begin-frame-scheduling");
	}
	else
	{
		// Enables things like CSS3 and WebGL
		CommandLine->AppendSwitch("enable-gpu-rasterization");
		CommandLine->AppendSwitch("enable-webgl");
		CommandLine->AppendSwitch("disable-web-security");
	}

	CommandLine->AppendSwitchWithValue("enable-blink-features", "HTMLImports");

	if (AutoPlay)
	{
		CommandLine->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
	}
	
	// Append more command line options here if you want
	// Visit Peter Beverloo's site: http://peter.sh/experiments/chromium-command-line-switches/ for more info on the switches

}

void BluManager::DoBluMessageLoop()
{
	CefDoMessageLoopWork();
}

CefSettings BluManager::Settings;
CefMainArgs BluManager::MainArgs;
bool BluManager::CPURenderSettings = false;
bool BluManager::AutoPlay = true;