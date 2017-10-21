#include <mutex>
#include <condition_variable>
#include <deque>

namespace gtools{

template <typename T>
class BlockingQueue
{
private:
    std::mutex              d_mutex;
    std::condition_variable d_condition;
    std::deque<T>           d_queue;
public:
    void push(T const& value) {
        {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            d_queue.push_front(value);
        }
        this->d_condition.notify_one();
    }
    T pop() {
		// Acquire a lock.
        std::unique_lock<std::mutex> lock(this->d_mutex);
		
		// Simultaneously unlock the lock, and use a Lambda Predicate feature to check 
		// whether block is needed. Block only if pred returns false - in this case, if queue is empty.
        this->d_condition.wait(lock, [=]{ return !this->d_queue.empty(); });

		// Create a temporary variable to hold the value of the queue back.
		// Use super-efficient std::move to directly assign rc to the
		// memory of the returned rvalue.
        T rc( std::move(this->d_queue.back()) );
        this->d_queue.pop_back();

        return rc;
    }

	bool isEmpty() {
    	std::unique_lock<std::mutex> lock(this->d_mutex);
		return this->d_queue.empty();
	}
};

}
