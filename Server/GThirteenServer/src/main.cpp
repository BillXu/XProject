#include "PokerServer.h"
#include "Application.h"

void Test()
{
	
}

int main()
{
	//Test();
	CApplication theAplication(PokerServerApp::getInstance());
	theAplication.startApp();
	return 0;
}