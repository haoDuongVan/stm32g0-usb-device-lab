/*
 * key_event_queue.c
 *
 *  Created on: Jul 2, 2026
 *      Author: haodu
 */

/* Includes ------------------------------------------------------------------*/
#include "key_event_queue.h"
#include <stddef.h>

/* Private types -------------------------------------------------------------*/

// Groups the ring-buffer storage and its head/tail/count bookkeeping together
typedef struct
{
  KeyEvent_t buffer[KEY_EVENT_QUEUE_SIZE];
  uint8_t    head;   /* next write position */
  uint8_t    tail;   /* next read position  */
  uint8_t    count;
} KeyEventQueueState_t;

/* Private variables ---------------------------------------------------------*/
static KeyEventQueueState_t sQueue;

/* Function definitions ------------------------------------------------------*/

// Reset queue to empty. Call once before use
void KeyEventQueue_Init(void)
{
  sQueue.head  = 0U;
  sQueue.tail  = 0U;
  sQueue.count = 0U;
}

// Push one event. Returns false if queue is full or event is NULL
bool KeyEventQueue_Push(const KeyEvent_t *event)
{
  if (event == NULL)
  {
    return false;
  }

  if (KeyEventQueue_IsFull())
  {
    return false;
  }

  sQueue.buffer[sQueue.head] = *event;  // write at the current head slot

  sQueue.head++;
  if (sQueue.head >= KEY_EVENT_QUEUE_SIZE)
  {
    sQueue.head = 0U;  // wrap back to the start of the ring buffer
  }

  sQueue.count++;

  return true;
}

// Pop the oldest event into *event. Returns false if queue is empty
// Passing NULL for event discards the front element without copying it
bool KeyEventQueue_Pop(KeyEvent_t *event)
{
  if (KeyEventQueue_IsEmpty())
  {
    return false;
  }

  if (event != NULL)
  {
    *event = sQueue.buffer[sQueue.tail];
  }

  sQueue.tail++;
  if (sQueue.tail >= KEY_EVENT_QUEUE_SIZE)
  {
    sQueue.tail = 0U;  // wrap back to the start of the ring buffer
  }

  sQueue.count--;

  return true;
}

// Copy the oldest event without removing it. Returns false if queue is empty or event is NULL
bool KeyEventQueue_Peek(KeyEvent_t *event)
{
  if (event == NULL)
  {
    return false;
  }

  if (KeyEventQueue_IsEmpty())
  {
    return false;
  }

  *event = sQueue.buffer[sQueue.tail];

  return true;
}

// Return true if the queue contains no events
bool KeyEventQueue_IsEmpty(void)
{
  return (sQueue.count == 0U);
}

// Return true if the queue has no space for another event
bool KeyEventQueue_IsFull(void)
{
  return (sQueue.count >= KEY_EVENT_QUEUE_SIZE);
}

// Return the number of events currently in the queue
uint8_t KeyEventQueue_GetCount(void)
{
  return sQueue.count;
}
