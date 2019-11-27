#pragma once
#include "Exception.h"

/*
#ifndef ASSRT_TR
# define ASSRT_TR( actual ){ if( !(actual) ) Jde::Logging::LogCritical( Logging::MessageBase(ELogLevel::Critical, "Assert:  {} is false"sv, __FILE__, __func__, __LINE__), #actual );  assert( actual ); }
#endif
*/
#ifndef ASSERT
# define ASSERT( actual ){ if( !(actual) ){ CRITICAL("Assert:  {} is false"sv,  #actual ); } assert( actual ); }
#endif
//#ifndef ASSRT_TR
//# define ASSRT_TR ASSERT
//#endif
#ifndef ASSERT_DESC
# define ASSERT_DESC( actual, desc ) {if( !(actual) ){ CRITICAL("Assert:  {} - {} is false"sv, desc, #actual ); } assert( actual );}
#endif
// #ifndef ASSRT_TR_DESC
// 	# define ASSRT_TR ASSERT
// #endif

#ifndef ASSRT_LT
	#define ASSRT_LT( expected, actual ) if( !(actual<expected) )CRITICAL("Assert: (expected:  {}) {} < {} (actual:  {})"sv, expected, #expected, #actual, actual ); assert( actual<expected );
#endif
// #ifndef ASSRT_LT_
// # define ASSRT_LT_( expected, actual, message ) if( !(actual<expected) )CRITICAL0( message ); assert( actual<expected );
// #endif
// #ifndef ASSRT_GT
// # define ASSRT_GT( expected, actual ) if( !(actual>expected) )CRITICAL("Assert:  (expected:  {}) {} > {} (actual:  {})"sv, expected, #expected, #actual, actual ); assert( actual>expected );
// #endif
// #ifndef ASSRT_LTE
// # define ASSRT_LTE( expected, actual ) {if( !(actual<=expected) )CRITICAL("ASSERT ({}) {}<={} ({})"sv, #actual, actual, expected, #expected ); assert(actual<=expected);}
// #endif
#ifndef ASSRT_EQ
	# define ASSRT_EQ( expected, actual ) if( expected!=actual ){CRITICAL("Assert:  (expected:  {}) {}=={} (actual:  {})"sv, expected, #expected, #actual, actual ); } assert( expected==actual );
#endif
// #ifndef ASSRT_NE
// # define ASSRT_NE( expected, actual ) if( actual==expected ){WARN("Expected ({}) {}!={} ({})"sv, #actual, actual, expected, #expected ); }
// #endif
#ifndef ASSRT_NN
# define ASSRT_NN( actual ) if( !actual )CRITICAL("Expected {} to not be null."sv, #actual ); assert( actual );
#endif
#ifndef ASSRT_NN_DESC
# define ASSRT_NN_DESC( actual, desc ) if( !actual )CRITICAL("Expected {} to not be null - {}"sv, #actual, desc ); assert( actual );
#endif
#ifndef ASSERT_NULL
# define ASSERT_NULL( p ) if( p )CRITICAL("Expected '{}' to be null."sv, #p ); assert( !p );
#endif
#ifndef ASSRT_BETWEEN
# define ASSRT_BETWEEN( expected_low, expected_high, actual ) if( !(expected_low<=actual && expected_high>actual) )CRITICAL("Expected ({}) {} to be between ({}){} and {}({}) null.", #actual, actual, #expected_low, expected_low, expected_high, #expected_high ); assert( expected_low<=actual && expected_high>actual );
#endif
// #ifndef VERIFY_LT
// # define VERIFY_LT( expected, actual ) if( !(actual<expected) ){WARN("Expected ({}) {}< {} ({})"sv, #actual, actual, expected, #expected ); }
// #endif
#ifndef VERIFY
# define VERIFY( actual ) if( !(actual) ){WARN( "VERIFY_TR - Expected {} to be false"sv, #actual ); }
#endif


// #ifndef TEST_LT
// # define TEST_LT( expected, actual ) if( !(actual<expected) ){ THROW( Exception(fmt::format("Expected ({}) {}< {} ({})", #actual, actual, expected, #expected)) ); }
// #endif
// #ifndef TEST_LTE
// # define TEST_LTE( expected, actual ) if( !(actual<=expected) ){ THROW( Exception(fmt::format("Expected ({}) {}<={} ({})", #actual, actual, expected, #expected)) ); }
// #endif
