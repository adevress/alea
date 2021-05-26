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

#ifndef RANDOM_DERIVATE_IMPL_HPP
#define RANDOM_DERIVATE_IMPL_HPP

#include <array>
#include <bitset>
#include <cstdint>
#include <cstring>

namespace alea {

namespace impl {
/*
template <typename Uint>
inline alea::sha1::digest32_t generate_deterministic_seed_160(Uint origin_seed,
Uint key) {
   // let's add a bit of salt to our meal today
  std::array<std::uint8_t, 16> salt = {
       {0x6c, 0x77, 0xad, 0xb8, 0x3e, 0xf8, 0x21, 0x61, 0xc3, 0xd8, 0x2e, 0x4c,
0x13, 0xfd, 0x75, 0xd3}};

   alea::sha1 sha_compute;
   //  we want to generate a new seed determinitically 'seed = f(old_seed, key)'
   //  the new seed need to be free or any  statistics correlation from old_seed
and key
   //
   // we do
   //   new_seed = sha1( concat(salt, key, origin_seed) )
   //
   // we return then the first 32 bits from the generations
   sha_compute.process_block(salt.begin(), salt.end());
   sha_compute.process(key);
   sha_compute.process(origin_seed);
   return sha_compute.get_digest();
}

*/

} // namespace impl

template <typename Engine>
inline Engine random_engine_derivate(const Engine &engine,
                                     const typename Engine::result_type &key) {
    Engine res(engine);

    typename Engine::result_type old_val = res();
    /*
        // compute a new  seed determinitically
        alea::sha1::digest32_t digest =
       impl::generate_deterministic_seed_160<typename
       Engine::result_type>(old_val, key); typename Engine::result_type
       new_seed; std::memcpy(&new_seed, &(digest[0]), sizeof(typename
       Engine::result_type));

        // use it
        res.seed(new_seed);

        // use the digest to increase entropy even more
        // we get the number of 1 bits in our digest and
        // initialize our random engine from 0 to n
        for (alea::sha1::digest32_t::iterator it = digest.begin(); it <
       digest.end(); ++it) {

            std::bitset<32> bits(*it);
            const std::uint64_t iteration = bits.count();
            for (std::uint64_t i = 0; i < iteration; ++i) {
                (void)res();
            }
        }*/

    return res;
}

} // namespace alea

#endif // RANDOM_DERIVATE_IMPL_HPP
