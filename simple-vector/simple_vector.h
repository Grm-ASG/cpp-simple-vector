#pragma once
#include "array_ptr.h"

#include <cassert>
#include <stdexcept>
#include <initializer_list>
#include <array>

class ProxyReserve
{
public:
	ProxyReserve() = delete;
	ProxyReserve(size_t size)
		: _size(size)
	{}

	size_t GetSize()
	{
		return _size;
	}

private:
	size_t _size;
};

ProxyReserve Reserve(size_t size)
{
	return ProxyReserve(size);
}

template <typename Type>
class SimpleVector
{
public:
	using Iterator = Type*;
	using ConstIterator = const Type*;

	SimpleVector() noexcept = default;

	// Создаёт пустой вектор c резервированием размера
	explicit SimpleVector(ProxyReserve proxy_size)
	{
		const size_t new_size = proxy_size.GetSize();
		ArrayPtr<Type> newArray(new_size);
		_data.swap(newArray);
		_capacity = new_size;
	}

	// Создаёт вектор из size элементов, инициализированных значением по умолчанию
	explicit SimpleVector(size_t size)
		: _size(size), _capacity(size)
	{
		SimpleVector tmp(size, Type{});
		_data.swap(tmp._data);
	}

	// Создаёт вектор из size элементов, инициализированных значением value
	SimpleVector(size_t size, const Type& value)
		: _size(size), _capacity(size)
	{
		ArrayPtr<Type> newArray(size);

		std::fill(newArray.Get(), newArray.Get() + size, value);

		_data.swap(newArray);
	}

	// Создаёт вектор из std::initializer_list
	SimpleVector(std::initializer_list<Type> init)
		: _size(init.size()), _capacity(_size)
	{
		ArrayPtr<Type> newArray(_size);

		std::copy(init.begin(), init.end(), newArray.Get());

		_data.swap(newArray);
	}

	SimpleVector(const SimpleVector& other)
	{
		ArrayPtr<Type> newArray(other._capacity);

		std::copy(other.begin(), other.end(), newArray.Get());

		_data.swap(newArray);
		_size = other._size;
		_capacity = other._capacity;
	}

	SimpleVector(SimpleVector&& other)
	{
		_data.swap(other._data);
		std::swap(_size, other._size);
		std::swap(_capacity, other._capacity);
	}

	SimpleVector& operator=(const SimpleVector& rhs)
	{
		if (this != &rhs)
		{
			ArrayPtr<Type> newArray(rhs._size);

			std::copy(rhs.begin(), rhs.end(), newArray.Get());

			_data.swap(newArray);

			_size = rhs._size;
			_capacity = rhs._capacity;
		}
		return *this;
	}

	SimpleVector& operator=(SimpleVector&& rhs)
	{
		if (this != &rhs)
		{
			this->_capacity = rhs._capacity;
			this->_size = rhs._size;
			this->_data.swap(rhs._data);
		}
		return *this;
	}

	// Резервирует емкость вектора заданного размера
	void Reserve(size_t new_capacity)
	{
		if (_capacity < new_capacity)
		{
			ArrayPtr<Type> newArray(new_capacity);

			std::copy(begin(), end(), newArray.Get());

			_data.swap(newArray);
			_capacity = new_capacity;
		}
	}

	// Добавляет элемент в конец вектора
	// При нехватке места увеличивает вдвое вместимость вектора
	void PushBack(const Type& item)
	{
		Type tmp(item);

		PushBack(std::move(tmp));
	}

	void PushBack(Type&& item)
	{
		if (_size < _capacity)
		{
			_data[_size++] = std::move(item);
		}
		else if (_size == _capacity)
		{
			if (_capacity == 0)
			{
				_capacity = 4;
			}
			ArrayPtr<Type> newArray(_capacity * 2);

			std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), newArray.Get());

