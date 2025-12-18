#ifndef XUTILS2_LIGHT_WEIGHT_SEMAPHORE_HPP
#define XUTILS2_LIGHT_WEIGHT_SEMAPHORE_HPP 1

// Provides an efficient implementation of a semaphore (LightweightSemaphore).
// This is an extension of Jeff Preshing's sempahore implementation (licensed
// under the terms of its separate zlib license) that has been adapted and
// extended by Cameron Desrochers.

#pragma once

#include <cstddef> // For std::size_t
#include <atomic>
#include <cassert>
#include <memory>
#include <type_traits> // For std::make_signed<T>
#include <XHelper/xversion.hpp>
#include <XGlobal/xclasshelpermacros.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

namespace moodycamel {

// Code in the mpmc_sema namespace below is an adaptation of Jeff Preshing's
// portable + lightweight semaphore implementations, originally from
// https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h
// LICENSE:
// Copyright (c) 2015 Jeff Preshing
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//	claim that you wrote the original software. If you use this software
//	in a product, an acknowledgement in the product documentation would be
//	appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//	misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
#if defined(_WIN32)
#elif defined(__MACH__)
#elif defined(__unix__) || defined(__MVS__)
#else
#error Unsupported platform! (No semaphore wrapper available)
#endif

//---------------------------------------------------------
// LightweightSemaphore
//---------------------------------------------------------

class XLightweightSemaphore;
class XLightweightSemaphorePrivate;

class XLightweightSemaphoreData {
public:
	XLightweightSemaphore * m_x_ptr{};
protected:
	constexpr XLightweightSemaphoreData() = default;
public:
	virtual ~XLightweightSemaphoreData() = default;
};

class XLightweightSemaphore final {

	X_DECLARE_PRIVATE_D(m_d_ptr, XLightweightSemaphore)
	std::unique_ptr<XLightweightSemaphoreData> m_d_ptr{};
	X_DISABLE_COPY(XLightweightSemaphore)

public:
	using ssize_t = std::make_signed_t<std::size_t> ;

	explicit XLightweightSemaphore(ssize_t initialCount = {}, int maxSpins = 10000);

	bool tryWait() noexcept;

	bool wait() noexcept;

	bool wait(std::int64_t timeout_usecs) noexcept;

	// Acquires between 0 and (greedily) max, inclusive
	ssize_t tryWaitMany(ssize_t max) noexcept;

	// Acquires at least one, and (greedily) at most max
	ssize_t waitMany(ssize_t max, std::int64_t timeout_usecs) noexcept;

	ssize_t waitMany(ssize_t max) noexcept;

	void signal(ssize_t count = 1) noexcept;

	[[nodiscard]] std::size_t availableApprox() const noexcept;
};

}

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
