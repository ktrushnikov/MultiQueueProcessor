// I did not make some UnitTests (like using google test/mock), just some cases at main func()

#include "MultiQueueProcessor.h"

#include <iostream>

template<typename Key, typename Value>
struct PrintConsumer: IConsumer<Key, Value>
{
	void Consume(Key id, const Value &value) override
	{
		std::cout << "Consumed id=" << id << ", value=" << value << std::endl;
	}
};

int main()
{
    MultiQueueProcessor<int, int> m;
    
    // enqueue before subscribe; should not miss them
    m.Enqueue(1, 1);
    m.Enqueue(1, 2);
    m.Enqueue(2, 3);
    m.Enqueue(2, 4);

    auto pc = std::make_shared<PrintConsumer<int,int>>();
    m.Subscribe(1, pc);
    m.Subscribe(2, pc);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    // enqueue when have consumers
    m.Enqueue(1, 5);
    m.Enqueue(2, 6);

    // enqueue with key, that has no consumer
    m.Enqueue(3, 7);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    m.Unsubscribe(1);
    m.Enqueue(1, 8);    // will not be processed, because not subscribed
    m.Enqueue(2, 9);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}