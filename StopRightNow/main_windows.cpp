#include <Windows.h>
#include <iostream>

using namespace std;

int main_windows(void)
{
	cout << "Hoi! If you're seeing this, you're on Windows! As a test, I shall proceed to use some Windows only functions!\n";

	LARGE_INTEGER x;
	QueryPerformanceCounter(&x);

	cout << "QueryPerformanceCounter() says: " << x.QuadPart;

	return 0;
}