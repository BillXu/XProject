#include "MJServer.h"
#include "Application.h"
int main()
{
	//tempTest();
	CApplication theAplication(CMJServerApp::getInstance());
	theAplication.startApp();
	return 0;
}