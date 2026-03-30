#pragma once
#include <compare>
#include <type_traits>
#include <exception>
#include <new>      
#include <memory>
#include <initializer_list> 
#include <utility>      

//However, in C++20, placement new is strictly forbidden inside constant expressions.
//So, change all occurences of placement new to std::construct_at. 
//This rule is changed in C++26, so placement new will now work in constexpr functions at compile time.

namespace m_std {
	// 20.6.3, class template optional
	template<class T>
	class optional;

	// 20.6.4, no-value state indicator
	struct nullopt_t {
		constexpr nullopt_t(int) {}
	};
	constexpr nullopt_t nullopt(0);

	// Fixed in_place_t to be instantiable
	struct in_place_t { explicit in_place_t() = default; };
	inline constexpr in_place_t in_place{};

	// 20.6.5, class bad_optional_access()
	class bad_optional_access : public std::exception {
	public:
		const char* what() const noexcept override {
			return "Accessing empty optional";
		}
	};

	// 20.6.6, relational operators
	template<class T, class U>
	constexpr bool operator==(const optional<T>& x, const optional<U>& y) {
		if (!bool(x) && (bool(x) == bool(y))) return true;
		return (bool(x) == bool(y)) ? (*x == *y) : false;
	}
	template<class T, class U>
	constexpr bool operator!=(const optional<T>& x, const optional<U>& y) {
		if (!bool(x) && (bool(x) == bool(y))) return false;
		return (bool(x) == bool(y)) ? (*x != *y) : true;
	}
	template<class T, class U>
	constexpr bool operator<(const optional<T>& x, const optional<U>& y) {
		if (!y) return false;
		if (!x) return true;
		return *x < *y;
	}
	template<class T, class U>
	constexpr bool operator>(const optional<T>& x, const optional<U>& y) {
		if (!x) return false;
		if (!y) return true;
		return *x > *y;
	}
	template<class T, class U>
	constexpr bool operator<=(const optional<T>& x, const optional<U>& y) {
		if (!x) return true;
		if (!y) return false;
		return *x <= *y;
	}
	template<class T, class U>
	constexpr bool operator>=(const optional<T>& x, const optional<U>& y) {
		if (!y) return true;
		if (!x) return false;
		return *x >= *y;
	}
	template<class T, std::three_way_comparable_with<T> U>
	constexpr std::compare_three_way_result_t<T, U>
		operator<=>(const optional<T>& x, const optional<U>& y) {
		if (x && y) return *x <=> *y;
		return bool(x) <=> bool(y);
	}

	// 20.6.7, comparison with nullopt
	template<class T> constexpr bool operator==(const optional<T>& x, nullopt_t) noexcept {
		return !x;
	}
	template<class T>
	constexpr std::strong_ordering operator<=>(const optional<T>& x, nullopt_t) noexcept {
		return bool(x) <=> false;
	}

	// 20.6.8, comparison with T
	template<class T, class U> constexpr bool operator==(const optional<T>& x, const U& v) { return bool(x) ? *x == v : false; }
	template<class T, class U> constexpr bool operator==(const T& v, const optional<U>& x) { return bool(x) ? v == *x : false; }
	template<class T, class U> constexpr bool operator!=(const optional<T>& x, const U& v) { return bool(x) ? *x != v : true; }
	template<class T, class U> constexpr bool operator!=(const T& v, const optional<U>& x) { return bool(x) ? v != *x : true; }
	template<class T, class U> constexpr bool operator<(const optional<T>& x, const U& v) { return bool(x) ? *x < v : true; }
	template<class T, class U> constexpr bool operator<(const T& v, const optional<U>& x) { return bool(x) ? v < *x : false; }
	template<class T, class U> constexpr bool operator>(const optional<T>& x, const U& v) { return bool(x) ? *x > v : false; }
	template<class T, class U> constexpr bool operator>(const T& v, const optional<U>& x) { return bool(x) ? v > *x : true; }
	template<class T, class U> constexpr bool operator<=(const optional<T>& x, const U& v) { return bool(x) ? *x <= v : true; }
	template<class T, class U> constexpr bool operator<=(const T& v, const optional<U>& x) { return bool(x) ? v <= *x : false; }
	template<class T, class U> constexpr bool operator>=(const optional<T>& x, const U& v) { return bool(x) ? *x >= v : false; }
	template<class T, class U> constexpr bool operator>=(const T& v, const optional<U>& x) { return bool(x) ? v >= *x : true; }
	template<class T, std::three_way_comparable_with<T> U>
	constexpr std::compare_three_way_result_t<T, U>
		operator<=>(const optional<T>& x, const U& v) {
		return bool(x) ? *x <=> v : std::strong_ordering::less; // Added std::
	}

