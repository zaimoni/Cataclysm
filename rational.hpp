#ifndef RATIONAL_HPP
#define RATIONAL_HPP 1

// clone std::ratio for runtime use

#include <stdexcept>
#include <compare>
#include <optional>
#include <variant>
#include <utility>

// From Iskandria; relicensed.
inline constexpr unsigned long long gcd(unsigned long long lhs, unsigned long long rhs)
{
	return  0 == rhs ? lhs
		: (0 == lhs ? rhs
			: (1 == rhs ? 1
				: (1 == lhs ? 1
					: (lhs == rhs ? lhs
						: (lhs < rhs ? gcd(lhs, rhs % lhs)
							: gcd(rhs, lhs % rhs))))));
}

class rational
{
	long long _numerator;
	long long _denominator;
public:
	// normal form: denominator positive
	explicit constexpr rational(long long n, long long d = 1) : _numerator(n), _denominator(d) {
		if (0 == _denominator) throw std::logic_error("division by zero");
		if (-(signed long long)((unsigned long long)(-1) / 2) > _numerator) throw std::logic_error("LLONG_MIN numerator unhandled");
		if (-(signed long long)((unsigned long long)(-1) / 2) > _denominator) throw std::logic_error("LLONG_MIN denominator unhandled");
		if (0 == _numerator) {
			_denominator = 1;
			return;
		}
		if (0 > _denominator) {
			_numerator = -_numerator;
			_denominator = -_denominator;
		}
		auto common_divisor = gcd(0 > _numerator ? -_numerator : _numerator, _denominator);
		if (1 < common_divisor) {
			_numerator /= common_divisor;
			_denominator /= common_divisor;
		}
	}
	rational(const rational& src) = default;
	~rational() = default;
	rational& operator=(const rational& src) = default;

	constexpr operator long long() { return _numerator / _denominator; }

	constexpr auto abs_numerator() const { return 0 <= _numerator ? _numerator : -_numerator; }
	constexpr auto numerator() const { return _numerator; }
	constexpr auto denominator() const { return _denominator; }

	constexpr friend rational operator*(const rational& lhs, const rational& rhs) {
		auto lhs_rhs_gcd = gcd(lhs.abs_numerator(), rhs._denominator);
		auto rhs_lhs_gcd = gcd(rhs.abs_numerator(), lhs._denominator);
		auto denominator_test = product(lhs._denominator / rhs_lhs_gcd, rhs._denominator / lhs_rhs_gcd);
		auto new_denominator = std::get_if<unsigned long long>(&denominator_test);
		if (!new_denominator || (unsigned long long)(-1) / 2 < *new_denominator) throw std::runtime_error("rational * overflow: denominator");
		auto numerator_test = product(lhs.abs_numerator() / lhs_rhs_gcd, rhs.abs_numerator() / rhs_lhs_gcd);
		auto new_numerator = std::get_if<unsigned long long>(&numerator_test);
		if (!new_numerator || (unsigned long long)(-1) / 2 < *new_numerator) throw std::runtime_error("rational * overflow: numerator");
		bool is_positive = (0 < lhs._numerator) == (0 < rhs._numerator);
		return rational(is_positive ? *new_numerator : -(signed long long)(*new_numerator), *new_denominator);
	}

	constexpr friend int& operator*=(int& lhs, const rational& rhs) {
		if (0 != lhs) {
			auto lhs_rhs_gcd = gcd(lhs, rhs._denominator);
			auto new_denominator = rhs._denominator / lhs_rhs_gcd;
			long long extended_lhs = lhs;
			unsigned long long abs_lhs = 0 < extended_lhs ? extended_lhs : -extended_lhs;
			unsigned long long abs_lhs_factored = abs_lhs / lhs_rhs_gcd;
			auto numerator_test = product(abs_lhs_factored, rhs.abs_numerator());
			bool is_positive = (0 < lhs) == (0 < rhs._numerator);
			if (auto new_numerator = std::get_if<unsigned long long>(&numerator_test)) {
				unsigned long long ret = *new_numerator / new_denominator;
				if ((unsigned int)(-1) / 2 >= ret) {
					lhs = is_positive ? (int)(ret) : -(int)(ret);
				} else lhs = is_positive ? (int)((unsigned int)(-1) / 2) : -(int)((unsigned int)(-1) / 2); // technically clamped early
			} else throw std::runtime_error("rational * overflow: numerator");
		}
		return lhs;
	}


	constexpr friend std::strong_ordering operator<=>(const rational& lhs, const rational& rhs) {
		if (0 == lhs._numerator && 0 == rhs._numerator) return std::strong_ordering::equal;
		if (0 >= lhs._numerator && 0 <= rhs._numerator) return std::strong_ordering::less;
		if (0 <= lhs._numerator && 0 >= rhs._numerator) return std::strong_ordering::greater;
		// test product, all positive integers: a/b < c/d when a*d < c*b
		if (0 < lhs._numerator) return compare(product(lhs._numerator, rhs._denominator), product(rhs._numerator, lhs._denominator));
		else return compare(product(-rhs._numerator, lhs._denominator), product(-lhs._numerator, rhs._denominator));
	}

private:
	// does not handle division by zero
	// postcondition: either evaluated, or unevaluated with the multiplicands sorted in increasing order
	static constexpr std::variant<unsigned long long, std::pair<unsigned long long, unsigned long long>> product(unsigned long long lhs, unsigned long long rhs) {
		if ((unsigned long long)(-1) / rhs >= lhs) return lhs * rhs;
		if (lhs <= rhs) std::pair(lhs, rhs);
		return std::pair(rhs, lhs);
	}

