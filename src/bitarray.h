#ifndef BITARRAY_H
#define BITARRAY_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

class bit_array
{
	unsigned m_n_bits;
	int m_n_elts;
	uint64_t *m_bits;
	uint64_t m_last_mask;

public:
	bit_array (unsigned sz, bool set = false)
		: m_n_bits (sz), m_n_elts ((sz + 63) / 64), m_bits (new uint64_t[m_n_elts]())
	{
		uint64_t mask = 0;
		mask = ~mask;
		mask >>= 63 - (sz + 63) % 64;
		m_last_mask = mask;

		if (set && m_n_elts > 0) {
			memset (m_bits, -1, m_n_elts * sizeof (uint64_t));
			m_bits[m_n_elts - 1] &= m_last_mask;
		}
	}
	bit_array (const bit_array &other)
		: m_n_bits (other.m_n_bits), m_n_elts (other.m_n_elts), m_bits (new uint64_t[m_n_elts]), m_last_mask (other.m_last_mask)
	{
		memcpy (m_bits, other.m_bits, m_n_elts * sizeof (uint64_t));
	}
	bit_array (bit_array &&other)
		: m_n_bits (other.m_n_bits), m_n_elts (other.m_n_elts), m_bits (other.m_bits), m_last_mask (other.m_last_mask)
	{
		other.m_bits = 0;
		other.m_n_elts = 0;
		other.m_bits = 0;
	}
	~bit_array ()
	{
		delete[] m_bits;
	}
	bit_array &operator= (bit_array other)
	{
		std::swap (m_n_elts, other.m_n_elts);
		std::swap (m_n_bits, other.m_n_bits);
		std::swap (m_bits, other.m_bits);
		std::swap (m_last_mask, other.m_last_mask);
		return *this;
	}
	bool operator== (const bit_array &other) const
	{
		if (other.m_n_bits != m_n_bits)
			return false;
		return memcmp (m_bits, other.m_bits, m_n_elts * sizeof (uint64_t)) == 0;
	}
	bool operator!= (const bit_array &other) const
	{
		return !(*this == other);
	}
	void debug ()
	{
		for (unsigned bit = 0; bit < m_n_bits; bit++) {
			uint64_t val = m_bits[bit / 64];
			printf ("%d", (int)((val >> (bit % 64)) & 1));
		}
		putchar ('\n');
	}
	void clear ()
	{
		for (int i = 0; i < m_n_elts; i++)
			m_bits[i] = 0;
	}

	bool test_bit (unsigned n) const
	{
		if (n >= m_n_bits)
			return false;
		unsigned off = n / 64;
		unsigned pos = n % 64;
		uint64_t val = m_bits[off];
		val >>= pos;
		return (val & 1) != 0;
	}

	void set_bit (unsigned n)
	{
		if (n >= m_n_bits)
			return;
		unsigned off = n / 64;
		unsigned pos = n % 64;
		uint64_t val = m_bits[off];
		val |= (uint64_t)1 << pos;
		m_bits[off] = val;
	}

	void clear_bit (unsigned n)
	{
		if (n >= m_n_bits)
			return;
		unsigned off = n / 64;
		unsigned pos = n % 64;
		uint64_t val = m_bits[off];
		val &= ~((uint64_t)1 << pos);
		m_bits[off] = val;
	}

	/* IOR with the other bitmap, shifted left (or right, for negative numbers.
	   Apply a shifted mask before the IOR.  */
	bool ior (const bit_array &other, int shift, const bit_array &mask)
	{
		if (shift == 0)
			return ior (other);
		shift = -shift;

		// int sgn = shift / std::abs (shift);

		/* Examples:
		   shift -3: right shift by 3.
		   Dest word 0 receives [word1:0-3 :: word0:3-64]
		   shift 3:  left shift by 3.
		   Dest word 0 receives [word0:0-61 :: 000 ]
		   Dest word 1 receives [word1:0-61 :: word0:62-64 ]
		   shift 64: left shift by 64.
		   Dest word 0 receives 64 bits from src word -1.  */

		shift += other.m_n_elts * 64;
		int wordshift = (shift + 63) / 64 - other.m_n_elts - 1;
		int bitshift = shift % 64;

		uint64_t last = 0;
		if (wordshift >= 0 && wordshift < other.m_n_elts)
			last = other.m_bits[wordshift];
		if (wordshift >= 0 && wordshift < mask.m_n_elts)
			last &= mask.m_bits[wordshift];
		bool changed = false;
		for (int i = 0; i < m_n_elts; i++) {
			wordshift++;
			uint64_t curr = 0;
			if (wordshift >= 0 && wordshift < other.m_n_elts)
				curr = other.m_bits[wordshift];
			if (wordshift >= 0 && wordshift < mask.m_n_elts)
				curr &= mask.m_bits[wordshift];
			uint64_t val = m_bits[i];
			uint64_t val1 = val;
			if (bitshift != 0) {
				val |= curr << (64 - bitshift);
				val |= last >> bitshift;
			} else {
				val |= curr;
			}
			if (i + 1 == m_n_elts)
				val &= m_last_mask;
			changed |= val != val1;
			m_bits[i] = val;
			last = curr;
		}
		return changed;
	}
	bool ior (const bit_array &other, int shift)
	{
		return ior (other, shift, other);
	}

	bool ior (const bit_array &other)
	{
		int limit = std::min (m_n_elts, other.m_n_elts);
		bool changed = false;
		for (int i = 0; i < limit; i++) {
			uint64_t val = m_bits[i];
			uint64_t val1 = val;
			val |= other.m_bits[i];
			if (i + 1 == m_n_elts)
				val &= m_last_mask;
			changed |= val != val1;
			m_bits[i] = val;
		}
		return changed;
	}
	bool and1 (const bit_array &other)
	{
		int limit = std::min (m_n_elts, other.m_n_elts);
		bool changed = false;
		for (int i = 0; i < limit; i++) {
			uint64_t val = m_bits[i];
			uint64_t val1 = val;
			val &= other.m_bits[i];
			changed |= val != val1;
			m_bits[i] = val;
		}
		return changed;
	}
	bool andnot (const bit_array &other)
	{
		unsigned limit = std::min (m_n_elts, other.m_n_elts);
		bool changed = false;
		for (unsigned i = 0; i < limit; i++) {
			uint64_t val = m_bits[i];
			uint64_t val1 = val;
			val &= ~other.m_bits[i];
			changed |= val != val1;
			m_bits[i] = val;
		}
		return changed;
	}
	bool intersect_p (const bit_array &other) const
	{
		unsigned limit = std::min (m_n_elts, other.m_n_elts);
		for (unsigned i = 0; i < limit; i++) {
			if (m_bits[i] & other.m_bits[i])
				return true;
		}
		return false;
	}
	unsigned popcnt () const
	{
		unsigned cnt = 0;
		for (int i = 0; i < m_n_elts; i++) {
			uint64_t val = m_bits[i];
			while (val != 0) {
				if (val & 1)
					cnt++;
				val >>= 1;
			}
		}
		return cnt;
	}
};

#endif
