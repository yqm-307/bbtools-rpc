#include "Channel.h"

namespace yrpc::rpc::detail
{

class ChannelMgr: public bbt::templateutil::BaseType<ChannelMgr>
{
    typedef yrpc::detail::shared::errorcode         errorcode;
    typedef yrpc::detail::net::Connection           Connection;
    typedef yrpc::detail::net::YAddress             Address;
    typedef yrpc::coroutine::poller::Epoller        Epoller;
    typedef yrpc::detail::net::Acceptor::LoadBalancer   LoadBalancer;
    typedef std::function<void(const errorcode&, Channel::SPtr, const yrpc::detail::net::YAddress&)> OnConnectCallback;
    typedef std::function<void(const errorcode&, Channel::SPtr)> OnAcceptCallback;
    typedef std::function<void(const errorcode&, Channel::SPtr)> OnCloseCallback; 
public:
    ChannelMgr(Epoller* main_loop);
    ~ChannelMgr();
    /* 注意初始化时机 */
    void SetOnConnect(const OnConnectCallback& cb)
    { m_onconnect = cb; }
    /* 注意初始化时机 */
    void SetOnAccept(const OnAcceptCallback& cb)
    { m_onaccept = cb; }
    /* 注意初始化时机 */
    void SetOnClose(const OnCloseCallback& cb)
    { m_onclose = cb; }
    /* 注意初始化 */
    void SetLoadBalancer(const LoadBalancer& cb)
    { m_loadblancer = cb; }
    void AsyncConnect(const yrpc::detail::net::YAddress& peer_addr);
    /* 需要先设置acceptor */
    void StartListen(const Address&);
    static SPtr Create(Epoller* main_loop);
private:
    void OnConnect(const errorcode& err, Connection::SPtr conn, const yrpc::detail::net::YAddress& addr);
    void OnAccept(const errorcode& err, Connection::SPtr conn);

    void DefaultOnConnect(const errorcode& err, Channel::SPtr conn);
    void DefaultOnAccept(const errorcode& err, Connection::SPtr conn);
    void DefaultOnClose(const errorcode& err, Connection::SPtr conn);

    void InitAChannel(Channel::SPtr chan);
    void InitConnector();
    void InitAcceptor(const Address& listen_addr);
private:
    Epoller*                            m_main_loop;
    yrpc::detail::net::Acceptor::SPtr   m_acceptor{nullptr};
    yrpc::detail::net::Connector::SPtr  m_connector{nullptr};
    OnConnectCallback   m_onconnect{nullptr};
    OnAcceptCallback    m_onaccept{nullptr};
    OnCloseCallback     m_onclose{nullptr};
    LoadBalancer        m_loadblancer{nullptr};
};



}