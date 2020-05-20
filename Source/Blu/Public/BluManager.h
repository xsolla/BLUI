#pragma once

#include "CEFInclude.h"

class BLU_API BluManager : public CefApp
{
public:

	BluManager();

	static void doBluMessageLoop();
	static CefSettings settings;
	static CefMainArgs main_args;
	static bool CPURenderSettings;
	static bool AutoPlay;

	virtual void OnBeforeCommandLineProcessing(const CefString& process_type,
			CefRefPtr< CefCommandLine > command_line) override;

	IMPLEMENT_REFCOUNTING(BluManager);
};

