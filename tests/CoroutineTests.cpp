﻿#include "gtest/gtest.h"
#include <random>
#include "../source/coroutine/Alarm.h"
#include "../source/coroutine/Coroutine.h"
#include "../source/threading/Mutex.h"
#include "../source/log/server/ServerSink.h"
#include "../source/coroutine/Lock.h"
#include <jde/io/File.h>
#ifdef _MSC_VER
	#include "../../Windows/source/WindowsDrive.h"
#endif

#define var const auto
namespace Jde::Coroutine
{
	static const LogTag& _logLevel = Logging::TagLevel( "tests" );

	struct CoroutineTests : public ::testing::Test
	{
	protected:
		CoroutineTests() {}
		~CoroutineTests() override{}

		void SetUp() override {}
		void TearDown() override {}
	public:
	};

	uint Count{0};
	Duration MinTick{1ms};
	vector<sp<ResumeThread>> threads;
	std::condition_variable_any cv;
	std::shared_mutex mtx;
	std::shared_mutex mtx2;
	using namespace Chrono;
	using std::chrono::milliseconds;
	/*
	struct Awaitable final : CancelAwaitable<Task2>
	{
		typedef CancelAwaitable<Task2> base;
		Awaitable( vector<sp<ResumeThread>>* pThreads, Duration idleLimit )noexcept:
			base(_hClient),
			IdleLimit{idleLimit},
			_pThreads{pThreads}
		{};
		~Awaitable()=default;
		void await_suspend( coroutine_handle<Task2::promise_type> h )noexcept override
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
					_pThreads->push_back( ms<ResumeThread>("pool[0]", IdleLimit, move(*param)) );
				}
				else
					LOGN( ELogLevel::Information, "Resumed", 1 );
			}
			else
				CoroutinePool::Resume( move(h) );
		}
		TaskResult await_resume()noexcept override
		{
			//DBG( "({})TickManager::Awaitable::await_resume"sv, std::this_thread::get_id() );
			if( OriginalThreadParamPtr )
				Threading::SetThreadInfo( *OriginalThreadParamPtr );
			auto& result = _pPromise->get_return_object();
			return result.GetResult();
		}
		const Duration IdleLimit;
	private:
		uint _hClient;
		vector<sp<ResumeThread>>* _pThreads;
		Task2::promise_type* _pPromise{nullptr};
		//void End( Awaitable::Handle h, const Tick* pTick )noexcept; std::once_flag _singleEnd;
	};
	static auto Co( vector<sp<ResumeThread>>* pThreads, Duration idle )noexcept{ return Awaitable{pThreads, idle}; }

	α Call()->Coroutine::Task2
	{
		ClearMemoryLog();
		co_await Co( &threads, MinTick*2 );
		jthread( []()->Coroutine::Task2
		{
			DBG( "Begin sleep1"sv );
			std::this_thread::sleep_for( MinTick );
			DBG( "End sleep1"sv );
			co_await Co( &threads, MinTick );//should resume
			std::this_thread::sleep_for( MinTick*2 );
			ASSERT_DESC( FindMemoryLog(1).size()==1, format("FindMemoryLog(1).size()=={}", FindMemoryLog(1).size()) );
			ClearMemoryLog();
			DBG( "--------------------------------New Test--------------------------------"sv );
			jthread( []()->Coroutine::Task2
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

	α CoroutineTests::CallPool()->Coroutine::Task2
	{
		ClearMemoryLog();
		co_await Co( nullptr, 1ms );
		ASSERT_DESC( CoroutinePool::_pInstance->_pQueue==nullptr && CoroutinePool::_pInstance->_pThread==nullptr, "Expected no queue when max thread count==1." );
		jthread( []()->Coroutine::Task2
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
	*/

	TEST_F(CoroutineTests, File)
	{
		var chunkSize = IO::DriveWorker::ChunkSize();
		var threadSize = IO::DriveWorker::ThreadSize();
		constexpr uint itemSize = sizeof( double );
		var itemCount = chunkSize*threadSize*2.5/itemSize;
		var totalSize = (itemCount+1)*itemSize;
		auto pBuffer = ms<vector<char>>( (size_t)totalSize );
		std::uniform_real_distribution<double> unif( 0, std::numeric_limits<double>::max() );
		std::default_random_engine re;
		for( uint i=0; i<itemCount; ++i )
		{
		   double v = unif( re );
			memcpy( pBuffer->data()+i*itemSize, &v, itemSize );
		}
		var path = fs::path( fs::temp_directory_path()/"test.dat" );
		fs::remove( path );
		VFuture( IO::Write(path, pBuffer) ).get();
		var pRead = SFuture<vector<char>>( IO::Read(path) ).get();
		bool equal = *pBuffer==*pRead;
		ASSERT_TRUE( equal );
	}

	α LockTest( sv symbol, uint i, bool singleThread=true, stop_token st={} )->Task
	{
		if( !singleThread )
			Threading::SetThreadDscrptn( format("{} - Starting",i) );
		var id = std::this_thread::get_id();
		var _ = ( co_await CoLockKey( format("Twitter::Search.{}", symbol), true) ).UP<CoLockGuard>();
		if( !singleThread )
			Threading::SetThreadDscrptn( format("{} - Running",i) );
		if( singleThread )
		{
			ASSERT( id==std::this_thread::get_id() );
		}
		else
		{
			while( !st.stop_requested() )
				std::this_thread::sleep_for( 1ms );
		}
		LOG( "({})Test - Done", i );
		co_return;
	}
	TEST_F(CoroutineTests, Locks)
	{
		//for( uint i=0; i<5; ++i )
		//	LockTest( "TSLA", i );

		array<up<jthread>,10> threads;
		for( uint i=0; i<threads.size(); ++i )
		{
			threads[i] = mu<jthread>( [i](stop_token st){ LockTest("TSLA", i, false, st); } );
		}
		std::this_thread::sleep_for( 1s );
		for( uint i=0; i<threads.size(); ++i )
			threads[i]->request_stop();
		for( uint i=0; i<threads.size(); ++i )
		{
			threads[i]->join();
			LOG( "({})Test - join complete", i );
			std::this_thread::sleep_for( 1ms );
		}
		std::this_thread::sleep_for( 1min );
	}
}