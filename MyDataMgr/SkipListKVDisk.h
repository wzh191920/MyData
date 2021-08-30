#pragma once
#include <random>
#include <utility>
#include "disk_alloc.h"

class VarCharDisk {
public:
	VarCharDisk() : length_(0) {}
	char *GetChar(char *c) {
		char *tc = c;
		char *tc_ = c_;
		int count = length_;
		while (count && (*tc++ = *tc_++)) 
			count--;
		if (count == 0)
			*tc = 0;
		return c;
	}
	unsigned char *GetByt(unsigned char *c, int &length) {
		memcpy(c, c_, length_);
		length = length_;
		return c;
	}
	void ReSetByt(unsigned char *c, unsigned char length) {
		if (length <= 255)
		{
			if (length_ == 0)
				c_ = (char *)c, length_ = length;
		}
	}
	int SetChar(char *c, unsigned char length) {
		int ret = 0;
		if (length <= 255)
		{
			if (length_ == 0)
				c_ = c, length_ = length;
			else if (length_ >= length)
			{
				memcpy(c_, c, length);
				if (length_ > length) c_[length] = 0;
			}
			else
				ret = -1;
		}
		else
			ret = -1;
		return ret;
	}
	void ReSetChar(char *c, unsigned char length) {
		if (length <= 255)
			c_ = c, length_ = length;
	}
	void Serialize(disk_alloc *da) {//仅用于插入
		char* tmpc = (char*)da->my_relative_alloc(length_);
		memcpy(da->get_absolute_pos(tmpc), c_, length_);
		c_ = tmpc;
	}
	void UnSerialize(disk_alloc *da) {
		c_ = da->get_absolute_pos(c_);
	}
private:
	char *c_;
	unsigned char length_;
};

template<typename Key, typename Value, class Comparator>
class SkipListKVDisk {
private:
	struct Node;
public:
	SkipListKVDisk(){}
	void Insert(const Key& key, Value& value);
	bool Contains(const Key& key);
	bool Erase(const Key& key);
	int Init(const char* filename, int length, int expend_length);
	int Close();
	int Size();
	int ReBuild();
	class Iterator {
	public:
		explicit Iterator(SkipListKVDisk* list);
		bool Valid() const;
		const Key& key() const;
		template<typename Value>
		Value VarCharValue() {
			Node* p = list_->da_.get_absolute_pos(node_);
			Value tv(p->value);
			tv.UnSerialize(&list_->da_);
			return tv;
		}
		Value& value() {
			Node* p = list_->da_.get_absolute_pos(node_);
			return p->value;
		}
		template<typename Value>
		std::pair<const Key, Value> Item() {
			Node* p = list_->da_.get_absolute_pos(node_);
			Value tv(p->value);
			tv.UnSerialize(&list_->da_);
			return std::pair<const Key, Value>(p->key, tv);
		}
		template<>
		std::pair<const Key, int> Item<int>() {
			Node* p = list_->da_.get_absolute_pos(node_);
			return std::pair<const Key, Value>(p->key, p->value);
		}
		template<>
		std::pair<const Key, double> Item<double>() {
			Node* p = list_->da_.get_absolute_pos(node_);
			return std::pair<const Key, Value>(p->key, p->value);
		}
		template<>
		std::pair<const Key, short> Item<short>() {
			Node* p = list_->da_.get_absolute_pos(node_);
			return std::pair<const Key, Value>(p->key, p->value);
		}
		template<>
		std::pair<const Key, void*> Item() {
			Node* p = list_->da_.get_absolute_pos(node_);
			return std::pair<const Key, Value>(p->key, p->value);
		}
		void Next();
		void Prev();
		void Seek(const Key& target);
		void SeekToFirst();
		void SeekToLast();
	private:
		SkipListKVDisk* list_;
		Node* node_;
	};
	
private:
	enum { kMaxHeight = 12 };
	Comparator compare_;
	Node* head_;
	int *max_height_;
	int *size_;
	std::random_device rnd_;
	disk_alloc da_;
	template<typename Value>
	Node* NewNode(const Key& key, Value &value, int height) {
		Node* mem = (Node*)da_.my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		Node* realmem = da_.get_absolute_pos(mem);
		realmem->key = key;
		realmem->value = value;
		realmem->value.Serialize(&da_);
		return mem;
	}
	Node* NewNode(const Key& key, int &value, int height) {
		char* mem = (char*)da_.my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}
	Node* NewNode(const Key& key, double &value, int height) {
		char* mem = (char*)da_.my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}
	Node* NewNode(const Key& key, short &value, int height) {
		char* mem = (char*)da_.my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}
	template<typename Value>
	Node* NewNode(const Key& key, Value* &value, int height) {
		char* mem = (char*)da_.my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}

