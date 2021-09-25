#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::Delete(const std::string &key) {

	auto map_iter = _lru_index.find(std::ref(key)); 
	if (map_iter != _lru_index.end()) {
                lru_node & node = map_iter->second.get();
		_lru_index.erase(map_iter);
		_cur_size -= node.key.length();
		_cur_size -= node.value.length();
		saved_value = node.value;
		lru_node * left = node.prev;
		if (left == nullptr) {
			//если node - первый элемент
			std::swap(_lru_head, node.next);
			if (_lru_head.get() != nullptr)
				_lru_head.get()->prev = nullptr;
			node.next.reset();		
		}
		else {
			if (node.next.get() == nullptr) {
				//если node - последний элемент
				tail = left;
				node.prev = nullptr;
				left->next.reset();
			}
			else { //если node -не первый и не последний элемент
				std::swap(left->next, node.next);
				left->next.get()->prev = left;
				node.prev = nullptr;		
				node.next.reset();
			}
		}
		return true;
	}
	return false;
}

void SimpleLRU::add_node1(const std::string & key, const std::string &value) {
	lru_node * new_node = new lru_node {key, value, nullptr, std::move(_lru_head)};
	if (new_node->next.get() != nullptr) {
		new_node->next.get()->prev = new_node;	
	}
	_lru_head = std::unique_ptr<lru_node>(new_node);
	if (_cur_size == 0) //если это первый элемент
		tail = _lru_head.get();
        _lru_index.emplace(std::make_pair(std::ref(new_node->key), std::ref(*new_node)));
}

void SimpleLRU::add_node(const std::string & key, const std::string &value, std::size_t size) {
	std::size_t new_size = _cur_size + size ;
	if (new_size <= _max_size) {
	//нам хватает места, чтобы вставить новый элемент
		add_node1(key, value);
		_cur_size = new_size; 	
	}
	else { 
		while (_cur_size + size > _max_size) {
			_cur_size -= tail->key.length();
			_cur_size -= tail->value.length();
			_lru_index.erase(tail->key);
			tail = tail->prev;
			tail->next.get()->prev = nullptr;
			tail->next.reset();
		}
		add_node1(key, value);
		_cur_size += size; 	
	}
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
	std::size_t size = key.length() + value.length();
	if (size > _max_size) 
		return false;
	//удаляем старое вхождения ключа, если есть
	Delete(key);
	add_node(key, value, size);
	return true;	
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { 
	std::size_t size = key.length() + value.length();
	if (size > _max_size) 
		return false;
	auto map_iter = _lru_index.find(std::ref(key));  
	if (map_iter != _lru_index.end())
		return false;
	add_node(key, value, size);
	return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) { 
	std::size_t size = key.length() + value.length();
	if (size > _max_size)
		return false;
	bool del = Delete(key);
	if (!del)
		return false;
	add_node(key, value, size);
	return true;
	}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { 
	bool del = Delete(key);
	if (!del)
		return false;
	value = saved_value;
	std::size_t size = key.length() + value.length();
	add_node(key, value, size);
	return true;
}

} // namespace Backend
} // namespace Afina




	
