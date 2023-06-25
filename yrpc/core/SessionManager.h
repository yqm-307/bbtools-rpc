#pragma once
#include "ConnQueue.hpp"
#include "Define.h"
#include "bbt/uuid/Uuid.hpp"
namespace yrpc::rpc::detail
{

/**
 * @brief session 管理器，客户端最重要的管理模块
 * 1、实现了 TCP 连接复用
 */
class __YRPC_SessionManager : bbt::noncopyable
{
public:
    typedef std::function<void(SessionPtr)>     OnSession;  
private:
    typedef bbt::uuid::UuidBase::Ptr UuidPtr;
    typedef std::unordered_map<UuidPtr,SessionPtr>   SessionMap;
    template<class T>
    using lock_guard = yrpc::util::lock::lock_guard<T>;
public:
    static __YRPC_SessionManager* GetInstance();
    /**
     * @brief 发起一个异步连接
     * 
     * @param peer      对端地址
     * @param onsession 连接建立成功时触发回调
     * @return int      # 0(连接中), 1(连接已完成)
     */
    int AsyncConnect(Address peer,OnSession onsession);
    void AsyncAccept(const Address& peer);
    /* 尝试获取Session，Session不存在或者正在连接中返回nullptr，线程安全 */
    SessionPtr TryGetSession(const Address& peer);
    /* 当前地址是否正在连接中 */
    bool IsConnecting(const Address& peer);
private:
    __YRPC_SessionManager(int Nthread);
    ~__YRPC_SessionManager();

    ////////////////////////////////////////////////////////////////////////
    //////// 普通回调操作
    ////////////////////////////////////////////////////////////////////////
    // 运行在 main loop 中的，只做新连接的分发
    void RunInMainLoop();
    // 运行在 sub loop 中的，只做io、协议解析
    void RunInSubLoop(Epoller*);
    /* 成功接收到连接 */
    void OnAccept(const errorcode &e, ConnectionPtr conn);
    /* 连接对端成功 */
    void OnConnect(const errorcode &e, const Address& addr, ConnectionPtr conn);
    /* 负载均衡策略 */
    Epoller* LoadBalancer();
    /* 子线程 */
    void SubLoop(int idx);
    /* 主线程 */
    void MainLoop();
    /* RpcSession 析构时调用 */
    void OnSessionClose(const yrpc::detail::shared::errorcode& e, SessionPtr addr);
    /* RpcSession 超时时调用 */
    void OnSessionTimeOut(const yrpc::detail::shared::errorcode& e, SessionPtr addr);

private:
    ////////////////////////////////////////////////////////////////////////
    //////// m_session_map 操作
    ////////////////////////////////////////////////////////////////////////
    // thread safe: 添加一个新的Session 到 SessionMap 中
    SessionPtr AddNewSession(Channel::ConnPtr newconn);
    // 此操作线程安全: 删除并释放 SessionMap 中一个Session 的资源。如果不存在，则返回false，否则返回true
    bool DelSession(UuidPtr peer_uuid);
    void Dispatch(Buffer&&string, SessionPtr sess);
    SessionID AddressToID(const Address&key);

    ////////////////////////////////////////////////////////////////////////
    //////// m_undone_conn_queue 操作
    ////////////////////////////////////////////////////////////////////////
    /* thread safe: 添加一个新的Session 到 undone queue 中 */
    SessionPtr AddUnDoneSession(Channel::ConnPtr newconn);
    

    
    ////////////////////////////////////////////////////////////////////////
    //////// 握手
    ////////////////////////////////////////////////////////////////////////
private:
    /* 接受握手请求，并响应 */
    MessagePtr Handler_HandShake(MessagePtr, const SessionPtr sess);

    /* 发送握手请求 */
    void StartHandShake(const yrpc::detail::shared::errorcode& e, SessionPtr sess);
    /* 握手成功回调 */
    void OnHandShakeSucc(const yrpc::detail::shared::errorcode& e, SessionPtr sess);
    /* 握手超时 */
    void OnHandShakeTimeOut(const yrpc::detail::shared::errorcode& e, SessionPtr sess);
private:
    Epoller*            m_main_loop;        // 只负责 listen 的 epoll
    Acceptor*           m_main_acceptor; 
    Connector*          m_connector;        
    const size_t        m_sub_loop_size;    // sub eventloop 数量
    std::vector<Epoller*>           m_sub_loop;         // sub eventloop
    CountDownLatch      m_loop_latch;       // 
    std::atomic_int     m_balance;          // 新连接轮转负载，跨线程（需要atomic，还需要考虑 memory order）
    
    std::thread*        m_main_thread;      
    std::vector<std::thread*>       m_sub_threads;   

    /////////////////////////////////
    SessionMap                  m_session_map;      // 全连接 map <uuid, session>
    ConnQueue::Ptr      m_undone_conn_queue;        // 半连接 queue 
    Mutex               m_mutex_session_map;
    /////////////////////////////////

    std::shared_ptr<bbt::uuid::UuidBase>    m_node_id;

    std::vector<bbt::uuid::UuidBase> m_node_id_list;
};
}