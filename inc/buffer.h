/* (c) 2020 ukrkyi */
#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <stddef.h>

#include "stm32_assert.h"

template <size_t N> class Buffer {
	uint8_t data[N];
	size_t head, tail;
public:
	class iterator {
		Buffer& parent;
		size_t position;
	public:
		iterator(Buffer& parent, size_t pos = 0) : parent(parent), position(pos) {}
		iterator(iterator &other) : parent(other.parent), position(other.position) {}
		uint8_t operator *() {return parent.data[position];}
		void operator++() {position = (position + 1) % N;}
		iterator &operator+=(size_t n) {position = (position + n) % N; return *this;}
		iterator operator+(size_t n) {return iterator(parent, (position + n) % N);}
		size_t getPosition() const {return position;}
		operator size_t() const {return getPosition();}
	};

	class chunk {
		friend class Buffer<N>;
		Buffer& parent;
		iterator begin, end;
		inline bool _same_start(const char *&other, iterator &pos) {
			for (; *other != 0 && pos.getPosition() != end; ++pos, ++other)
				if (*pos != *other)
					return false;
			return true;
		}
		size_t propagateStart(size_t position) {
			begin += position;
			return begin;
		}
	public:
		chunk(Buffer& parent, size_t start, size_t end) :
			parent(parent), begin(parent, start), end(parent, end) {}
		chunk(chunk const &other) :
			parent(other.parent), begin(other.parent, other.begin), end(other.parent, other.end) {}
		bool operator ==(const char *other) {
			iterator pos = begin;
			if (_same_start(other, pos)
			    && (*other == 0)
			    && (pos.getPosition() == end))
				return true;
			else
				return false;
		}
		bool beginOf(const char *other) {
			iterator pos = begin;
			return _same_start(other, pos) && (pos.getPosition() == end);
		}
		bool beginsWith(const char *string, size_t * next = NULL) {
			iterator pos = begin;
			bool result = _same_start(string, pos) && (*string == 0);
			if (next != NULL)
				*next = pos;
			return result;
		}
		operator uint8_t*() { return &(parent.data[begin]); }
		inline bool isOverlap() {return end < begin; }
		size_t length() {
			if (isOverlap())
				return N + end.getPosition() - begin.getPosition();
			else
				return end - begin;
		}
//		size_t tillEnd() {return N - begin;}
		iterator start() {return iterator(begin);}
		chunk getBefore(size_t pos) {return chunk(parent, begin, begin + pos); }
		chunk getAfter(size_t pos) { return chunk(parent, begin + pos, end); }
	};

	Buffer() : head(0), tail(0) {}
	Buffer(const Buffer&) = delete;
	Buffer& operator= ( const Buffer &other ) = delete;

	size_t length() {return N;}

	operator uint8_t*() { return &(data[0]); }
	uint8_t& operator [](size_t i) { return data[i];}

	chunk newData(size_t position, bool * was_overwrite) {
		bool overwrite;
		if (tail >= head) {
			if (position >= tail || position <= head)
				overwrite = false;
			else overwrite = true;
		} else {
			if (position >= tail && position <= head)
				overwrite = false;
			else overwrite = true;
		}

		if (was_overwrite != NULL)
			*was_overwrite = overwrite;

		chunk result(*this, tail, position);

		if (overwrite)
			head = tail; // Fix overwrite by losing data that can be overwritten

		tail = position;

		return result;
	}

	void processed(chunk& what) {
		assert_param(what.begin == head);
		assert_param((head < tail && what.end <= tail) ||
			     (head > tail && (what.end >= head || what.end <= tail)));

		head = what.propagateStart(what.length());
	}

	void next(chunk& what, size_t howMuch) {
		assert_param(what.begin == head);
		assert_param((head < tail && what.end <= tail) ||
			     (head > tail && (what.end >= head || what.end <= tail)));
		assert_param(howMuch <= what.length());

		if (howMuch == what.length()){
			processed(what);
		} else
			head = what.propagateStart(howMuch);
	}
};

#endif // BUFFER_H
