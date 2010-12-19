#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, const char* argv[])
{
#ifdef G3D_WIN32
	if (FileSystem::exists("data-files")) {
		chdir("data-files");
	}
#endif

	GApp::Settings settings(argc, argv);
	settings.window.width = 1440;
	settings.window.height = 800;

	return App(settings).run();
}