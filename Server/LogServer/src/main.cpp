#include "LogSvrApp.h"
#include "Application.h"
int main()
{
	CLogSvrApp theApp ;
	CApplication theAplication(&theApp);
	theAplication.startApp();
	return 0 ;
}