	int RandomHeight();
	bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }
	bool KeyIsAfterNode(const Key& key, Node* n);
	Node* FindGreaterOrEqual(const Key& key, Node** prev);
	Node* FindLessThan(const Key& key);
	Node* FindLast();
	SkipListKVDisk(const SkipListKVDisk&);
	void operator=(const SkipListKVDisk&);
	static void ReMapFun(void* para);
};
template<typename Key, typename Value, class Comparator>
struct SkipListKVDisk<Key, typename Value, Comparator>::Node {
	explicit Node(const Key& k, const Value& v, SkipListKVDisk<Key, Value, Comparator> *skiplist) {
		skiplist->da_.get_absolute_pos(this)->key = k;
		skiplist->da_.get_absolute_pos(this)->value = v;
	}
	Key key;
	Value value;
	Node* Next(int n, SkipListKVDisk<Key, Value, Comparator> *skiplist) {
		return reinterpret_cast<Node*>(skiplist->da_.get_absolute_pos(next_)[n]);
	}
	void SetNext(int n, Node* x, SkipListKVDisk<Key, Value, Comparator> *skiplist) {
		skiplist->da_.get_absolute_pos(next_)[n] = x;
	}

private:
	void* next_[1];
};


template<typename Key, typename Value, class Comparator>
inline SkipListKVDisk<Key, typename Value, Comparator>::Iterator::Iterator(SkipListKVDisk* list) {
	list_ = list;
	node_ = NULL;
}
template<typename Key, typename Value, class Comparator>
inline bool SkipListKVDisk<Key, Value, Comparator>::Iterator::Valid() const {
	return node_ != NULL;
}
template<typename Key, typename Value, class Comparator>
inline const Key& SkipListKVDisk<Key, typename Value, Comparator>::Iterator::key() const {
	//return node_->key;//==
	return list_->da_.get_absolute_pos(node_)->key;
}

template<typename Key, typename Value, class Comparator>
inline void SkipListKVDisk<Key, typename Value, Comparator>::Iterator::Next() {
	node_ = node_->Next(0, list_);
}
template<typename Key, typename Value, class Comparator>
inline void SkipListKVDisk<Key, typename Value, Comparator>::Iterator::Prev() {
	//node_ = list_->FindLessThan(node_->key);//==
	node_ = list_->FindLessThan(list_->da_.get_absolute_pos(node_)->key);
	if (node_ == list_->head_) {
		node_ = NULL;
	}
}
template<typename Key, typename Value, class Comparator>
inline void SkipListKVDisk<Key, typename Value, Comparator>::Iterator::Seek(const Key& target) {
	node_ = list_->FindGreaterOrEqual(target, NULL);
}
template<typename Key, typename Value, class Comparator>
inline void SkipListKVDisk<Key, typename Value, Comparator>::Iterator::SeekToFirst() {
	node_ = list_->head_->Next(0, list_);
}
template<typename Key, typename Value, class Comparator>
inline void SkipListKVDisk<Key, typename Value, Comparator>::Iterator::SeekToLast() {
	node_ = list_->FindLast();
	if (node_ == list_->head_) {
		node_ = NULL;
	}
}

