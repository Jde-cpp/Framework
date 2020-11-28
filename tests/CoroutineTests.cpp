#include "gtest/gtest.h"
#include "../source/threading/Alarm.h"
#include "../source/threading/Coroutine.h"
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
	};
	uint Count{0};
	Duration MinTick{1ms};
	using namespace Chrono;
	using std::chrono::milliseconds;

	struct Awaitable final : CancelAwaitable<Task<string>>
	{
		typedef CancelAwaitable<Task<string>> base;
		Awaitable( vector<sp<ResumeThread>>* pThreads, Duration idleLimit )noexcept:base(_hClient),IdleLimit{idleLimit},_pThreads{pThreads}
		{};
		~Awaitable()=default;
		bool await_ready()noexcept{return false;};
		void await_suspend( Awaitable::Handle h )noexcept override
		{
			base::await_suspend( h );
			optional<CoroutineParam> param{ CoroutineParam{OriginalThreadParam.value(), h} };
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
					LOGN0( ELogLevel::Information, "Resumed", 1 );
			}
			else
				CoroutinePool::Resume( move(h), move(*OriginalThreadParam) );
		}
		string await_resume()noexcept
		{
			//DBG( "({})TickManager::Awaitable::await_resume"sv, std::this_thread::get_id() );
			return _pPromise->get_return_object().Result;
		}
		const Duration IdleLimit;
	private:
		uint _hClient;
		vector<sp<ResumeThread>>* _pThreads;
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
			DBG0( "Begin sleep1"sv );
			std::this_thread::sleep_for( MinTick );
			DBG0( "End sleep1"sv );
			co_await Co( &threads, MinTick );//should resume
			std::this_thread::sleep_for( MinTick );
			ASSERT_DESC( FindMemoryLog(1).size()==1, format("FindMemoryLog(1).size()=={}", FindMemoryLog(1).size()) );
			ClearMemoryLog();
			DBG0( "--------------------------------New Test--------------------------------"sv );
			jthread( []()->Coroutine::Task<string>
			{
				DBG0( "Begin sleep2"sv );
				std::this_thread::sleep_for( MinTick*2 );
				DBG0( "End sleep2"sv );
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
		global["coroutinePool"]["maxThreadCount"] = 1;
		CallPool();
		std::shared_lock l{ mtx };
		cv.wait( l );
		std::this_thread::sleep_for( MinTick*3 );
	}
}