#include "stdafx.hpp"

#include "FlexEngine.hpp"
#include "Platform/Platform.hpp"
#include "Test.hpp"

// Memory leak checking includes
#if defined(DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif

#if ENABLE_VMEM
#define VMEM_OVERRIDE_NEW_DELETE
#include "VMemSingleFile/VMem.cpp"
#endif

bool g_bShowConsole = true;

int main(int argc, char *argv[])
{
	FLEX_UNUSED(argc);
	FLEX_UNUSED(argv);

#ifdef _WIN32
	// Enable run-time memory leak check for debug builds
#if defined(DEBUG)
	// Notify user if heap is corrupt
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(47947);
#endif
#endif

	flex::Platform::GetConsoleHandle();
	flex::InitializeLogger();

#if RUN_UNIT_TESTS
	flex::FlexTest::Run();

	system("PAUSE");
	return 0;

#else

	{
		flex::FlexEngine* engineInstance = new flex::FlexEngine();
		engineInstance->Initialize();
		engineInstance->UpdateAndRender();
		delete engineInstance;
	}

	if (g_bShowConsole)
	{
		system("PAUSE");
	}

	return 0;
#endif
}

#ifdef _WIN32
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	FLEX_UNUSED(hInstance);
	FLEX_UNUSED(hPrevInstance);
	FLEX_UNUSED(lpCmdLine);
	FLEX_UNUSED(nCmdShow);

	g_bShowConsole = false;

	return main(0, nullptr);
}
#endif
