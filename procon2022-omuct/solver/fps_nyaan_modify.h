
#pragma once

#include "montgomery_modint_info.h"

#include <immintrin.h>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <cassert>

namespace FpsNyaan {

	

	template<class Elem>
	class AlignedArray {
	private:
		uint32 m_length;
		Elem* m_ptr = nullptr;

	public:

		AlignedArray(uint32 length)
			: m_length(length)
		{}

		AlignedArray(const AlignedArray&) = delete;

		AlignedArray(AlignedArray&& r)
			: m_length(r.m_length)
			, m_ptr(r.m_ptr)
		{
			r.m_ptr = nullptr;
		}

		~AlignedArray() {
			free();
		}

		bool isAllocated() const { return m_ptr; }
		bool isAvailable() const { return this->isAllocated(); }
		Elem* getPtr() const { return m_ptr; }
		uint32 getLength() const { return m_length; }

		// メモリ全体を 0 で上書きする
		void clear() const {
			if (!isAvailable()) throw Error(U"AlignedArray::clear failed (memory is not available)");
			memset(m_ptr, 0, sizeof(Elem) * m_length);
		}

		// Array から device memory にコピー
		void copyFrom(const Array<Elem>& src) const {
			if (!isAvailable()) throw Error(U"AlignedArray::copyFrom failed (memory is not available)");
			uint32 sz = (uint32)std::min<size_t>(m_length, src.size());
			memcpy(m_ptr, src.data(), sizeof(Elem) * sz);
		}

		// device memory から Array にコピー
		void copyTo(Array<Elem>& dest) const {
			if (!isAvailable()) throw Error(U"AlignedArray::copyTo failed (memory is not available)");
			uint32 sz = (uint32)std::min<size_t>(m_length, dest.size());
			memcpy(dest.data(), m_ptr, sizeof(Elem) * sz);
		}

		// メモリを確保する
		// すでに確保されていた場合は何もしない
		void malloc() {
			if (isAllocated()) return;

			m_ptr = (Elem*)_aligned_malloc(sizeof(Elem) * m_length, 64);
			if (m_ptr == nullptr) throw Error(U"AlignedArray::malloc generated null");
		}

		// メモリを解放する
		// メモリを確保していない場合は何もしない
		void free() {
			if (!isAllocated()) return;

			::_aligned_free(m_ptr);
			m_ptr = nullptr;
		}

	};



	class ModintVector {

	public:

		uint32* getPtr() const;

		ModintVector(uint32 MOD);

		ModintVector(Modint::ModPreset MOD_PRESET);

		Modint::ModPreset getModPreset();

		uint32 length() const;

		// すべての要素を 0 にする
		// 0 はモンゴメリ表現でも通常表現でも 0 になる
		void clear() const;

		// Array に読み込む
		Array<uint32> read(uint32 lengthMax = ~(uint32)0) const;

		// Array から書き込む
		void write(const Array<uint32>& src) const;

		// 通常表現をモンゴメリ表現に変換
		ModintVector& intoMontgomery();

		// モンゴメリ・リダクション
		// モンゴメリ表現を通常表現に変換
		ModintVector& montgomeryReduction();

		// すべての要素はモンゴメリ表現
		// r : モンゴメリ表現
		// A[i] <- r A[i]
		ModintVector& multiplyInMontgomery(uint32 r);

		// すべての要素はモンゴメリ表現
		// r : モンゴメリ表現
		// A[i] <- r[i] A[i]
		ModintVector& multiplyInMontgomery(const ModintVector& r);

		// バタフライ演算を行う
		ModintVector& butterflyInMontgomery();

		// バタフライ演算の逆を行う
		ModintVector& butterflyInvInMontgomery();

	private:

		Modint::ModPreset PRESET;
		AlignedArray<uint32> m_mem;

	};


} // namespace FpsNyaan
