#include <memory>

class A 
{
public:
	A() { printf("create A object \n") ; }
	~A(){ printf("A object delete \n") ;}
	std::shared_ptr<A> getThisPtr()
	{
		std::shared_ptr<A> t (this) ;
		return t ;
	}
	int a ;
};

class B 
{
public:
	B(): m_ptrA(new A() ){ } 
	std::shared_ptr<A> getAPtr(){ return m_ptrA ;}
	void setAPtr(std::shared_ptr<A> p ){ if ( m_ptrA == p ){ printf("the same set ptr\n") ;}  m_ptrA  = p ;}
	void count(){ printf("ref cnt = %u \n",m_ptrA.use_count() ) ;}
protected:
	std::shared_ptr<A> m_ptrA ;
};

void  testFunc()
{
	B b ;
	b.count();
	{
		auto temp = b.getAPtr() ;

		b.count();
		auto thiptr = temp->getThisPtr();
		b.count();
		if ( temp == thiptr )
		{
			printf("this ptr the same \n");
		}
		else
		{
			printf("this ptr not the same \n") ;
		}
		//temp = thiptr ;
		auto temp1 = b.getAPtr() ;
		b.count();
		temp = temp1 ;
		b.count();
		b.setAPtr( temp );
		b.count();
	}
	printf(" leve scope \n");
	b.count();
	b.setAPtr(nullptr);
	b.count();
	return  ;
}

#include "VerifyApp.h"
#include "Application.h"

int main()
{
	//testFunc();
	CVerifyApp theApp ;
	CApplication theAplication(&theApp);
	theAplication.startApp();
	return 0 ;
}