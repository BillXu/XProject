#ifndef  _IO_THREADMOD__H_
#define  _IO_THREADMOD__H_
class CThreadT
{
public:
	CThreadT();
	virtual  ~CThreadT();
	bool Start();
	void Stop();

	virtual void __run() = 0;

private:
	static  void  __threadfunc( void *p )
	{
		//屏蔽SIGPIPE,避免宕机
		//sigset_t signal_mask;
		//sigemptyset (&signal_mask);
		//sigaddset (&signal_mask, SIGPIPE);
		//int rc = pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
		//if (rc != 0) 
		//{
		//	printf("Err: block sigpipe error\n");
		//}

		((CThreadT *)p)->__run();
	}

};

#endif // _IO_THREADMOD__H_


