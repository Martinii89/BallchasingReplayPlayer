#pragma once
#include <functional>

class ScopeExec
{
public:
	ScopeExec(const ScopeExec& other) = delete;
	ScopeExec(ScopeExec&& other) noexcept = delete;
	ScopeExec& operator=(const ScopeExec& other) = delete;
	ScopeExec& operator=(ScopeExec&& other) noexcept = delete;

	explicit ScopeExec(std::function<void()> deleter);

	~ScopeExec();

private:
	std::function<void()> on_scope_exit_;
};
