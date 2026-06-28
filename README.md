# Flow

## Why

Reactivity is useful when one part of the code produces values and several other parts must react to those changes without being tightly coupled to the producer.

In embedded code, that often means:

- inputs and measurements update shared state,
- transformations derive new values from that state,
- outputs publish or display the final result.

`flow` provides a small reactive core for that use case. The goal is not to compete with larger frameworks such as RxCpp, but to offer a lightweight alternative that fits better in constrained systems.

## Core concepts

### `y::Observable<T>`

Wrap a value in `y::Observable<T>` to make it observable:

```c++
int plainValue = 0;
// becomes
y::Observable<int> value = 0;
```

You can read it like a regular value:

```c++
int copy = value;
int copy2 = value.get();
```

You can assign to it like a regular variable:

```c++
value = 5;
```

Observers are notified only when the value actually changes.

For in-place mutations, `update` lets you modify the stored value and declare whether that mutation should trigger notifications:

```c++
value.update([](int & current) {
    ++current;
    return true;
});
```

### `y::reactive(...)`

The `reactive` helper creates a builder for a reaction. A reaction subscribes to one or more observables and calls a function with their current values.

```c++
y::Observable<int> value = 0;

auto reaction = y::reactive(
    [](int currentValue) {
        doSomethingUsefulWhenVariableChanges(currentValue);
    },
    value
).build();
```

The reaction must stay alive as long as you want the subscription to remain active.

## Builder API

`y::reactive(...)` returns a `ReactiveBuilder`, which exposes the main construction options:

```c++
auto builder = y::reactive(
    [](int a, float b) {
        consume(a, b);
    },
    observableA,
    observableB
);
```

### `build()`

Builds the reactive object directly, with automatic storage decided by the caller:

```c++
auto reaction = builder.build();
```

### `unique()`

Builds a `std::unique_ptr`:

```c++
auto reaction = builder.unique();
```

### `shared()`

Builds a `std::shared_ptr`:

```c++
auto reaction = builder.shared();
```

### `skipInitialEvaluation(bool = true)`

Controls whether the first flush should execute the reaction immediately after subscription.

```c++
auto reaction = y::reactive(
    [](int currentValue) { consume(currentValue); },
    value
).skipInitialEvaluation().build();
```

### `asReactiveFunction()`

Converts the builder to a builder backed by `std::function`, which can be useful when a concrete function wrapper type is required:

```c++
auto reaction = y::reactive(
    [](int currentValue) { consume(currentValue); },
    value
).asReactiveFunction().build();
```

## Evaluation model

Reactions are scheduled when an observable changes. They are then evaluated by the flow engine.

The tests illustrate one important detail: a freshly created reaction is not executed immediately by default. You can force pending reactions to run with:

```c++
y::Data::flush();
```

Example:

```c++
y::Observable<int> value = 1024;
int replica = 0;

auto reaction = y::reactive(
    [&replica](int currentValue) {
        replica = currentValue;
    },
    value
).build();

// No evaluation yet
assert(replica == 0);

y::Data::flush();
assert(replica == 1024);

value = 1;
assert(replica == 1);
```

## Multiple observables

A reaction can depend on several observables. The callback receives their values in the same order:

```c++
y::Observable<int> count = 1024;
y::Observable<float> ratio = 1.5f;

auto reaction = y::reactive(
    [](int currentCount, float currentRatio) {
        consume(currentCount, currentRatio);
    },
    count,
    ratio
).build();
```

## Build

The project uses CMake and requires C++17.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Tests use GoogleTest and the library target is `flow`.
