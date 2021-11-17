#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {


Engine::~Engine() {
	if (StackBottom) {
		delete[] std::get<0>(idle_ctx->Stack);
		delete idle_ctx;
	}
	while (alive) {
		context *ctx = alive;
		alive = alive->next;
		delete[] std::get<0>(ctx->Stack);
		delete ctx;
	}
	while (blocked) {
		context *ctx = blocked;
		blocked = blocked->next;
		delete[] std::get<0>(ctx->Stack);
		delete ctx;
	}
}


void Engine::Store(context &ctx) {
	char a;
	ctx.Hight = &a;
	std::size_t stack_size = ctx.Hight - ctx.Low;
	if (std::get<1>(ctx.Stack) < stack_size) {
		delete[] std::get<0>(ctx.Stack);
		std::get<1>(ctx.Stack) = stack_size;
		std::get<0>(ctx.Stack) = new char[stack_size];
	}

	memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);
		
}

void Engine::Restore(context &ctx) {
	std::size_t stack_size = ctx.Hight - ctx.Low;
	//
	memcpy(ctx.Low, std::get<0>(ctx.Stack),stack_size);
	cur_routine = &ctx;
	longjmp(ctx.Environment, 1);
}

void Engine::yield() {
	if (!alive || (cur_routine == alive && !alive->next)) {
		return;
	}
	context *ctx = alive;
	if (ctx == cur_routine) {
		ctx = ctx->next;
	}
	if (ctx) {
		sched(ctx);
	}
}

void Engine::sched(void *routine_) {
	context *routine = static_cast<context *>(routine_);
	if (routine_ == nullptr) {
		yield();
		return;
	}
	if (cur_routine == routine || routine->is_blocked) {
		return;
	}
	if (cur_routine != idle_ctx) {
		if (setjmp(cur_routine->Environment) > 0) {
			return;
		}
		Store(*cur_routine);
	}
	Restore(*routine);
}

} // namespace Coroutine
} // namespace Afina