	static constexpr std::optional<std::strong_ordering> trivial_compare(const std::variant<unsigned long long, std::pair<unsigned long long, unsigned long long>>& lhs, const std::variant<unsigned long long, std::pair<unsigned long long, unsigned long long>>& rhs) {
		if (auto lhs_small = std::get_if<unsigned long long>(&lhs)) {
			if (auto rhs_small = std::get_if<unsigned long long>(&rhs)) return *lhs_small <=> *rhs_small;
			else return std::strong_ordering::less;
		}
		else if (auto rhs_small = std::get_if<unsigned long long>(&rhs)) return std::strong_ordering::greater;
		return std::nullopt;
	}

	static constexpr std::optional<std::strong_ordering> trivial_compare(const std::pair<unsigned long long, unsigned long long>& lhs, const std::pair<unsigned long long, unsigned long long>& rhs) {
		if (lhs.first >= rhs.second) return std::strong_ordering::greater; // cf. postcondition of indirect sole caller's data source
		if (lhs.second >= rhs.first) return std::strong_ordering::less; // cf. postcondition of indirect sole caller's data source
		if (lhs.first == rhs.first) return lhs.second <=> rhs.second;
		if (lhs.second == rhs.second) return lhs.first <=> rhs.first;
		if (lhs.first < rhs.first && lhs.second < rhs.second) return std::strong_ordering::less;
		if (lhs.first > rhs.first && lhs.second > rhs.second) return std::strong_ordering::greater;
		return std::nullopt;
	}

	static constexpr std::strong_ordering compare(std::variant<unsigned long long, std::pair<unsigned long long, unsigned long long>> lhs, std::variant<unsigned long long, std::pair<unsigned long long, unsigned long long>> rhs) {
		if (const auto test = trivial_compare(lhs, rhs)) return *test;

		// both are large; will be non-null
		auto lhs_large = std::get_if<std::pair<unsigned long long, unsigned long long> >(&lhs);
		auto rhs_large = std::get_if<std::pair<unsigned long long, unsigned long long> >(&rhs);
		if (const auto test = trivial_compare(*lhs_large, *rhs_large)) return *test;

		while (true) {
			// all four cases needed due to sorting postcondition of sole caller's data source
			bool adjusted = false;
			if (auto gcd_test = gcd(lhs_large->first, rhs_large->first); 1 < gcd_test) {
				lhs_large->first /= gcd_test;
				rhs_large->first /= gcd_test;
				adjusted = true;
			}
			if (auto gcd_test = gcd(lhs_large->first, rhs_large->second); 1 < gcd_test) {
				lhs_large->first /= gcd_test;
				rhs_large->second /= gcd_test;
				adjusted = true;
			}
			if (auto gcd_test = gcd(lhs_large->second, rhs_large->first); 1 < gcd_test) {
				lhs_large->second /= gcd_test;
				rhs_large->first /= gcd_test;
				adjusted = true;
			}
			if (auto gcd_test = gcd(lhs_large->second, rhs_large->second); 1 < gcd_test) {
				lhs_large->second /= gcd_test;
				rhs_large->second /= gcd_test;
				adjusted = true;
			}

			if (adjusted) {
				// raw pointers die here
				lhs = product(lhs_large->first, lhs_large->second);
				rhs = product(rhs_large->first, rhs_large->second);
				if (const auto test = trivial_compare(lhs, rhs)) return *test;

				// regenerate raw pointers
				lhs_large = std::get_if<std::pair<unsigned long long, unsigned long long> >(&lhs);
				rhs_large = std::get_if<std::pair<unsigned long long, unsigned long long> >(&rhs);
				if (const auto test = trivial_compare(*lhs_large, *rhs_large)) return *test;
			}

			// subtract off common block; cf postcondition for trivial_compare
			if (lhs_large->first < rhs_large->first && lhs_large->second > rhs_large->second) {
				rhs_large->first -= lhs_large->first;
				lhs_large->second -= rhs_large->second;
			} else /* if (lhs_large->first > rhs_large->first && lhs_large->second < rhs_large->second) */ {
				lhs_large->first -= rhs_large->first;
				rhs_large->second -= lhs_large->second;
			}

			// raw pointers die here
			lhs = product(lhs_large->first, lhs_large->second);
			rhs = product(rhs_large->first, rhs_large->second);
			if (const auto test = trivial_compare(lhs, rhs)) return *test;

			// regenerate raw pointers
			lhs_large = std::get_if<std::pair<unsigned long long, unsigned long long> >(&lhs);
			rhs_large = std::get_if<std::pair<unsigned long long, unsigned long long> >(&rhs);
			if (const auto test = trivial_compare(*lhs_large, *rhs_large)) return *test;
		};
	}
};

constexpr rational operator/(const rational& lhs, const rational& rhs) { return lhs * rational(rhs.denominator(), rhs.numerator()); }
constexpr rational operator/(long long lhs, const rational& rhs) { return rational(lhs) * rational(rhs.denominator(), rhs.numerator()); }

static_assert(0 == rational(1) <=> rational(1));
static_assert(1 == rational(1).numerator());
static_assert(1 == rational(1).denominator());
static_assert(1 == rational(1));

#endif
