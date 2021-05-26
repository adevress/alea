/**
 * Copyright (c) 2021, Adrien Devresse <adev@adev.name>
 *
 * Boost Software License - Version 1.0
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef _ALEA_COUNTER_ENGINE_HPP_
#define _ALEA_COUNTER_ENGINE_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

///
/// This work is inspired of algorithm
///  presented in the publication
/// "Parallel random numbers: as easy as 1, 2, 3".
///    John K. Salmon, Mark A. Moraes, Ron O. Dror, David E. Shaw"
///    (doi:10.1145/2063384.2063405)
///
///  It is freely inspired of the Boost.Random123
///  (https://github.com/DEShawResearch/Random123-Boost ) and of the original
///  random123 package distribution
///
///  counter_engine offers an interface compatible with both C++11 random engine
///  and Boost.Random
///
///  available cbrng backend  are : threefy
///

#include <random>

namespace alea {

template <typename CBRNG> class counter_engine {
  public:
    typedef CBRNG cbrng_type;
    typedef typename CBRNG::domain_type ctr_type;
    typedef typename CBRNG::key_type key_type;
    typedef typename ctr_type::value_type result_type;
    typedef size_t elem_type;

    explicit counter_engine(const key_type &uk) : b(uk), c(), elem() {}

    explicit counter_engine(key_type &uk) : b(uk), c(), elem() {}

    explicit counter_engine() : b(), c(), elem() {}

    explicit counter_engine(result_type r) : b(), c(), elem() {
        key_type key;
        std::fill(key.begin(), key.end(), typename key_type::value_type(r));
        b.set_key(key);
    }

    explicit counter_engine(std::seed_seq &seq) : b(), c(), elem() {
        key_type key;

        seq.generate(key.begin(), key.end());
        b.set_key(key);
    }

    counter_engine(const counter_engine &) = default;

    counter_engine(counter_engine &&) = default;

    counter_engine &operator=(const counter_engine &) = default;

    counter_engine &operator=(counter_engine &&) = default;

    void seed(result_type r) { *this = counter_engine(r); }

    template <typename SeedSeq> void seed(SeedSeq &s) {
        *this = counter_engine(s);
    }

    void seed() { *this = counter_engine(); }

    void seed(const key_type &uk) { *this = counter_engine(uk); }

    void seed(key_type &uk) { *this = counter_engine(uk); }

    friend bool operator==(const counter_engine &lhs,
                           const counter_engine &rhs) {
        return lhs.c == rhs.c && lhs.elem == rhs.elem && lhs.b == rhs.b;
    }

    friend bool operator!=(const counter_engine &lhs,
                           const counter_engine &rhs) {
        return lhs.c != rhs.c || lhs.elem != rhs.elem || lhs.b != rhs.b;
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    const counter_engine &be) {
        return os << be.c << " " << be.b.getkey() << " " << be.elem;
    }

    const static result_type _min = 0;
    const static result_type _max = ~((result_type)0);

    static constexpr result_type min() { return _min; }

    static constexpr result_type max() { return _max; }

    result_type operator()() {
        if (elem == 0) {
            incr_array(c.begin(), c.end());
            v = b(c);
            elem = c.size();
        }
        return v[--elem];
    }

    result_type generate() { return (*this)(); }

    ctr_type generate_block() {
        elem = 0;
        incr_array(c.begin(), c.end());
        return b(c);
    }

    void discard(std::uintmax_t skip) {
        // any buffered turn need to be dropped
        while (elem != 0 && skip > 0) {
            skip--;
            elem--;
        }
        const size_t nelem = c.size();
        std::uintmax_t counter_increment = skip / nelem;
        std::uintmax_t counter_rest = skip % nelem;
        incr_array(c.begin(), c.end(), counter_increment);

        // call generator for remaining turns
        while (counter_rest--) {
            (void)(*this)();
        }
    }

    counter_engine<cbrng_type> derivate(const key_type &key) const {
        // for counter engine, derivate need to return a unique counter
        // from a tuple <old_counter_state, old_key, new_key>

        // to achive this we rely on the block cipher properties
        // of the counter based random generators
        // new_key = cipher_block(key, cipher_block(old_key, old_counter_state))

        counter_engine<cbrng_type> derivate_counter(*this);
        // call the new counter with the old key and old counter value
        // to get a value function of the counter state and the counter key
        (void)derivate_counter();

        // now we setup the new key
        derivate_counter.b.set_key(key);

        // do a simple rotation based on the elem value
        // to take into consideration "elem" without
        std::rotate(derivate_counter.v.begin(),
                    derivate_counter.v.begin() + elem,
                    derivate_counter.v.end());

        // and using previous rotate generated block as element
        key_type new_key = derivate_counter.b(derivate_counter.v);
        // use the new key as counter
        derivate_counter.seed(new_key);

        return derivate_counter;
    }

    counter_engine<cbrng_type> derivate(result_type r) const {
        key_type key;
        std::fill(key.begin(), key.end(), typename key_type::value_type(r));
        return derivate(key);
    }

    ctr_type operator()(const ctr_type &c) const { return b(c); }

    key_type getseed() const { return c.get_key(); }

    ctr_type getcounter() const { return c; }

  private:
    template <typename Iterator>
    inline void incr_array(Iterator start, Iterator finish) {
        static const typename cbrng_type::uint_type max_elem =
            std::numeric_limits<typename cbrng_type::uint_type>::max();

        while (start != finish) {
            if (*start == max_elem) {
                *start = 0;
                start++;
            } else {
                *start += 1;
                break;
            }
        }
    }

    template <typename Iterator>
    void incr_array(Iterator start, Iterator finish, std::uintmax_t inc_val) {
        static const typename cbrng_type::uint_type max_elem =
            std::numeric_limits<typename cbrng_type::uint_type>::max();

        if (inc_val == 0 || start == finish) {
            return;
        }

        std::uintmax_t current_inc_val = inc_val & max_elem;
        const std::uintmax_t next_inc_val =
            ((sizeof(std::uintmax_t) != sizeof(typename cbrng_type::uint_type))
                 ? (inc_val >> (sizeof(max_elem) * 8))
                 : 0);

        const typename cbrng_type::uint_type past_val = *start;
        *start += current_inc_val;

        if (*start < past_val) { // overflow occured, inc +1 next elem
            incr_array(start + 1, finish, next_inc_val + 1);
        } else {
            incr_array(start + 1, finish, next_inc_val);
        }
    }

    cbrng_type b;
    ctr_type c;
    elem_type elem;
    ctr_type v;
};

// specialize random_engine_derivate
// for counter base random generator
template <typename CBRNG>
inline counter_engine<CBRNG>
random_engine_derivate(const counter_engine<CBRNG> &engine,
                       const typename counter_engine<CBRNG>::result_type &key) {
    return engine.derivate(key);
}

} // namespace alea

#endif
