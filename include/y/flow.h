#ifndef Y_FLOW_H
#define Y_FLOW_H

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace y {
#ifdef Y_FLOW_HAVE_THREAD_CHECK_FUNCTION
void flow_thread_check();
#else
inline void flow_thread_check() {}
#endif


class Data;
class Function;
class Consumer;

template<typename T> class Observable;
template<typename ...T> class Observer;
template<typename F, typename ...T> class Reactive;


class Data {
    mutable Consumer * first_consumer_;

public:
    constexpr Data() : first_consumer_(nullptr) {}
    Data(const Data &) = delete;
    void operator=(const Data &) = delete;

    static inline void flush() { notify_change(nullptr); }

protected:
    inline void notify_change() const { notify_change(first_consumer_); }

private:
    static void notify_change(Consumer * consumer);
    static void mark_outdated(const Consumer * consumer);
    static void update_outdated();

    friend Consumer;
    void observe(Consumer & consumer) const;
    void unobserve(Consumer & consumer) const;
};


class Function {
    friend class Data;
    Function * next_outdated_;

public:
    explicit Function(bool skip_initial_evaluation = false);
    Function(const Function &) = delete;
    virtual ~Function();
    void operator=(const Function &) = delete;

    virtual void evaluate() = 0;
};


class Consumer {
    friend class Data;
    Consumer * next_consumer_;
    Function & related_function_;
    const Data & data_;

public:
    Consumer(const Consumer &) = delete;
    void operator=(const Consumer &) = delete;

protected:
    Consumer(const Data & data, Function & related_function);
    ~Consumer();

    [[nodiscard]] const Data & data() const { return data_; }
};


template<typename T>
class Observable : public Data {
    T value_;

public:
    constexpr explicit Observable(T initial_value = T()) : value_(std::move(initial_value)) {}
    template<typename F> Observable<T> & update(F updater) {
        if (updater(value_)) { notify_change(); }
        return *this; }

    operator const T &() const { return get(); }
    const T & get() const { return value_; }
    Observable & operator=(T value) {
        return update([&](T & v){
            auto are_different = value != v;
            v = std::move(value);
            return are_different; });
    }
};


template<typename T>
class Observer<T>: Consumer {
public:
    Observer(const Observable<T> & observable, Function & function)
        : Consumer(observable, function)
    {
    }
    Observer(const Observer &) = delete;

    const Observable<T> & observable() const { return static_cast<const Observable<T> &>(data()); }
    operator const T &() const { return get(); }
    const T & get() const { return observable().get(); }

    template<typename F, typename ...Us>
    void callWithObservableValue(F f, const Us &... args) const
    {
        f(args..., get());
    }
};


template<typename T, typename ...Ts>
class Observer<T, Ts...>: Consumer {
    Observer<Ts...> observer_;

public:
    Observer(const Observable<T> & observable, const Observable<Ts> & ...observables, Function & function)
        : Consumer(observable, function)
        , observer_(observables..., function)
    {
    }

    template<typename F, typename ...Us>
    void callWithObservableValue(F f, const Us &... args) const
    {
        observer_.callWithObservableValue(f, args..., T(static_cast<const Observable<T>&>(data())));
    }
};


template<typename F, typename ...T>
class Reactive: public Function, Observer<T...> {
    F action_;

public:
    explicit Reactive(F action, const Observable<T> & ...observable, const bool skipInitialEvaluation = false)
        : Function(skipInitialEvaluation)
        , Observer<T...>(observable..., *this)
        , action_(action)
    {
    }
    void evaluate() override { Observer<T...>::callWithObservableValue(action_); }
};


template<typename ...T>
using ReactiveFunction = Reactive<std::function<void(const T &...)>, T...>;


template<typename F, typename ...T>
class ReactiveBuilder
{
    F action_;
    const std::tuple<const Observable<T> &...> observables_;
    bool skipInitialEvaluation_ = false;

public:
    constexpr explicit ReactiveBuilder(F action, const Observable<T> & ...observable) : action_(action), observables_(std::ref(observable)...) {}
    constexpr explicit ReactiveBuilder(F action, std::tuple<const Observable<T> &...> observables) : action_(action), observables_(observables) {}
    constexpr Reactive<F, T...> build() { return std::apply([&](auto && ...observable) { return Reactive<F, T...>(action_, observable..., skipInitialEvaluation_); }, observables_); }
    constexpr std::unique_ptr<Reactive<F, T...>> unique() { return std::apply([&](auto && ...observable) { return std::make_unique<Reactive<F, T...>>(action_, observable..., skipInitialEvaluation_); }, observables_); }
    constexpr std::shared_ptr<Reactive<F, T...>> shared() { return std::apply([&](auto && ...observable) { return std::make_shared<Reactive<F, T...>>(action_, observable..., skipInitialEvaluation_); }, observables_); }

    constexpr ReactiveBuilder<F, T...> & skipInitialEvaluation(bool skip = true) { skipInitialEvaluation_ = skip; return *this; }
    constexpr ReactiveBuilder<std::function<void(const T &...)>, T...> asReactiveFunction() { return ReactiveBuilder<std::function<void(const T &...)>, T...>(action_, observables_); }
};


template<typename F, typename ...T>
constexpr ReactiveBuilder<F, T...> reactive(F action, const Observable<T> & ...observable) {
    return ReactiveBuilder<F, T...>(action, observable...);
}


template<typename F, typename ...T>
class CoherentVariable: public Observable<std::invoke_result_t<F, const T &...>> {
    using value_type = std::invoke_result_t<F, const T &...>;

    struct UpdateAction {
        CoherentVariable * self;

        void operator()(const T & ...args) const
        {
            self->refresh(args...);
        }
    };

    F f_;
    Reactive<UpdateAction, T...> reactive_;

public:
    CoherentVariable(F f, const Observable<T> & ...observable)
        : Observable<value_type>(std::invoke(f, observable.get()...))
        , f_(std::move(f))
        , reactive_(UpdateAction{this}, observable..., true)
    {
    }

private:
    void refresh(const T & ...args)
    {
        Observable<value_type>::operator=(std::invoke(f_, args...));
    }
};

} // namespace


#endif //Y_FLOW_H
