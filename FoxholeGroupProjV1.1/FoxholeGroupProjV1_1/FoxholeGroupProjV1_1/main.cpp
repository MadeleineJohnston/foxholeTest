//this is absolute minimum needed

#include "SDLManager.h"
#include "GLEWManager.h"
#include "Initialise.h"

// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {

	SDLManager manager;

	manager.SDLInit();
	GLEWManager::GLEWInitialise();
	Initialise::init();
	manager.SDLRun();
	manager.SDLEnd();

	return 0;
}