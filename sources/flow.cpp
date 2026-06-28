#include <y/flow.h>

using namespace y;


static Function * first_outdated;
static Function * never_evaluated;


void Data::notify_change(Consumer * consumer)
{
    flow_thread_check();
    const auto is_already_notifying = first_outdated != nullptr;
    mark_outdated(consumer);
    if (!is_already_notifying) {
        update_outdated();
    }
}


void Data::mark_outdated(const Consumer * consumer)
{
    Function ** outdated = &first_outdated;
    while (*outdated) { outdated = &(*outdated)->next_outdated_; }

    *outdated = never_evaluated;
    while (*outdated) { outdated = &(*outdated)->next_outdated_; }
    never_evaluated = nullptr;

    for (; consumer; consumer = consumer->next_consumer_) {
        if (const auto is_already_outdated = consumer->related_function_.next_outdated_ || outdated == &consumer->related_function_.next_outdated_; !is_already_outdated) {
            *outdated = &consumer->related_function_;
            outdated = &(*outdated)->next_outdated_;
        }
    }
}


void Data::update_outdated()
{
    while (first_outdated) {
        const auto outdated = first_outdated;
        const auto next = first_outdated->next_outdated_;
        first_outdated->next_outdated_ = nullptr;
        first_outdated = next;
        outdated->evaluate();
    }
}


void Data::observe(Consumer & consumer) const
{
    flow_thread_check();
    Consumer** next = &first_consumer_;
    while (*next) {
        next = &(*next)->next_consumer_;
    }
    *next = &consumer;
}


void Data::unobserve(Consumer & consumer) const
{
    flow_thread_check();
    for (Consumer** next = &first_consumer_; *next; next = &(*next)->next_consumer_) {
        if (*next == &consumer) {
            *next = consumer.next_consumer_;
            consumer.next_consumer_ = nullptr;
            return;
        }
    }
}


Function::Function(bool skip_initial_evaluation)
    : next_outdated_(nullptr)
{
    if (skip_initial_evaluation) { return; }
    Function ** outdated = &never_evaluated;
    while (*outdated) { outdated = &(*outdated)->next_outdated_; }
    *outdated = this;
}


Function::~Function()
{
    for (Function** next = &first_outdated; *next; next = &(*next)->next_outdated_) {
        if (*next == this) {
            *next = next_outdated_;
            next_outdated_ = nullptr;
            return;
        }
    }
    for (Function** next = &never_evaluated; *next; next = &(*next)->next_outdated_) {
        if (*next == this) {
            *next = next_outdated_;
            next_outdated_ = nullptr;
            return;
        }
    }
}


Consumer::Consumer(const Data & data, Function & related_function)
    : next_consumer_(nullptr)
    , related_function_(related_function)
    , data_(data)
{
    data.observe(*this);
}


Consumer::~Consumer()
{
    data_.unobserve(*this);
}