template<typename Key, typename Value, class Comparator>
int SkipListKVDisk<Key, typename Value, Comparator>::RandomHeight() {
	static const unsigned int kBranching = 4;
	int height = 1;
	while (height < kMaxHeight && ((rnd_() % kBranching) == 0)) {
		height++;
	}
	return height;
}
template<typename Key, typename Value, class Comparator>
bool SkipListKVDisk<Key, typename Value, Comparator>::KeyIsAfterNode(const Key& key, Node* n) {
	//return (n != NULL) && (compare_(n->key, key) < 0);//==
	return (n != NULL) && (compare_(da_.get_absolute_pos(n)->key, key) < 0);
}
template<typename Key, typename Value, class Comparator>
typename SkipListKVDisk<Key, Value, Comparator>::Node* SkipListKVDisk<Key, Value, Comparator>::FindGreaterOrEqual(const Key& key, Node** prev) {
	Node* x = head_;
	int level = *da_.get_absolute_pos(max_height_) - 1;
	while (true)
	{
		Node* next = x->Next(level, this);
		if (KeyIsAfterNode(key, next)) {
			x = next;
		}
		else {
			if (prev != NULL) prev[level] = x;//prev是相对
			if (level == 0) {
				return next;
			}
			else {
				level--;
			}
		}
	}
}
template<typename Key, typename Value, class Comparator>
typename SkipListKVDisk<Key, typename Value, Comparator>::Node*
	SkipListKVDisk<Key, Value, Comparator>::FindLessThan(const Key& key) {
		Node* x = head_;
		int level = *da_.get_absolute_pos(max_height_) - 1;
		while (true)
		{
			Node* next = x->Next(level, this);
			//if (next == NULL || compare_(next->key, key) >= 0) {
			if (next == NULL || compare_(da_.get_absolute_pos(next)->key, key) >= 0) {
				if (level == 0) {
					return x;
				}
				else {
					level--;
				}
			}
			else {
				x = next;
			}
		}
}
template<typename Key, typename Value, class Comparator>
typename SkipListKVDisk<Key, Value, Comparator>::Node* SkipListKVDisk<Key, Value, Comparator>::FindLast()
{
	Node* x = head_;
	int level = *da_.get_absolute_pos(max_height_) - 1;
	while (true) {
		Node* next = x->Next(level, this);
		if (next == NULL) {
			if (level == 0) {
				return x;
			}
			else {
				level--;
			}
		}
		else
		{
			x = next;
		}
	}
}

template<typename Key, typename Value, class Comparator>
void SkipListKVDisk<Key, Value, Comparator>::Insert(const Key& key, Value& value) {
	Node* prev[kMaxHeight];
	Node* x = FindGreaterOrEqual(key, prev);
	//不允许重复,重复的问题：反向遍历只遍历到一个值
	if (x != NULL && Equal(da_.get_absolute_pos(x)->key, key))
		return;
	
	int height = RandomHeight();
	int *max_height = da_.get_absolute_pos(max_height_);
	if (height > *max_height) {
		for (int i = *max_height; i < height; i++)
		{
			prev[i] = head_;
		}
		*max_height = height;
	}
	x = NewNode(key, value, height);
	for (int i = 0; i < height; i++) {
		x->SetNext(i, prev[i]->Next(i, this), this);
		prev[i]->SetNext(i, x, this);
	}
	int *size = da_.get_absolute_pos(size_);
	++*size;
}
template<typename Key, typename Value, class Comparator>
bool SkipListKVDisk<Key, Value, Comparator>::Erase(const Key& key) {
	Node* prev[kMaxHeight] = { 0 };
	Node* x = FindGreaterOrEqual(key, prev);
	if (x != NULL && Equal(key, da_.get_absolute_pos(x)->key))
	{
		int *max_height = da_.get_absolute_pos(max_height_);
		for (int i = 0; i < *max_height; i++)
		{
			if (prev[i] != NULL && prev[i]->Next(i, this) == x)
			{
				prev[i]->SetNext(i, x->Next(i, this), this);
			}
		}
		int *size = da_.get_absolute_pos(size_);
		--*size;
		//delete[] (char*)x;//==
		return true;
	}
	return false;
}
template<typename Key, typename Value, class Comparator>
bool SkipListKVDisk<Key, Value, Comparator>::Contains(const Key& key) {
	Node* x = FindGreaterOrEqual(key, NULL);
	if (x != NULL && Equal(key, da_.get_absolute_pos(x)->key)) {
		return true;
	}
	else
	{
		return false;
	}
}

