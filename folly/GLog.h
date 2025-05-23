/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/Likely.h>

#include <atomic>
#include <chrono>

#include <glog/logging.h>

#ifndef FB_LOG_EVERY_MS
/**
 * Issues a LOG(severity) no more often than every
 * milliseconds. Example:
 *
 * FB_LOG_EVERY_MS(INFO, 10000) << "At least ten seconds passed"
 *   " since you last saw this.";
 *
 * The implementation uses for statements to introduce variables in
 * a nice way that doesn't mess surrounding statements.  It is thread
 * safe.  Non-positive intervals will always log.
 */
#define FB_LOG_EVERY_MS(severity, milli_interval)                              \
  for (decltype(milli_interval)                                                \
           FB_LEM_once = 1,                                                    \
           FB_LEM_interval = (milli_interval);                                 \
       FB_LEM_once;)                                                           \
    for (::std::chrono::milliseconds::rep FB_LEM_prev,                         \
         FB_LEM_now = FB_LEM_interval <= 0                                     \
             ? 0                                                               \
             : ::std::chrono::duration_cast<::std::chrono::milliseconds>(      \
                   ::std::chrono::system_clock::now().time_since_epoch())      \
                   .count();                                                   \
         FB_LEM_once;)                                                         \
      for (static ::std::atomic<::std::chrono::milliseconds::rep> FB_LEM_hist; \
           FB_LEM_once;                                                        \
           FB_LEM_once = 0)                                                    \
        if (FB_LEM_interval > 0 &&                                             \
            (FB_LEM_now -                                                      \
                     (FB_LEM_prev =                                            \
                          FB_LEM_hist.load(std::memory_order_acquire)) <       \
                 FB_LEM_interval ||                                            \
             !FB_LEM_hist.compare_exchange_strong(FB_LEM_prev, FB_LEM_now))) { \
        } else                                                                 \
          LOG(severity)

#endif

/**
 * Issues a LOG(severity) once.
 *
 *   FB_LOG_ONCE(ERROR) << "Log this error only once";
 *
 * This macro is thread-safe and does not impact surrounding statements.
 *
 * NOTE: A load() is used in the fast-path scenario (steady state) in order to
 * avoid a locked RMW operation.
 */
#ifndef FB_LOG_ONCE
#define FB_LOG_ONCE(severity)                                                  \
  for (auto __folly_detail_glog_once = true; __folly_detail_glog_once;)        \
    for (static ::std::atomic_bool __folly_detail_glog_logged{false};          \
         __folly_detail_glog_once;                                             \
         __folly_detail_glog_once = false)                                     \
      if (FOLLY_LIKELY(                                                        \
              __folly_detail_glog_logged.load(::std::memory_order_relaxed)) || \
          __folly_detail_glog_logged.exchange(                                 \
              true, ::std::memory_order_relaxed)) {                            \
      } else                                                                   \
        LOG(severity)
#endif
