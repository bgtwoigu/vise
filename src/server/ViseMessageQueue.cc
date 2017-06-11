#include "ViseMessageQueue.h"

ViseMessageQueue* ViseMessageQueue::vise_global_message_queue_ = NULL;

ViseMessageQueue* ViseMessageQueue::Instance() {
  if ( !vise_global_message_queue_ ) {
    vise_global_message_queue_ = new ViseMessageQueue;
  }
  return vise_global_message_queue_;
}

void ViseMessageQueue::Push( const std::string &d ) {
  //boost::lock_guard<boost::mutex> guard(mtx_);
  boost::mutex::scoped_lock lock(mtx_);
  messages_.push( d );
  lock.unlock();
  queue_condition_.notify_one();
}

std::string ViseMessageQueue::BlockingPop() {
  boost::mutex::scoped_lock lock(mtx_);
  while ( messages_.empty() ) {
    queue_condition_.wait(lock);
/**/
    if ( queue_condition_.timed_wait(lock, boost::posix_time::milliseconds(997), []{ return false; }) ) {
      continue;
    } else {
      std::cout << "\nBlockingPop() TIMEOUT UNBLOCKED" << std::flush;
      return "";
    }

  }
  std::string d = messages_.front();
  messages_.pop();
  return d;
}

unsigned int ViseMessageQueue::GetSize() {
  boost::mutex::scoped_lock lock(mtx_);
  return messages_.size();
}

void ViseMessageQueue::WaitUntilEmpty() {
  boost::mutex::scoped_lock lock(mtx_);
  while ( !messages_.empty() ) {
    queue_condition_.wait(lock);
  }
}