template<typename Key, typename Value, class Comparator>
int SkipListKVDisk<Key, Value, Comparator>::Init(const char* filename, int length, int expend_length)
{
	int ret = da_.init(filename, length, expend_length);
	if (ret) return ret;
	
	if (da_.isclean())
	{
		max_height_ = (int*)da_.my_relative_alloc(sizeof(int));
		int *max_height = da_.get_absolute_pos(max_height_);
		*max_height = 1;
		size_ = (int*)da_.my_relative_alloc(sizeof(int));
		int *size = da_.get_absolute_pos(size_);
		*size = 0;
		Value tv;
		head_ = NewNode(Key(), tv, kMaxHeight);
		for (int i = 0; i < kMaxHeight; i++) {
			head_->SetNext(i, NULL, this);
		}
	}
	else
	{
		max_height_ = (int*)(sizeof(int)*2); //跳过disk_alloc的头部
		size_ = (int*)(sizeof(int)*3);
		head_ = (Node*)(sizeof(int)*4);
	}
	compare_ = Comparator();
	return ret;
}

template<typename Key, typename Value, class Comparator>
void SkipListKVDisk<Key, Value, Comparator>::ReMapFun(void* para)
{
	//使用相对地址，不再需要
	/*SkipListKVDisk* skiplist = (SkipListKVDisk*)para;
	skiplist->max_height_ = (int*)skiplist->da_.data_addr();*/
}

template<typename Key, typename Value, class Comparator>
int SkipListKVDisk<Key, Value, Comparator>::Close()
{
	return da_.close();
}

template<typename Key, typename Value, class Comparator>
int SkipListKVDisk<Key, Value, Comparator>::Size()
{
	int *size = da_.get_absolute_pos(size_);
	return *size;
}

template<typename Key, typename Value, class Comparator>
int SkipListKVDisk<Key, Value, Comparator>::ReBuild()
{
	int ret = 0;
	char file[MAX_PATH];
	char tmpfile[MAX_PATH];
	size_t filesize = da_.size();
	size_t expend_length = da_.get_expend_length();
	strcpy_s(file, da_.filename());
	strcpy_s(tmpfile, file);
	strcat_s(tmpfile, "_tmp");
	SkipListKVDisk<Key, Value, Comparator> tmpskiplist;
	ret = tmpskiplist.Init(tmpfile, 0, expend_length);
	if (ret)
		return ret;
	SkipListKVDisk<Key, Value, Comparator>::Iterator iter(this);
	{
		iter.SeekToFirst();
		while(iter.Valid())
		{
			std::pair<const Key, Value> p(iter.Item<Value>());
			tmpskiplist.Insert(p.first, p.second);
			iter.Next();
		}
	}
	ret = tmpskiplist.da_.close();
	if (ret)
		return ret;
	ret = da_.close();
	if (ret)
		return ret;
	remove(file);
	rename(tmpfile, file);
	ret = Init(file, 0, expend_length);
	return ret;
}

class SerBase
{
public:
	void Serialize(disk_alloc *da)
	{
	}
	void UnSerialize(disk_alloc *da)
	{
	}
};