#include "DDZServer.h"
#include "Application.h"
int main()
{
	CApplication theAplication(DDZServerApp::getInstance());
	theAplication.startApp();
	return 0;
}