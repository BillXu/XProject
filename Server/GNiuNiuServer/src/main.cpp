#include "PokerServer.h"
#include "Application.h"
#include "NiuNiu/NiuNiuPeerCard.h"
int main()
{
	CApplication theAplication(PokerServerApp::getInstance());
	theAplication.startApp();
	return 0;
}