	// 20.6.9, specialized algorithms
	template<class T>
	void swap(optional<T>& x, optional<T>& y) noexcept(noexcept(x.swap(y)))
		requires(std::is_move_constructible_v<T>&& std::is_swappable_v<T>) {
		x.swap(y);
	}
	template<class T>
	constexpr optional<std::decay_t<T>> make_optional(T&& v) {
		return optional<std::decay_t<T>>(std::forward<T>(v));
	}
	template<class T, class... Args>
	constexpr optional<T> make_optional(Args&&... args) {
		return optional<T>(in_place, std::forward<Args>(args)...);
	}
	template<class T, class U, class... Args>
	constexpr optional<T> make_optional(std::initializer_list<U> il, Args&&... args) {
		return optional<T>(in_place, il, std::forward<Args>(args)...);
	}

	template<class T>
	class optional {
	public:
		// 20.6.3.1, constructors
		constexpr optional() noexcept : m_val{ nullptr }, m_isValid{ false } {}
		constexpr optional(nullopt_t) noexcept : m_val{ nullptr }, m_isValid{ false } {}
		constexpr optional(const optional& rhs)
			requires(std::is_copy_constructible_v<T>) {
			if (!bool(rhs)) {
				m_isValid = false; m_val = nullptr;
			}
			else {
				//m_val = new (std::addressof(m_union.val)) T(*rhs);
				m_val = std::construct_at(std::addressof(m_union.val), *rhs);
				m_isValid = true;
			}
		}
		constexpr optional(optional&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
			requires(std::is_move_constructible_v<T>) {
			if (!bool(rhs)) {
				m_val = nullptr; m_isValid = false;
			}
			else {
				//m_val = new (std::addressof(m_union.val)) T(std::move(*rhs));
				m_val = std::construct_at(std::addressof(m_union.val), std::move(*rhs));
				m_isValid = true;
			}
		}
		template<class... Args>
		constexpr explicit optional(in_place_t, Args&&... args)
			requires(std::is_constructible_v<T, Args...>) {
			//m_val = new (std::addressof(m_union.val)) T(std::forward<Args>(args)...);
			m_val = std::construct_at(std::addressof(m_union.val), std::forward<Args>(args)...);
			m_isValid = true;
		}
		template<class U, class... Args>
		constexpr explicit optional(in_place_t, std::initializer_list<U> il, Args&&... args)
			requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>) {
			//m_val = new (std::addressof(m_union.val)) T(il, std::forward<Args>(args)...);
			m_val = std::construct_at(std::addressof(m_union.val),il,std::forward<Args>(args)...);
			m_isValid = true;
		}
		template<class U = T>
		constexpr explicit(!std::is_convertible_v<U, T>) optional(U&& rhs)
			requires(std::is_constructible_v<T, U> && !std::is_same_v<std::remove_cvref_t<U>, in_place_t> && !std::is_same_v<std::remove_cvref_t<U>, optional>) {
			//m_val = new (std::addressof(m_union.val)) T(std::forward<U>(rhs));
			m_val = std::construct_at(std::addressof(m_union.val), std::forward<U>(rhs));
			m_isValid = true;
		}
		template<class U>
		explicit(!std::is_convertible_v<const U&, T>) optional(const optional<U>& rhs)
			requires(std::is_constructible_v<T, const U&> && !std::is_constructible_v<T, optional<U>&> && !std::is_constructible_v<T, optional<U>&&> && !std::is_constructible_v<T, const optional<U>&> && !std::is_constructible_v<T, const optional<U>&&> && !std::is_constructible_v<optional<U>&, T> && !std::is_constructible_v<optional<U>&&, T> && !std::is_constructible_v<const optional<U>&, T> && !std::is_constructible_v<const optional<U>&&, T>) {
			if (!bool(rhs)) {
				m_val = nullptr; m_isValid = false;
			}
			else {
				//m_val = new (std::addressof(m_union.val)) T(*rhs);
				m_val = std::construct_at(std::addressof(m_union.val), *rhs);
				m_isValid = true;
			}
		}
		template<class U>
		explicit(!std::is_convertible_v<U, T>) optional(optional<U>&& rhs)
			requires(std::is_constructible_v<T, U> && !std::is_constructible_v<T, optional<U>&> && !std::is_constructible_v<T, optional<U>&&> && !std::is_constructible_v<T, const optional<U>&> && !std::is_constructible_v<T, const optional<U>&&> && !std::is_constructible_v<optional<U>&, T> && !std::is_constructible_v<optional<U>&&, T> && !std::is_constructible_v<const optional<U>&, T> && !std::is_constructible_v<const optional<U>&&, T>) {
			if (!bool(rhs)) {
				m_val = nullptr; m_isValid = false;
			}
			else {
				//m_val = new (std::addressof(m_union.val)) T(std::move(*rhs));
				m_val = std::construct_at(std::addressof(m_union.val), std::move(*rhs));
				m_isValid = true;
			}
		}

