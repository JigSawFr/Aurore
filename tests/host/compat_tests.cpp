// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Host tests for aurore/compat: move_only_function holds what a
// std::function cannot copy, generator yields lazily and from another range,
// stacktrace says whether it is there. Built twice: with the standard
// library's where it has them, and with AURORE_COMPAT_FALLBACK, the bundled
// ones (compat_fallback).
#include <memory>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.h"

#include <aurore/compat/functional.hpp>
#include <aurore/compat/generator.hpp>
#include <aurore/compat/stacktrace.hpp>

static void test_move_only_function()
{
    aurore::move_only_function<int(int)> f = [owned = std::make_unique<int>(40)](int n)
    { return *owned + n; };
    CHECK(f);
    CHECK(f(2) == 42);
    aurore::move_only_function<int(int)> g = std::move(f);
    CHECK(g(1) == 41);
    aurore::move_only_function<void()> empty;
    CHECK(!empty);
}

static aurore::generator<int> count(int n, int* made)
{
    for (int i = 0; i < n; i++)
    {
        ++*made;
        co_yield i;
    }
}

static aurore::generator<const std::string&> words()
{
    co_yield "one";
    co_yield std::string("two");
}

static aurore::generator<int> nested(int* made)
{
    co_yield -1;
    co_yield aurore::elements_of(count(3, made));
    co_yield 99;
}

static void test_generator()
{
    static_assert(std::ranges::input_range<aurore::generator<int>>);
    int made = 0;
    auto g   = count(5, &made);
    CHECK(made == 0); // nothing runs before the first read
    int sum = 0;
    for (int v : g)
    {
        sum += v;
        if (v == 2)
            break;
    }
    CHECK(sum == 3);
    CHECK(made == 3); // and nothing after the last

    std::vector<std::string> seen;
    for (const std::string& w : words())
        seen.push_back(w);
    CHECK((seen == std::vector<std::string> { "one", "two" }));

    made = 0;
    std::vector<int> all;
    for (int v : nested(&made))
        all.push_back(v);
    CHECK((all == std::vector<int> { -1, 0, 1, 2, 99 }));
}

// Only the type: calling current() needs -lstdc++exp with GCC 13 and 14
static void test_stacktrace()
{
    static_assert(AURORE_HAS_STACKTRACE == 0 || AURORE_HAS_STACKTRACE == 1);
#if AURORE_HAS_STACKTRACE
    CHECK((std::is_same_v<aurore::stacktrace, std::stacktrace>));
#endif
}

int main()
{
    test_move_only_function();
    test_generator();
    test_stacktrace();
#ifdef AURORE_COMPAT_FALLBACK
    return CHECK_DONE("compat (bundled): move_only_function, generator");
#else
    return CHECK_DONE("compat: move_only_function, generator, stacktrace");
#endif
}
