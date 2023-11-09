#ifndef IGECS_EVT_QUEUE_H
#define IGECS_EVT_QUEUE_H

#include <concurrentqueue.h>

namespace igecs {

template <typename T>
struct CtxEventQueue {
  moodycamel::ConcurrentQueue<T> queue;
};

}  // namespace igecs

#endif