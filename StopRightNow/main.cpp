// main.cpp : Defines the entry point for the application.
//

#include "StopRightNow.h"

int main_windows(void);
int main_linux(void);

using namespace std;

int main()
{
	SRN_init();
	
#ifdef PLATFORM_WINDOWS
	main_windows();
#elif PLATFORM_LINUX
	main_linux();
#else
	#error "Undefined platform"
#endif

	return 0;
}
