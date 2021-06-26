#include "gtest/gtest.h"
#include "../source/coroutine/Alarm.h"
#include "../source/coroutine/Coroutine.h"
#include "../source/threading/Mutex.h"
//#include "../source/log/Logging.h"
#include "../source/log/server/ServerSink.h"

#define var const auto
namespace Jde::Coroutine
{
	struct CoroutineTests : public ::testing::Test
	{
	protected:
		CoroutineTests() {}
		~CoroutineTests() override{}

		void SetUp() override {}
		void TearDown() override {}
	public:
		Coroutine::Task<string> CallPool();
		Coroutine::Task<string> CallThrow();
	};

/*	struct ThrowAwaitable final : CancelAwaitable<Task<string>>
	{
		typedef CancelAwaitable<Task<string>> base;
		ThrowAwaitable()noexcept:base(_hClient){};
		~ThrowAwaitable()=default;
		bool await_ready()noexcept{return false;};
		void await_suspend( ThrowAwaitable::Handle h )noexcept override
		{
			base::await_suspend( h );
			//optional<CoroutineParam> param{ CoroutineParam{h} };
			jthread( [h, this]()mutable
			{
				Threading::SetThreadDscrptn( "await_suspend" );
				std::this_thread::yield();
				_pPromise->get_return_object().ExceptionPtr = std::make_exception_ptr( Exception{"Error"} );
				h.resume();
			} ).detach();
		}
		string await_resume()noexcept(false)
		{
			if( OriginalThreadParam )
				Threading::SetThreadInfo( *OriginalThreadParam );
			auto& result = _pPromise->get_return_object();
			if( result.ExceptionPtr )
				throw result.ExceptionPtr;
			return result.Result;
		}
		uint _hClient;
	};
*/

	uint Count{0};
	Duration MinTick{1ms};
	using namespace Chrono;
	using std::chrono::milliseconds;

	struct Awaitable final : CancelAwaitable<Task<string>>
	{
		typedef CancelAwaitable<Task<string>> base;
		Awaitable( vector<sp<ResumeThread>>* pThreads, Duration idleLimit )noexcept:
			base(_hClient),
			IdleLimit{idleLimit},
			_pThreads{pThreads}
		{};
		~Awaitable()=default;
		void await_suspend( coroutine_handle<Task<string>::promise_type> h )noexcept override
		{
			base::await_suspend( h );
			_pPromise = &h.promise();
			optional<CoroutineParam> param{ CoroutineParam{h} };
			if( _pThreads )
			{
				if( _pThreads->size() )
					param = _pThreads->back()->Resume( move(*param) );
				if( param )
				{
					DBG( "new thread duration={}."sv, duration_cast<milliseconds>(IdleLimit).count() );
					_pThreads->push_back( make_shared<ResumeThread>("pool[0]", IdleLimit, move(*param)) );
				}
				else
					LOGN( ELogLevel::Information, "Resumed", 1 );
			}
			else
				CoroutinePool::Resume( move(h) );
		}
		string await_resume()noexcept override
		{
			//DBG( "({})TickManager::Awaitable::await_resume"sv, std::this_thread::get_id() );
			if( OriginalThreadParamPtr )
				Threading::SetThreadInfo( *OriginalThreadParamPtr );
			auto& result = _pPromise->get_return_object();
			return result.Result;
		}
		const Duration IdleLimit;
	private:
		uint _hClient;
		vector<sp<ResumeThread>>* _pThreads;
		Task<string>::promise_type* _pPromise{nullptr};
		//void End( Awaitable::Handle h, const Tick* pTick )noexcept; std::once_flag _singleEnd;
	};
	static auto Co( vector<sp<ResumeThread>>* pThreads, Duration idle )noexcept{ return Awaitable{pThreads, idle}; }

	vector<sp<ResumeThread>> threads;
	std::condition_variable_any cv;
	std::shared_mutex mtx;