			newArray[_size++] = std::move(item);
			_data.swap(newArray);
			_capacity *= 2;
		}
		else // size > capacity
		{
			throw std::logic_error("Something bad");
		}
	}

	// Вставляет значение value в позицию pos.
	// Возвращает итератор на вставленное значение
	// Если перед вставкой значения вектор был заполнен полностью,
	// вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
	Iterator Insert(ConstIterator pos, const Type& value)
	{
		return Insert(pos, std::move(value));
	}

	Iterator Insert(ConstIterator pos, Type&& value)
	{
		Iterator cur_pos = Iterator(pos);
		Iterator to_return{};
		size_t num_pos = cur_pos - begin();
		if (_size < _capacity)
		{
			if (cur_pos == end())
			{
				PushBack(std::move(value));
				return cur_pos;
			}
			else
			{
				std::move_backward(cur_pos, end(), std::next(end()));
				*cur_pos = std::move(value);
			}

			to_return = cur_pos;
		}
		else if (_size == _capacity)
		{
			_capacity = _capacity == 0 ? 1 : _capacity * 2;

			ArrayPtr<Type> newArray(_capacity);

			Type* ptr = newArray.Get();

			std::copy(std::make_move_iterator(begin()), std::make_move_iterator(cur_pos), ptr);

			ptr[num_pos] = std::move(value);

			std::copy(std::make_move_iterator(cur_pos), std::make_move_iterator(end()), &ptr[num_pos + 1]);

			_data.swap(newArray);

			to_return = begin() + num_pos;
		}
		else // _size > _capacity
		{
			throw std::logic_error("Something bad");
		}
		++_size;
		return to_return;
	}

	// "Удаляет" последний элемент вектора. Вектор не должен быть пустым
	void PopBack() noexcept
	{
		if (_size != 0)
		{
			--_size;
		}
	}

	// Удаляет элемент вектора в указанной позиции
	Iterator Erase(ConstIterator pos)
	{
		size_t pos_num = pos - begin();
		auto iter = begin() + pos_num;
		auto end_it = begin() + _size - 1;
		while (iter < end_it)
		{
			auto iter_next = std::next(iter);
			*iter = std::move(*iter_next);
			++iter;
		}
		--_size;
		return Iterator(pos);
	}

	// Обменивает значение с другим вектором
	void swap(SimpleVector& other) noexcept
	{
		size_t tmp_size = other._size;
		size_t tmp_capacity = other._capacity;

		_data.swap(other._data);
		other._size = _size;
		other._capacity = _capacity;
		_size = tmp_size;
		_capacity = tmp_capacity;
	}

	// Возвращает количество элементов в массиве
	size_t GetSize() const noexcept
	{
		return _size;
	}

	// Возвращает вместимость массива
	size_t GetCapacity() const noexcept
	{
		return _capacity;
	}

	// Сообщает, пустой ли массив
	bool IsEmpty() const noexcept
	{
		return _size == 0;
	}

	// Возвращает ссылку на элемент с индексом index
	Type& operator[](size_t index) noexcept
	{
		return _data[index];
	}

	// Возвращает константную ссылку на элемент с индексом index
	const Type& operator[](size_t index) const noexcept
	{
		return const_cast<const Type&>(_data[index]);
	}

	// Возвращает константную ссылку на элемент с индексом index
	// Выбрасывает исключение std::out_of_range, если index >= size
	Type& At(size_t index)
	{
		if (index >= _size)
		{
			throw std::out_of_range("Out of range");
		}
		return _data[index];
	}

	// Возвращает константную ссылку на элемент с индексом index
	// Выбрасывает исключение std::out_of_range, если index >= size
	const Type& At(size_t index) const
	{
		if (index >= _size)
		{
			throw std::out_of_range("Out of range");
		}
		return const_cast<const Type&>(_data[index]);
	}

	// Обнуляет размер массива, не изменяя его вместимость
	void Clear() noexcept
	{
		_size = 0;
	}

	// Изменяет размер массива.
	// При увеличении размера новые элементы получают значение по умолчанию для типа Type
	void Resize(size_t new_size)
	{
		if (new_size < _size)
		{
			_size = new_size;
		}
		else if (new_size <= _capacity)
		{
			auto iter = begin() + _size;
			fill_non_copyble(iter, iter + (new_size - _size));

			_size = new_size;
		}
		else // new_size > _capacity
		{
			ArrayPtr<Type> newArray(new_size);

			std::move(begin(), end(), newArray.Get());
			fill_non_copyble(newArray.Get() + _size, newArray.Get() + new_size);

			_data.swap(newArray);
			_capacity = _size = new_size;
		}
	}

	void fill_non_copyble(Iterator first, Iterator last)
	{
		while (first < last)
		{
			*first++ = std::move(Type{});
		}
	}

	// Возвращает итератор на начало массива
	// Для пустого массива может быть равен (или не равен) nullptr
	Iterator begin() noexcept
	{
		return _data.Get();
	}

	// Возвращает итератор на элемент, следующий за последним
	// Для пустого массива может быть равен (или не равен) nullptr
	Iterator end() noexcept
	{
		return _data.Get() + _size;
	}

	// Возвращает константный итератор на начало массива
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator begin() const noexcept
	{
		return cbegin();
	}

	// Возвращает итератор на элемент, следующий за последним
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator end() const noexcept
	{
		return cend();
	}

	// Возвращает константный итератор на начало массива
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator cbegin() const noexcept
	{
		return const_cast<ConstIterator>(_data.Get());
	}

	// Возвращает итератор на элемент, следующий за последним
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator cend() const noexcept
	{
		return const_cast<ConstIterator>(_data.Get() + _size);
	}

private:
	ArrayPtr<Type> _data;
	size_t _size = 0;
	size_t _capacity = 0;
};


template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
	return lhs.GetSize() != rhs.GetSize() ? false : std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
	return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
	return (lhs == rhs || lhs < rhs);;
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
	return !(lhs < rhs) && (lhs != rhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs)
{
	return (lhs == rhs || lhs > rhs);
}