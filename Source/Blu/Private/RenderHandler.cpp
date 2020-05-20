#include "RenderHandler.h"
#include "BluEye.h"

RenderHandler::RenderHandler(int32 width, int32 height, UBluEye* ui)
{
	this->Width = width;
	this->Height = height;
	this->ParentUI = ui;
}

void RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	rect = CefRect(0, 0, Width, Height);
}

void RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
{
	FUpdateTextureRegion2D *updateRegions = static_cast<FUpdateTextureRegion2D*>(FMemory::Malloc(sizeof(FUpdateTextureRegion2D) * dirtyRects.size()));

	int current = 0;
	for (auto dirtyRect : dirtyRects)
	{
		updateRegions[current].DestX = updateRegions[current].SrcX = dirtyRect.x;
		updateRegions[current].DestY = updateRegions[current].SrcY = dirtyRect.y;
		updateRegions[current].Height = dirtyRect.height;
		updateRegions[current].Width = dirtyRect.width;

		current++;
	}

	// Trigger our parent UIs Texture to update
	ParentUI->TextureUpdate(buffer, updateRegions, dirtyRects.size());
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
	//CEF_REQUIRE_UI_THREAD();
	if (!BrowserRef.get())
	{
		// Keep a reference to the main browser.
		BrowserRef = browser;
		BrowserId = browser->GetIdentifier();
	}
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
	//CEF_REQUIRE_UI_THREAD();
	if (BrowserId == browser->GetIdentifier())
	{
		BrowserRef = NULL;
	}
}

bool BrowserClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line)
{
	FString LogMessage = FString(message.c_str());
	LogEmitter->Broadcast(LogMessage);
	return true;
}

void BrowserClient::OnFullscreenModeChange(CefRefPtr< CefBrowser > browser, bool fullscreen)
{
	UE_LOG(LogTemp, Log, TEXT("Changed to Fullscreen: %d"), fullscreen);
}

void BrowserClient::OnTitleChange(CefRefPtr< CefBrowser > browser, const CefString& title)
{
	FString TitleMessage = FString(title.c_str());
	LogEmitter->Broadcast(TitleMessage);
}

CefRefPtr<CefBrowser> BrowserClient::GetCEFBrowser()
{
	return BrowserRef;
}

bool BrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
	FString data;
	FString name = FString(UTF8_TO_TCHAR(message->GetArgumentList()->GetString(0).ToString().c_str()));
	FString type = FString(UTF8_TO_TCHAR(message->GetArgumentList()->GetString(2).ToString().c_str()));
	FString data_type = FString(UTF8_TO_TCHAR(message->GetArgumentList()->GetString(3).ToString().c_str()));

	if (type == "js_event")
	{

		// Check the datatype

		if (data_type == "bool")
			data = message->GetArgumentList()->GetBool(1) ? TEXT("true") : TEXT("false");
		else if (data_type == "int")
			data = FString::FromInt(message->GetArgumentList()->GetInt(1));
		else if (data_type == "string")
			data = FString(UTF8_TO_TCHAR(message->GetArgumentList()->GetString(1).ToString().c_str()));
		else if (data_type == "double")
			data = FString::SanitizeFloat(message->GetArgumentList()->GetDouble(1));

		EventEmitter->Broadcast(name, data);
	}

	return true;
}

void BrowserClient::OnUncaughtException(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace)
{
	FString ErrorMessage = FString(exception->GetMessage().c_str());
	UE_LOG(LogClass, Warning, TEXT("%s"), *ErrorMessage);
}

//The path slashes have to be reversed to work with CEF
FString ReversePathSlashes(FString forwardPath)
{
	return forwardPath.Replace(TEXT("/"), TEXT("\\"));
}
FString UtilityBLUIDownloadsFolder()
{
	return ReversePathSlashes(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + "Plugins/BLUI/Downloads/"));
}


void BrowserClient::SetEventEmitter(FScriptEvent* emitter)
{
	this->EventEmitter = emitter;
}

void BrowserClient::SetLogEmitter(FLogEvent* emitter)
{
	this->LogEmitter = emitter;
}

void BrowserClient::OnBeforeDownload(
	CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefDownloadItem> download_item,
	const CefString & suggested_name,
	CefRefPtr<CefBeforeDownloadCallback> callback)
{
	UNREFERENCED_PARAMETER(browser);
	UNREFERENCED_PARAMETER(download_item);

	//We use this concatenation method to mix c_str with regular FString and then convert the result back to c_str
	FString downloadPath = UtilityBLUIDownloadsFolder() + FString(suggested_name.c_str());

	callback->Continue(*downloadPath, false);	//don't show the download dialog, just go for it

	UE_LOG(LogClass, Log, TEXT("Downloading file for path %s"), *downloadPath);
}

void BrowserClient::OnDownloadUpdated(
	CefRefPtr<CefBrowser> ForBrowser,
	CefRefPtr<CefDownloadItem> DownloadItem,
	CefRefPtr<CefDownloadItemCallback> Callback)
{
	int Percentage = DownloadItem->GetPercentComplete();
	FString Url = FString(DownloadItem->GetFullPath().c_str());
	
	UE_LOG(LogClass, Log, TEXT("Download %s Updated: %d"), *Url , Percentage);

	RenderHandlerRef->ParentUI->DownloadUpdated.Broadcast(Url, Percentage);

	if (Percentage == 100 && DownloadItem->IsComplete()) {
		UE_LOG(LogClass, Log, TEXT("Download %s Complete"), *Url);
		RenderHandlerRef->ParentUI->DownloadComplete.Broadcast(Url);
	}

	//Example download cancel/pause etc, we just have to hijack this
	//callback->Cancel();
}

