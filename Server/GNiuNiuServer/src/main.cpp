#include "PokerServer.h"
#include "Application.h"
int main()
{
	CApplication theAplication(PokerServerApp::getInstance());
	theAplication.startApp();
	return 0;
}