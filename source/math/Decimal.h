#pragma once
#include <iomanip>
#include "MathUtilities.h"
namespace Jde
{
	enum class DecimalType{ Max, Min };
/*	struct IDecimal
	{
		virtual string to_string()const noexcept=0;
		//operator bool()const noexcept{ return _value; }
	protected:
		IDecimal()=default;
		constexpr IDecimal( int64_t value ):_value(value){};
		constexpr IDecimal( DecimalType type ):_value( type==DecimalType::Max ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int64_t>::min() ){}
		//virtual operator double()const noexcept=0;
		virtual uint Multiplier()const noexcept=0;
	protected:
		int64_t _value{0};
		//static constexpr int64_t _max{ std::numeric_limits<int64_t>::max() };
	};
	inline std::ostream& operator<<( std::ostream& os, const IDecimal& decimal )noexcept
	{
		os << decimal.to_string();
		return os;
	}
	*/
	typedef int64_t Decimal0;

	struct Decimal2 //: IDecimal
	{
		Decimal2()=default;
		Decimal2( double value ):_value{ lround(value*Multiplier()) }{}
		constexpr Decimal2( DecimalType type ):_value(  type==DecimalType::Max ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int64_t>::min() ){}
		//explicit constexpr Decimal2( int64_t value ):IDecimal( value*_multiplier ){}
		//Decimal2( uint value ):IDecimal( value*_multiplier ){}
		//Decimal2( _int value ):IDecimal( v alue*_multiplier ){}
		explicit operator uint()const noexcept{ return Math::URound(_value/100.0); }
		explicit operator double()const noexcept{ return _value/100.0; }
		explicit operator float()const noexcept{ return _value/100.0f; }
		Decimal2 operator -()const{ Decimal2 result; result._value=-_value; return result; }
		friend Decimal2 operator -( const Decimal2& value1, const double& value2 )noexcept{ return Decimal2( static_cast<double>(value1._value/100.0)-value2 ); }
		//friend Decimal2 operator -( const Decimal2& value1, const _int& value2 )noexcept{ return Decimal2( static_cast<double>(value1._value/100.0)-value2 ); }
		friend Decimal2 operator -( const Decimal2& value1, const Decimal2& value2 )noexcept{ auto result=Decimal2();result._value=value1._value-value2._value; return result; }
		friend Decimal2 operator +( const Decimal2& value1, const _int& value2 )noexcept{ return Decimal2( static_cast<double>(value1._value/100.0)+value2 ); }
		friend Decimal2 operator +( const Decimal2& value1, const double& value2 )noexcept{ return Decimal2( static_cast<double>(value1._value/100.0)+value2 ); }
		friend Decimal2 operator +( const Decimal2& value1, const Decimal2& value2 )noexcept{ auto result=Decimal2();result._value=value1._value+value2._value; return result; }

		friend double operator/(const Decimal2& numerator, const Decimal2& denominator)noexcept{ return numerator._value/static_cast<double>(denominator._value);}
		friend Decimal2 operator*(const Decimal2& value, uint other)noexcept{ return Decimal2( (value._value*other)/100.0 );}
		friend Decimal2 operator*(const Decimal2& value, _int other)noexcept{ return Decimal2( (value._value*other)/100.0 );}
		friend Decimal2 operator*(const Decimal2& value, int other)noexcept{ return Decimal2( (value._value*other)/100.0 );}
		friend Decimal2 operator*(const Decimal2& value, double other)noexcept{ return Decimal2( (value._value/100.0)*other ) ;}
		friend Decimal2 operator*(double other, const Decimal2& value)noexcept{ return Decimal2( (value._value/100.0)*other ) ;}
		friend bool operator!=(const Decimal2& value, double other)noexcept{ return llround(other*100)!=value._value; }
		friend bool operator!=(const Decimal2& value, const Decimal2& other)noexcept{ return other._value!=value._value; }
		friend bool operator==(const Decimal2& value, double other)noexcept{ return llround(other*100)==value._value; }
		friend bool operator==(const Decimal2& value, const Decimal2& other)noexcept{ return value._value==other._value; }
		friend bool operator<(const Decimal2& value, double other)noexcept{ return value._value<llround(other*100); }
		friend bool operator<=(const Decimal2& value, double other)noexcept{ return ((double)value)<=other; }
		friend bool operator<=(const Decimal2& value, const Decimal2& other)noexcept{ return value._value<=other._value; }
		friend bool operator>=(const Decimal2& value, const Decimal2& other)noexcept{ return value._value>=other._value; }
		friend bool operator>=(const Decimal2& value, double other)noexcept{ return ((double)value)>=other; }
		friend bool operator>(const Decimal2& value, double other)noexcept{ return value._value>llround(other*100); }
		friend bool operator<(const Decimal2& value, const Decimal2& value2)noexcept{ return value._value<value2._value; }
		friend bool operator>(const Decimal2& value, const Decimal2& value2)noexcept{ return value._value>value2._value; }
		
		Decimal2& operator -=(Decimal2 decrement)noexcept{ _value-=decrement._value; return *this;}
		Decimal2& operator +=(Decimal2 decrement)noexcept{ _value+=decrement._value; return *this;}
		template<typename T> std::basic_string<T> to_string()const noexcept;// override;
		//static constexpr Decimal2 Max{ Decimal2;
	private:
		//constexpr static uint _multiplier{100};
		int64_t _value{0};
		//static Decimal2 _maxDecimal{0.0};
	protected:
		static constexpr uint Multiplier()noexcept/* override*/{ return 100;}
	};
	static const Decimal2 AmountMax{ Decimal2(DecimalType::Max) };
	//static constexpr int64_t _max{  };
	template<typename T>
	std::basic_ostream<T>& operator<<( std::basic_ostream<T>& os, const Decimal2& decimal )noexcept
	{
		os << decimal.to_string<T>();
		return os;
	}

	template<typename T> 
	inline std::basic_string<T> Decimal2::to_string()const noexcept 
	{
		std::basic_ostringstream<T> os;
		os << std::fixed << std::setprecision(2)  << static_cast<double>(_value/100.0);
		return os.str();
	}

/*	template<uint8 TPlaces>
	struct Decimal
	{
		Decimal( double value );
		operator double()const noexcept override;
		int64_t Multiplier()const noexcept override{ return _multiplier;}
		virtual string to_string()const noexcept=0;
	private:
		static uint _multiplier = static_cast<uint>( ::pow(10.0, TPlaces) );
	};
	
	template<uint8 TPlaces>
	Decimal<TPlaces>::Decimal( double value ):
		IDecimal( value * _multiplier )
	{}

	template<uint8 TPlaces>
	Decimal<TPlaces>::operator double()const noexcept
	{
		return static_cast<double>(_value)/static_cast<double>(_multipler);
	}
	template<uint8 TPlaces>
	string Decimal<TPlaces>::to_string()const noexcept
	{
		ostringstream os;
		//os.precision( TPlaces );
		os << std::fixed() << std::setprecision(TPlaces)  << std::setprecision(5);
		return os.str();
	}
	*/
}