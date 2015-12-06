#include "WindowsAPIs.h"
#include "TaskIcon.h"

INT WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, INT)
{
    hinstance = hinst;

    TaskIcon().MessageLoop();

    return 0;
}
