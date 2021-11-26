#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <cassert>


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
	if (&a <= ctx.Low) {
		ctx.Low = &a;
	}
	else {
		ctx.Hight = &a;
	}
	std::size_t stack_size = ctx.Hight - ctx.Low;
	if (std::get<1>(ctx.Stack) < stack_size) {
		delete[] std::get<0>(ctx.Stack);
		std::get<1>(ctx.Stack) = stack_size;
		std::get<0>(ctx.Stack) = new char[stack_size];
	}

	memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);
		
}

void Engine::Restore(context &ctx) {
	char a;
	if (&a >= ctx.Low && &a <= ctx.Hight) {
		Restore(ctx);
	}
	memcpy(ctx.Low, std::get<0>(ctx.Stack),ctx.Hight - ctx.Low);
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
	sched(ctx);
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

void Engine::block(void *coro) {
	context * coro_to_block;
	if (coro == nullptr) {
		coro_to_block = cur_routine;	
	}
	else {
		coro_to_block = static_cast<context *>(coro);
	}
	if (!coro_to_block or coro_to_block->is_blocked) {
		return;	
	}
        coro_to_block->is_blocked = true;
	//delete from alive add to blocked
 	if (alive == coro_to_block) {
                alive = alive->next;
	}
	if (coro_to_block->prev) {
		coro_to_block->prev->next = coro_to_block->next;
	}
	if (coro_to_block->next) {
		coro_to_block->next->prev = coro_to_block->prev;
	}
	coro_to_block->prev = nullptr;
	coro_to_block->next = blocked;
	blocked = coro_to_block;
	if (coro_to_block->next) {
		coro_to_block->next -> prev = coro_to_block;
	}
	if (coro_to_block == cur_routine) {
		assert(cur_routine != idle_ctx);
		if (setjmp(cur_routine->Environment) > 0) {
			return;
		}
		Store(*cur_routine);
		Restore(*idle_ctx);
	}
}


void Engine::unblock(void *coro){ 
	context * coro_to_unblock = static_cast<context *> (coro);
	if (!coro_to_unblock or coro_to_unblock->is_blocked) {
		return;	
	}
	//delete from blocked add to alive
 	if (blocked == coro_to_unblock) {
                blocked = blocked->next;
	}
        coro_to_unblock->is_blocked = false;
	if (coro_to_unblock->prev) {
		coro_to_unblock->prev->next = coro_to_unblock->next;
	}
	if (coro_to_unblock->next) {
		coro_to_unblock->next->prev = coro_to_unblock->prev;
	}
	coro_to_unblock->prev = nullptr;
	coro_to_unblock->next = alive;
	alive = coro_to_unblock;
	if (coro_to_unblock->next) {
		coro_to_unblock->next -> prev = coro_to_unblock;
	}
}

} // namespace Coroutine
} // namespace Afina
