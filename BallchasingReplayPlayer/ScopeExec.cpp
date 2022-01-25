#include "pch.h"
#include "ScopeExec.h"

ScopeExec::ScopeExec(std::function<void()> deleter): on_scope_exit_(std::move(deleter))
{
}

ScopeExec::~ScopeExec()
{
	if (on_scope_exit_)
	{
		on_scope_exit_();
	}
}
