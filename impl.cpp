#include <iostream>
#include <string>
#include <memory>
#include "impl.h"

using m_std::optional;
using m_std::nullopt;
using m_std::bad_optional_access;

struct Tracker {
    int val;

    Tracker(int v) : val(v) {
        std::cout << "Construct " << val << "\n";
    }

    Tracker(const Tracker& other) : val(other.val) {
        std::cout << "Copy " << val << "\n";
    }

    Tracker(Tracker&& other) noexcept : val(other.val) {
        std::cout << "Move " << val << "\n";
    }

    Tracker& operator=(const Tracker& other) {
        val = other.val;
        std::cout << "Copy assign " << val << "\n";
        return *this;
    }

    Tracker& operator=(Tracker&& other) noexcept {
        val = other.val;
        std::cout << "Move assign " << val << "\n";
        return *this;
    }

    ~Tracker() {
        std::cout << "Destroy " << val << "\n";
    }

    bool operator==(const Tracker& other) const {
        return val == other.val;
    }

    auto operator<=>(const Tracker& other) const = default;
};

int main() {

    std::cout << "==== Basic construction ====\n";
    optional<int> a;
    optional<int> b(10);

    std::cout << "a.has_value(): " << a.has_value() << "\n";
    std::cout << "b.has_value(): " << b.has_value() << "\n";

    std::cout << "\n==== Value access ====\n";
    if (b) std::cout << *b << "\n";

    std::cout << "\n==== Assignment ====\n";
    a = b;
    std::cout << *a << "\n";

    std::cout << "\n==== Reset ====\n";
    a = nullopt;
    std::cout << "a.has_value(): " << a.has_value() << "\n";

    std::cout << "\n==== Emplace ====\n";
    a.emplace(42);
    std::cout << *a << "\n";

    std::cout << "\n==== Swap ====\n";
    optional<int> x(1), y(2);
    x.swap(y);
    std::cout << *x << " " << *y << "\n";

    std::cout << "\n==== Comparisons ====\n";
    optional<int> c(5), d(10), e;
    std::cout << (c < d) << "\n";
    std::cout << (e < c) << "\n";
    std::cout << (c == 5) << "\n";

    std::cout << "\n==== Exception ====\n";
    try {
        optional<int> z;
        std::cout << z.value() << "\n";
    }
    catch (const bad_optional_access& ex) {
        std::cout << "Caught: " << ex.what() << "\n";
    }

    std::cout << "\n==== Non-trivial type ====\n";
    optional<Tracker> t1;
    t1.emplace(100);

    optional<Tracker> t2(t1);
    optional<Tracker> t3(std::move(t1));

    std::cout << "\n==== Assignment (non-trivial) ====\n";
    t2 = t3;

    std::cout << "\n==== Swap (non-trivial) ====\n";
    t2.swap(t3);

    std::cout << "\n==== Move-only type ====\n";
    optional<std::unique_ptr<int>> p1;
    p1.emplace(new int(99));

    optional<std::unique_ptr<int>> p2(std::move(p1));
    if (p2) std::cout << **p2 << "\n";

    std::cout << "\n==== End ====\n";
}