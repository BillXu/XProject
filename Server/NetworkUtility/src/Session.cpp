#include "Session.h"
uint32_t CSession::s_ConnectID = 1 ;
CSession::CSession( std::weak_ptr<asio::io_service::strand> ptrStrand )
	:m_socket( ptrStrand.lock()->get_io_service() ),m_pReadIngBuffer(new CInternalBuffer())
{
	m_nConnectID = ++s_ConnectID ;
	m_ptrStrand = ptrStrand;
	m_lpfErrorCallBack = nullptr;
	m_plfReciveCallBack = nullptr;
}

bool CSession::sendData(const char* pData , size_t nLen )
{
	InternalBuffer_ptr pBuffer( new CInternalBuffer() );
	if ( ! pBuffer->setData(pData,nLen) )
	{
		std::cout << "send this data error len = %u" << nLen << std::endl;
		return false ;
	}
	
	auto pStrand = m_ptrStrand.lock();
	auto pSelf(shared_from_this());
	pStrand->get_io_service().post(pStrand->wrap([pBuffer, pSelf]()->void
	{ 
		auto bSending = pSelf->m_vWillSendBuffers.empty() == false;
		pSelf->m_vWillSendBuffers.push_back(pBuffer);

		if ( false == bSending)
		{
			pSelf->doWriteBuffer();
		}
	}));
	return true ;
}

void CSession::doReadHeader()
{
	auto pSelf(shared_from_this());

	auto pfn = m_ptrStrand.lock()->wrap([pSelf](const asio::error_code& error, std::size_t bytes_transferred )->void
		{
			if (!error && pSelf->m_pReadIngBuffer->decodeHeader())
			{
				// go on read body 
				pSelf->doReadBody();
			}
			else
			{
				pSelf->handleError(error);
			}
	});

	asio::async_read(m_socket,
		asio::buffer(m_pReadIngBuffer->data(), CInternalBuffer::header_length), pfn);
}

void CSession::doReadBody()
{
	auto pSelf(shared_from_this());

	asio::async_read(m_socket,
		asio::buffer(m_pReadIngBuffer->body(), m_pReadIngBuffer->bodyLength() ),
		m_ptrStrand.lock()->wrap([pSelf](const asio::error_code& error,std::size_t bytes_transferred)->void
	{
		if ( !error )
		{
			pSelf->m_plfReciveCallBack(pSelf->getConnectID(), pSelf->m_pReadIngBuffer->body(), pSelf->m_pReadIngBuffer->bodyLength());

			// go on read header 
			pSelf->doReadHeader();
		}
		else
		{
			pSelf->handleError(error);
		}
	}
	));
}

void CSession::doWriteBuffer()
{
	auto pSelf(shared_from_this());
	asio::async_write(m_socket,asio::buffer(m_vWillSendBuffers.front()->data(),m_vWillSendBuffers.front()->length()),
		m_ptrStrand.lock()->wrap([pSelf](const asio::error_code& error, std::size_t bytes_transferred )->void
	{
		if (!error)
		{
			if (pSelf->m_vWillSendBuffers.empty() == false)
			{
				pSelf->m_vWillSendBuffers.pop_front();
			}

			// go on 
			if (pSelf->m_vWillSendBuffers.empty() == false)
			{
				pSelf->doWriteBuffer();
			}
		}
		else
		{
			pSelf->handleError(error);
		}
	}));
}

uint32_t CSession::getConnectID()
{
	return m_nConnectID ;
}

void CSession::start()
{
	doReadHeader();
}

void CSession::closeSession()
{
#ifdef _DEBUG
	std::cout << "serssion close close" << std::endl;
#endif // _DEBUG
	if (!m_socket.is_open())
	{
#ifdef _DEBUG
		std::cout << "already closed so can not close again" << std::endl;
#endif // _DEBUG
		return;
	}
	asio::error_code ec;
	m_socket.shutdown(tcp::socket::shutdown_send,ec);
	m_socket.close(ec);
	m_vWillSendBuffers.clear();
}


