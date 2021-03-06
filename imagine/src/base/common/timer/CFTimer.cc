/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/base/Timer.hh>
#include <imagine/logger/logger.h>
#include <limits>

namespace Base
{

CFTimer::CFTimer(const char *debugLabel, CallbackDelegate c):
	debugLabel{debugLabel ? debugLabel : "unnamed"},
	info{std::make_unique<CFTimerInfo>(CFTimerInfo{c, {}})}
{}

CFTimer::CFTimer(CFTimer &&o)
{
	*this = std::move(o);
}

CFTimer &CFTimer::operator=(CFTimer &&o)
{
	deinit();
	timer = std::exchange(o.timer, {});
	info = std::move(o.info);
	debugLabel = o.debugLabel;
	return *this;
}

CFTimer::~CFTimer()
{
	deinit();
}

void CFTimer::callbackInCFAbsoluteTime(CFAbsoluteTime relTime, CFTimeInterval repeatInterval, CFRunLoopRef loop)
{
	auto realRepeatInterval = repeatInterval ? repeatInterval
		: std::numeric_limits<CFTimeInterval>::max(); // set a massive repeat interval to reuse a one-shot timer
	if(timer && CFRunLoopTimerGetInterval(timer) != realRepeatInterval)
	{
		// re-create timer if repeat interval changed
		deinit();
	}
	CFAbsoluteTime time = CFAbsoluteTimeGetCurrent() + relTime;
	if(unlikely(!timer))
	{
		CFRunLoopTimerContext context{};
		context.info = info.get();
		timer = CFRunLoopTimerCreate(nullptr, time, realRepeatInterval, 0, 0,
			[](CFRunLoopTimerRef timer, void *infoPtr)
			{
				using namespace Base;
				logMsg("running callback for timer: %p", timer);
				auto &info = *((CFTimerInfo*)infoPtr);
				bool keep = info.callback();
				if(!keep)
				{
					CFRunLoopRemoveTimer(info.loop, timer, kCFRunLoopDefaultMode);
					info.loop = {};
				}
			}, &context);
		logMsg("creating timer:%p (%s) to run in %f sec(s), repeats %f sec(s)",
			timer, label(), (double)relTime, (double)repeatInterval);
	}
	else
	{
		logMsg("re-arming timer:%p (%s) to run in %f sec(s), repeats %f sec(s)",
			timer, label(), (double)relTime, (double)repeatInterval);
		CFRunLoopTimerSetNextFireDate(timer, time);
	}
	if(loop != info->loop)
	{
		if(info->loop)
			CFRunLoopRemoveTimer(info->loop, timer, kCFRunLoopDefaultMode);
		CFRunLoopAddTimer(loop, timer, kCFRunLoopDefaultMode);
		info->loop = loop;
	}
}

void Timer::run(Time time, Time repeatTime, EventLoop loop, CallbackDelegate callback)
{
	if(callback)
		setCallback(callback);
	if(!loop)
		loop = EventLoop::forThread();
	callbackInCFAbsoluteTime(time.count(), repeatTime.count(), loop.nativeObject());
}

void Timer::cancel()
{
	if(!info->loop)
		return;
	CFRunLoopRemoveTimer(info->loop, timer, kCFRunLoopDefaultMode);
	info->loop = {};
}

void Timer::setCallback(CallbackDelegate callback)
{
	info->callback = callback;
}

bool Timer::isArmed()
{
	return info->loop;
}

void CFTimer::deinit()
{
	if(!timer)
		return;
	logMsg("closing timer: %p", timer);
	CFRunLoopTimerInvalidate(timer);
	CFRelease(timer);
	timer = {};
	info->loop = {};
}

const char *CFTimer::label()
{
	return debugLabel;
}

}