		// 20.6.3.2, destructor
		~optional() {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				if (bool(*this)) m_val->~T();
			}
		}

		// 20.6.3.3, assignment
		optional& operator=(nullopt_t) noexcept {
			if (bool(*this)) {
				if constexpr (!std::is_trivially_destructible_v<T>) {
					m_val->~T();
				}
				m_isValid = false;
			}
			return *this;
		}

		//exception safety!!!
		constexpr optional& operator=(const optional& rhs)
			requires (std::is_copy_constructible_v<T>&& std::is_copy_assignable_v<T>) {
			if (bool(*this)) {
				if (bool(rhs)) *m_val = *rhs;
				else {
					if constexpr (!std::is_trivially_destructible_v<T>) {
						m_val->~T();
					}
					m_isValid = false;
				}
			}
			else {
				if (bool(rhs)) {
					//m_val = new (std::addressof(m_union.val)) T(*rhs);
					m_val = std::construct_at(std::addressof(m_union.val), *rhs);
					m_isValid = true;
				}
			}
			return *this;
		}

		constexpr optional& operator=(optional&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>&& std::is_nothrow_move_constructible_v<T>)
			requires(std::is_move_constructible_v<T>&& std::is_move_assignable_v<T>) {
			if (bool(*this)) {
				if (bool(rhs)) *m_val = std::move(*rhs);
				else {
					if constexpr (!std::is_trivially_destructible_v<T>) {
						m_val->~T();
					}
					m_isValid = false;
				}
			}
			else {
				if (bool(rhs)) {
					//m_val = new (std::addressof(m_union.val)) T(std::move(*rhs));
					m_val = std::construct_at(std::addressof(m_union.val), std::move(*rhs));
					m_isValid = true;
				}
			}
			return *this;
		}

		template<class U = T> optional& operator=(U&& v)
			requires(!std::is_same_v<std::remove_cvref_t<U>, optional> && !std::conjunction_v<std::is_scalar<T>,
		std::is_same<T, std::decay_t<U>>>&& std::is_constructible_v<T, U>&& std::is_assignable_v<T&, U>) {
			if (bool(*this)) *m_val = std::forward<U>(v);
			else {
				//m_val = new (std::addressof(m_union.val)) T(std::forward<U>(v));
				m_val = std::construct_at(std::addressof(m_union.val), std::forward<U>(v));
				m_isValid = true;
			}
			return *this;
		}

		template<class U> optional& operator=(const optional<U>& rhs)
			requires(std::is_constructible_v<T, const U&>&& std::is_assignable_v<T&, const U&> && !std::is_constructible_v<T, optional<U>&> && !std::is_constructible_v<T, optional<U>&&> && !std::is_constructible_v<T, const optional<U>&> && !std::is_constructible_v<T, const optional<U>&&> && !std::is_convertible_v<optional<U>&, T> && !std::is_convertible_v<optional<U>&&, T> && !std::is_convertible_v<const optional<U>&, T> && !std::is_convertible_v<const optional<U>&&, T> && !std::is_assignable_v<T&, optional<U>&> && !std::is_assignable_v<T&, optional<U>&&> && !std::is_assignable_v<T&, const optional<U>&> && !std::is_assignable_v<T&, const optional<U>&&>) {
			if (bool(*this)) {
				if (bool(rhs)) *m_val = *rhs;
				else {
					if constexpr (!std::is_trivially_destructible_v<T>) {
						m_val->~T();
					}
					m_isValid = false;
				}
			}
			else {
				if (bool(rhs)) {
					//m_val = new (std::addressof(m_union.val)) T(*rhs);
					m_val = std::construct_at(std::addressof(m_union.val), *rhs);
					m_isValid = true;
				}
			}
			return *this;
		}

