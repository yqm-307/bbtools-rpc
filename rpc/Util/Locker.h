#pragma once
#include <pthread.h>
#include <atomic>
#include <thread>
#include <time.h>
#include "logger.h"

namespace yrpc::util::lock
{

template<typename _Lock>
class lock_guard
{
public:
	lock_guard(_Lock& a):_lock(a)
	{
		a.lock();
	}
	
	~lock_guard()
	{
		_lock.unlock();
	}
private:
	_Lock& _lock;
};

class Mutex:noncopyable::noncopyable
{
public:
	Mutex():_mutex((pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)))
	{
		pthread_mutex_init(_mutex,NULL);
	}
	~Mutex(){
		pthread_mutex_destroy(_mutex);
		free(_mutex);	
	}
	void lock()
	{
		if(0>pthread_mutex_lock(_mutex))
		{
			FATAL("Mutex::lock() error!");
			exit(-1);
		}
	}
	void unlock()
	{
		if(0>pthread_mutex_unlock(_mutex))
		{
			FATAL("Mutex::unlock() error!");
			exit(-1);
		}
	}
	pthread_mutex_t& getlock(){return *_mutex;}

private:
	pthread_mutex_t* _mutex;
};

//自旋锁
class Spinlock:noncopyable::noncopyable
{
public:
	Spinlock()
	{	pthread_spin_init(&_spin,0); }
    
	~Spinlock(){pthread_spin_destroy(&_spin);}
    void lock()
	{
		if(0>pthread_spin_lock(&_spin))
		{
			FATAL("Spinlock::lock() error");
			exit(-1);
		}
	}
	
    void unlock()
	{
		if(0>pthread_spin_unlock(&_spin))
		{
			FATAL("Spinlock::unlock() error!");
			exit(-1);
		}		
	}

private:
	pthread_spinlock_t _spin;
};


class Sem_t:noncopyable::noncopyable
{
public:
	Sem_t()
	{
		pthread_cond_init(&_cond_t,NULL);
	}
	~Sem_t()
	{
		pthread_cond_destroy(&_cond_t);
	}

	void wait()
	{
		lock_guard<Mutex> lock(_mutex);
		if( 0 > pthread_cond_wait(&_cond_t,&_mutex.getlock()))
		{
			FATAL("MyLocker::sem_t::wait() error!");
			exit(-1);
		}
	}
	void notify_one()
	{
		
		if(0>pthread_cond_signal(&_cond_t))
		{
			FATAL("sem_t::notify_one() error!");
			exit(-1);
		}
	}

	void notify_all()
	{
		if(0>pthread_cond_broadcast(&_cond_t))
		{
			FATAL("sem_t::notify_all() error!");
			exit(-1);
		}
	}

	void timewait(int time_ms)
	{
		struct timespec endtime;
		clock_gettime(CLOCK_REALTIME,&endtime);
		uint64_t ns = endtime.tv_sec * 1000*1000*1000 + endtime.tv_nsec + time_ms*1000*1000;
		endtime.tv_sec = ns/(1000*1000*1000);
		endtime.tv_nsec = ns%(1000*1000*1000);

		if(0>pthread_cond_timedwait(&_cond_t,&_mutex.getlock(),&endtime))
		{
			FATAL("sem_t::notify_all() error!");
			exit(-1);
		}
	}

private:
	Mutex _mutex;
	pthread_cond_t _cond_t;
};



//
class CountDownLatch:noncopyable::noncopyable
{
public:
	CountDownLatch(int cot):_count(cot)
	{
		_sem = PTHREAD_COND_INITIALIZER;
	}
	~CountDownLatch()
	{
		if(_count!=0)
			DEBUG("~CountDownLatch() _count=%d",_count.load());
	}

	void wait()
	{
		lock_guard<Mutex> lock(_lock);
		if(_count > 0)
			pthread_cond_wait(&_sem,&_lock.getlock());
	}	
	//todo 设置超时的阻塞时间
	//void clockwait(Timestamp);
	void down()
	{
		--_count;
		//INFO("count %d",_count.load());
		if(_count <= 0)
			pthread_cond_broadcast(&_sem);
	}
private:
	std::atomic_int _count;
	Mutex _lock;
	pthread_cond_t _sem;
};



}