	Coroutine::Task<string> Call()
	{
		ClearMemoryLog();
		co_await Co( &threads, MinTick*2 );
		jthread( []()->Coroutine::Task<string>
		{
			DBG( "Begin sleep1"sv );
			std::this_thread::sleep_for( MinTick );
			DBG( "End sleep1"sv );
			co_await Co( &threads, MinTick );//should resume
			std::this_thread::sleep_for( MinTick*2 );
			ASSERT_DESC( FindMemoryLog(1).size()==1, format("FindMemoryLog(1).size()=={}", FindMemoryLog(1).size()) );
			ClearMemoryLog();
			DBG( "--------------------------------New Test--------------------------------"sv );
			jthread( []()->Coroutine::Task<string>
			{
				DBG( "Begin sleep2"sv );
				std::this_thread::sleep_for( MinTick*2 );
				DBG( "End sleep2"sv );
				co_await Co( &threads, MinTick*2 );//should create new thread
				ASSERT_DESC( FindMemoryLog(1).size()==0, format("FindMemoryLog(1).size()=={}", FindMemoryLog(1).size()) );
				std::shared_lock l{ mtx };
				cv.notify_one();
			}).detach();
		}).detach();
	}
	TEST_F(CoroutineTests, ResumeThread)
	{
		Call();
		std::shared_lock l{ mtx };
		cv.wait( l );
		std::this_thread::sleep_for( MinTick*3 );
	}

	Coroutine::Task<string> CoroutineTests::CallPool()
	{
		ClearMemoryLog();
		co_await Co( nullptr, 1ms );
		ASSERT_DESC( CoroutinePool::_pInstance->_pQueue==nullptr && CoroutinePool::_pInstance->_pThread==nullptr, "Expected no queue when max thread count==1." );
		jthread( []()->Coroutine::Task<string>
		{
			co_await Co( nullptr, 1ms );//should queue
			ASSERT_DESC( CoroutinePool::_pInstance->_pQueue!=nullptr && CoroutinePool::_pInstance->_pThread!=nullptr, "Expected queue when max thread count==1." );
			std::shared_lock l{ mtx };
			cv.notify_one();
		}).detach();
		std::this_thread::sleep_for( MinTick*2 );
	}

	TEST_F(CoroutineTests, Pool)
	{
		auto& global = Settings::Global().Json();
		auto& value = global["coroutinePool"]["maxThreadCount"];
		value = 1;
		CallPool();
		std::shared_lock l{ mtx };
		cv.wait( l );
		std::this_thread::sleep_for( MinTick*3 );
	}

	α SyncLock()->Task2
	{
		for( uint i=0; i<50; ++i )
			auto l = co_await Threading::CoLock( "Test" );
	}

	α AsyncLock2( uint i, bool last )->Task2
	{
		static uint a = 0;
		auto l = co_await Threading::CoLock( "Test" );
		DBG( "a={}, i={}"sv, a, i );
		ASSERT( a==i );
		++a;
		if( last )
		{
			std::shared_lock l{ mtx };
			cv.notify_one();
		}
	}
	α AsyncLock()->Task2
	{
		var l = co_await Threading::CoLock( "Test" );
		for( uint i=0; i<50; ++i )
			AsyncLock2( i, i==49 );
		DBG( "~AsyncLock"sv );
	}
	TEST_F(CoroutineTests, CoLock)
	{
		var id = Threading::GetThreadId();
		//SyncLock();
		ASSERT_DESC( id==Threading::GetThreadId(), "Expecting same thread." );
		AsyncLock();
		std::shared_lock l{ mtx };
		cv.wait( l );
	}

/*	static auto Throw()noexcept{ return ThrowAwaitable{}; }
	Coroutine::Task<string> CoroutineTests::CallThrow()
	{
		bool thrown;
		try
		{
			co_await Throw();
			thrown = false;
		}
		catch( Exception& e )
		{
			DBG( string{e.what()} );
			thrown = true;
		}
		ASSERT_DESC( thrown, "Expecting thrown exception." );

		std::shared_lock l{ mtx };
		cv.notify_one();
	}

	TEST_F(CoroutineTests, Error)
	{
		Threading::SetThreadDscrptn( "Test" );
		CallThrow();

		std::shared_lock l{ mtx };
		cv.wait( l );
		std::this_thread::sleep_for( MinTick*3 );
	}*/
}