#include <jde/coroutine/Task.h>

namespace Jde::Coroutine
{
	std::atomic<ClientHandle> _handleIndex{0};
	Handle NextHandle()noexcept{return ++_handleIndex;}

	std::atomic<ClientHandle> TaskHandle{0};
	Handle NextTaskHandle()noexcept{ return ++TaskHandle; }

	std::atomic<ClientHandle> TaskPromiseHandle{0};
	Handle NextTaskPromiseHandle()noexcept{ return ++TaskPromiseHandle; }
}
