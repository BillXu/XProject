#include "BJServer.h"
#include "Application.h"
int main()
{
	CApplication theAplication(BJServerApp::getInstance());
	theAplication.startApp();
	return 0;
}