		template<class U> optional& operator=(optional<U>&& rhs)
			requires(std::is_constructible_v<T, U>&& std::is_assignable_v<T&, U> && !std::is_constructible_v<T, optional<U>&> && !std::is_constructible_v<T, optional<U>&&> && !std::is_constructible_v<T, const optional<U>&> && !std::is_constructible_v<T, const optional<U>&&> && !std::is_convertible_v<optional<U>&, T> && !std::is_convertible_v<optional<U>&&, T> && !std::is_convertible_v<const optional<U>&, T> && !std::is_convertible_v<const optional<U>&&, T> && !std::is_assignable_v<T&, optional<U>&> && !std::is_assignable_v<T&, optional<U>&&> && !std::is_assignable_v<T&, const optional<U>&> && !std::is_assignable_v<T&, const optional<U>&&>) {
			if (bool(*this)) {
				if (bool(rhs)) *m_val = std::move(*rhs);
				else {
					if constexpr (!std::is_trivially_destructible_v<T>) {
						m_val->~T();
					}
					m_isValid = false;
				}
			}
			else {
				if (bool(rhs)) {
					//m_val = new (std::addressof(m_union.val)) T(std::move(*rhs));
					m_val = std::construct_at(std::addressof(m_union.val), *rhs);
					m_isValid = true;
				}
			}
			return *this;
		}

		template<class... Args> T& emplace(Args&&... args)
			requires(std::is_constructible_v<T, Args...>) {
			*this = nullopt;
			//m_val = new (std::addressof(m_union.val)) T(std::forward<Args>(args)...);
			m_val = std::construct_at(std::addressof(m_union.val), std::forward<Args>(args)...);
			m_isValid = true;
			return *m_val;
		}

		template<class U, class... Args> T& emplace(std::initializer_list<U> il, Args&&... args)
			requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>) {
			*this = nullopt;
			//m_val = new (std::addressof(m_union.val)) T(il, std::forward<Args>(args)...);
			m_val = std::construct_at(std::addressof(m_union.val), il ,std::forward<Args>(args)...);
			m_isValid = true;
			return *m_val;
		}

		// 20.6.3.4, swap
		void swap(optional& rhs) noexcept(std::is_nothrow_move_constructible_v<T>&& std::is_nothrow_swappable_v<T>)
			requires(std::is_move_constructible_v<T>) {
			using std::swap;
			if (bool(*this)) {
				if (bool(rhs)) swap(*(*this), *rhs);
				else {
					rhs.emplace(std::move(*(*this)));
					if constexpr (!std::is_trivially_destructible_v<T>) {
						m_val->~T();
					}
					m_isValid = false;
				}
			}
			else {
				if (bool(rhs)) {
					this->emplace(std::move(*rhs));
					rhs = nullopt;
				}
			}
		}

		// 20.6.3.5, observers
		constexpr const T* operator->() const {
			if (!m_isValid) throw bad_optional_access();
			return m_val;
		}
		constexpr T* operator->() {
			if (!m_isValid) throw bad_optional_access();
			return m_val;
		}
		constexpr const T& operator*() const& {
			if (!m_isValid) throw bad_optional_access();
			return *m_val;
		}
		constexpr T& operator*()& {
			if (!m_isValid) throw bad_optional_access();
			return *m_val;
		}
		constexpr T&& operator*()&& {
			if (!m_isValid) throw bad_optional_access();
			return std::move(*m_val);
		}
		constexpr const T&& operator*() const&& {
			if (!m_isValid) throw bad_optional_access();
			return std::move(*m_val);
		}
		constexpr explicit operator bool() const noexcept {
			return m_isValid;
		}
		constexpr bool has_value() const noexcept {
			return m_isValid;
		}
		constexpr const T& value() const& {
			if (!m_isValid) throw bad_optional_access();
			return *m_val;
		}
		constexpr T& value()& {
			if (!m_isValid) throw bad_optional_access();
			return *m_val;
		}
		constexpr T&& value()&& {
			if (!m_isValid) throw bad_optional_access();
			return std::move(*m_val);
		}
		constexpr const T&& value() const&& {
			if (!m_isValid) throw bad_optional_access();
			return std::move(*m_val);
		}
		template<class U> constexpr T value_or(U&& v) const&
			requires(std::is_copy_constructible_v<T>&& std::is_convertible_v<U&&, T>) {
			return bool(*this) ? **this : static_cast<T>(std::forward<U>(v));
		}
		template<class U> constexpr T value_or(U&& v) &&
			requires(std::is_move_constructible_v<T>&& std::is_convertible_v<U&&, T>) {
			return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(v));
		}

		// 20.6.3.6, modifiers
		void reset() noexcept {
			*this = nullopt;
		}

	private:
		T* m_val; // exposition only
		union Storage {
			char dummy;
			T val;
			constexpr Storage() : dummy{} {}
			constexpr ~Storage() {}
		} m_union;
		bool m_isValid;
	};

	template<class T>
	optional(T) -> optional<T>;
}