#include "WindowsAPIs.h"
#include "TaskIcon.h"

INT WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, INT)
{
    hinstance = hinst;

    XInputEnable(TRUE);
    TaskIcon().MessageLoop();

    return 0;
}
