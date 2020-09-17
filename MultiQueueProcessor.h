/* bugs at current code:
	1. MaxCapacity is the limit to queue size only. No limit to number of consumers
	2. Key is passed by value at IConsumer::Consume(). If it is a huge type, then it may be better to pass by const& or &&
	3. StopProcessing() only changes flag and does not notify Process(), so it will not stop immediately, if it is inside Sleep()
	4. Sub/Unsub/Enqueue traverse consumers twice: first search in consumers using find, second using insert/erase
	5. If Sub() is called twice with same key, consumer will not be reassingned. May return bool false or use insert_or_assing func
	6. Sleep() func is not crosspltaform. May use std::this_thread::sleep_for()
	7. Process() calls Consume() with locked mutex. It may lead to a deadlock:
		external thead locked external mutex and called Enqueue for example, while Process()-thread locked mtx and called Consume(), which tries to lock external mutex.
	   We can unlock the mutex to call Consume(), but a side effect is: Consume() may be called even after Unsub() is called.
	   Also, Sub/Unsub/Enqueue will wait for mutex, while Process() thread is calling all the Consume() handlers, that can take a long time.
	   It may be better to miss a cv notifation (and process at a timeout), than to block Sub/Unsub/Enqueue
	8. IConsumer is a raw pointer. External code should guarantee the lifetime of the object.
		May use shared_ptr/weak_ptr to control the life time of a consumer
	9. Cosume() will not be called with default constructed Value, due to "if (front != Value{})"
		it would be better, if Dequeue() would return a bool true, if it got something from queue
	10. While iterating queues and consumers, we process on Deqeuue() at a time.
		It might be better to process the whole queue first, like "while(Dequeue())", then continue with next queue iteration
	11. Better use atomic for "bool running"
	12. We have two maps (consumers and queues) with same key. Better to use one map.
		And depending on situation, may be use other types of map, like:
		std::unordered_map, boost::flat_map, google::dense_map, etc.
	13. Member is called queues, but contains std::list. At least for consistency, it may be better to use std::queue
	14. Enqueue() returns void. May return bool, to tell caller, that we did not enqueue the task due to MaxCapacity limit
		Or need to pop old task, without Process() them
	15. Dequeue() if public, so anyone can "steal" a task from queue. Make it private/protected
	16. Queues, mutex, thead are protected. May be better to make them private
	17. Not all include files we added: "functional" for std::bind
	18. How to deal with Enqueue() before Sub()? I chosed not to lose tasks, so save them in a queue
*/ 

#pragma once
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <memory>
#include <condition_variable>

template<typename Key, typename Value>
struct IConsumer
{
	virtual void Consume(Key id, const Value &value)
	{
		id;
		value;
	}
};

#define MaxQueueCapacity 1000
#define MaxHandlersCapacity 1000

template<typename Key, typename Value>
class MultiQueueProcessor
{
public:
	MultiQueueProcessor() :
		running{ true },
		ready{ false },
		th(std::bind(&MultiQueueProcessor::Process, this)) {}

	~MultiQueueProcessor()
	{
		StopProcessing();
		th.join();
	}

	void StopProcessing()
	{
		running = false;
	}

	bool Subscribe(Key id, std::weak_ptr<IConsumer<Key, Value>> consumer)
	{
		{
			std::lock_guard<std::mutex> lock{ mtx };
			auto it = data.find(id);
			if (it == data.end())
			{
				if (data.size() >= MaxHandlersCapacity)
					return false;
				it = data.emplace(id, Data()).first;
			}
			it->second.handler = std::move(consumer);
			ready = true;
		}
		cv.notify_one();
		return true;
	}

	void Unsubscribe(Key id)
	{
		std::lock_guard<std::mutex> lock{ mtx };
		data.erase(id);
	}

	bool Enqueue(Key id, Value value)
	{
		{
			std::lock_guard<std::mutex> lock{ mtx };
			auto& d = data[id];
			if (d.tasks.size() >= MaxQueueCapacity)
				return false;
			d.tasks.push(value);
			ready = true;
		}
		cv.notify_one();
		return true;
	}

protected:
	void Process()
	{
		while (running)
		{
			decltype(data) d;
			{
				std::unique_lock<std::mutex> lock{ mtx };
				cv.wait_for(lock, std::chrono::microseconds(10), [this]{return ready;});
				for(auto& i: data)
				{
					if (!i.second.tasks.empty() && !i.second.handler.expired())
					{
						Data t;
						t.handler = i.second.handler;
						t.tasks = std::move(i.second.tasks);
						d.emplace(i.first, std::move(t));
					}
				}
			}
			
			for(auto& i: d)
			{
				auto& key = i.first;
				if (auto h = i.second.handler.lock())
				{
					while(!i.second.tasks.empty())
					{
						auto value = std::move(i.second.tasks.front());
						i.second.tasks.pop();
						h->Consume(key, value);
					}
				}
			}
		}
	}

private:
	struct Data{
		std::weak_ptr<IConsumer<Key, Value>> handler;
		std::queue<Value> tasks;
	};
	std::map<Key, Data> data;

	std::atomic<bool> running;
	std::mutex mtx;

	std::condition_variable cv;
	bool ready;

	std::thread th;
};