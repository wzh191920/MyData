#pragma once
#include <random>
#include <utility>
#include "disk_alloc.h"
#include "SkipListKVDisk.h"

template<typename Key, typename Value, class Comparator>
class MultiSkipListMgr;

template<typename Key, typename Value, class Comparator>
class MultiSkipListKVDisk {
private:
	struct Node;
public:
	MultiSkipListKVDisk(){}
	void Insert(const Key& key, Value& value);
	bool Contains(const Key& key);
	bool Erase(const Key& key);
	int Size();
	class Iterator {
	public:
		explicit Iterator(MultiSkipListKVDisk* list);
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
			Node* p = list_->parent_->da_.get_absolute_pos(node_);
			return p->value;
		}
		template<typename Value>
		std::pair<const Key, Value> Item() {
			Node* p = list_->parent_->da_.get_absolute_pos(node_);
			Value tv(p->value);
			tv.UnSerialize(&list_->parent_->da_);
			return std::pair<const Key, Value>(p->key, tv);
		}
		template<>
		std::pair<const Key, int> Item<int>() {
			Node* p = list_->parent_->da_.get_absolute_pos(node_);
			return std::pair<const Key, Value>(p->key, p->value);
		}
		template<>
		std::pair<const Key, double> Item<double>() {
			Node* p = list_->parent_->da_.get_absolute_pos(node_);
			return std::pair<const Key, Value>(p->key, p->value);
		}
		template<>
		std::pair<const Key, short> Item<short>() {
			Node* p = list_->parent_->da_.get_absolute_pos(node_);
			return std::pair<const Key, Value>(p->key, p->value);
		}
		template<>
		std::pair<const Key, void*> Item<void*>() {
			Node* p = list_->parent_->da_.get_absolute_pos(node_);
			return std::pair<const Key, void*>(p->key, p->value);
		}
		void Next();
		void Prev();
		void Seek(const Key& target);
		void SeekToFirst();
		void SeekToLast();
	private:
		MultiSkipListKVDisk* list_;
		Node* node_;
	};
	template<typename Key, typename Value, class Comparator>
	friend class MultiSkipListMgr;
private:
	enum { kMaxHeight = 12 };
	int* max_height_;
	int* size_;
	Node* head_;
	MultiSkipListMgr<Key, Value, Comparator> *parent_;//动态设置
	template<typename Value>
	Node* NewNode(const Key& key, Value &value, int height) {
		Node* mem = (Node*)(parent_->da_).my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		Node* realmem = da_.get_absolute_pos(mem);
		realmem->key = key;
		realmem->value = value;
		realmem->value.Serialize(&da_);
		return mem;
	}
	Node* NewNode(const Key& key, int &value, int height) {
		char* mem = (char*)(parent_->da_).my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}
	Node* NewNode(const Key& key, double &value, int height) {
		char* mem = (char*)(parent_->da_).my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}
	Node* NewNode(const Key& key, short &value, int height) {
		char* mem = (char*)(parent_->da_).my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}
	template<typename Value>
	Node* NewNode(const Key& key, Value* &value, int height) {
		char* mem = (char*)(parent_->da_).my_relative_alloc(sizeof(Node) + sizeof(void*) * (height - 1));
		return new (mem)Node(key, value, this);
	}

	int RandomHeight();
	bool Equal(const Key& a, const Key& b) const { return (parent_->compare_(a, b) == 0); }
	bool KeyIsAfterNode(const Key& key, Node* n);
	Node* FindGreaterOrEqual(const Key& key, Node** prev);
	Node* FindLessThan(const Key& key);
	Node* FindLast();
	MultiSkipListKVDisk(const MultiSkipListKVDisk&);
	//void operator=(const MultiSkipListKVDisk&); 副本是主本的浅拷贝！！！
	static void ReMapFun(void* para);
};

template<typename Key, typename Value, class Comparator>
struct MultiSkipListKVDisk<Key, typename Value, Comparator>::Node {
	explicit Node(const Key& k, const Value& v, MultiSkipListKVDisk<Key, Value, Comparator> *skiplist) {
		skiplist->parent_->da_.get_absolute_pos(this)->key = k;
		skiplist->parent_->da_.get_absolute_pos(this)->value = v;
	}
	Key key;
	Value value;
	Node* Next(int n, MultiSkipListKVDisk<Key, Value, Comparator> *skiplist) {
		return reinterpret_cast<Node*>(skiplist->parent_->da_.get_absolute_pos(next_)[n]);
	}
	void SetNext(int n, Node* x, MultiSkipListKVDisk<Key, Value, Comparator> *skiplist) {
		skiplist->parent_->da_.get_absolute_pos(next_)[n] = x;
	}

private:
	void* next_[1];
};


template<typename Key, typename Value, class Comparator>
inline MultiSkipListKVDisk<Key, typename Value, Comparator>::Iterator::Iterator(MultiSkipListKVDisk* list) {
	list_ = list;
	node_ = NULL;
}
template<typename Key, typename Value, class Comparator>
inline bool MultiSkipListKVDisk<Key, Value, Comparator>::Iterator::Valid() const {
	return node_ != NULL;
}
template<typename Key, typename Value, class Comparator>
inline const Key& MultiSkipListKVDisk<Key, typename Value, Comparator>::Iterator::key() const {
	//return node_->key;//==
	return list_->parent_->da_.get_absolute_pos(node_)->key;
}

template<typename Key, typename Value, class Comparator>
inline void MultiSkipListKVDisk<Key, typename Value, Comparator>::Iterator::Next() {
	node_ = node_->Next(0, list_);
}
template<typename Key, typename Value, class Comparator>
inline void MultiSkipListKVDisk<Key, typename Value, Comparator>::Iterator::Prev() {
	//node_ = list_->FindLessThan(node_->key);//==
	node_ = list_->FindLessThan(list_->parent_->da_.get_absolute_pos(node_)->key);
	if (node_ == list_->head_) {
		node_ = NULL;
	}
}
template<typename Key, typename Value, class Comparator>
inline void MultiSkipListKVDisk<Key, typename Value, Comparator>::Iterator::Seek(const Key& target) {
	node_ = list_->FindGreaterOrEqual(target, NULL);
}
template<typename Key, typename Value, class Comparator>
inline void MultiSkipListKVDisk<Key, typename Value, Comparator>::Iterator::SeekToFirst() {
	node_ = list_->head_->Next(0, list_);
}
template<typename Key, typename Value, class Comparator>
inline void MultiSkipListKVDisk<Key, typename Value, Comparator>::Iterator::SeekToLast() {
	node_ = list_->FindLast();
	if (node_ == list_->head_) {
		node_ = NULL;
	}
}

template<typename Key, typename Value, class Comparator>
int MultiSkipListKVDisk<Key, typename Value, Comparator>::RandomHeight() {
	static const unsigned int kBranching = 4;
	int height = 1;
	while (height < kMaxHeight && ((parent_->rnd_() % kBranching) == 0)) {
		height++;
	}
	return height;
}
template<typename Key, typename Value, class Comparator>
bool MultiSkipListKVDisk<Key, typename Value, Comparator>::KeyIsAfterNode(const Key& key, Node* n) {
	//return (n != NULL) && (compare_(n->key, key) < 0);//==
	return (n != NULL) && (parent_->compare_(parent_->da_.get_absolute_pos(n)->key, key) < 0);
}
template<typename Key, typename Value, class Comparator>
typename MultiSkipListKVDisk<Key, Value, Comparator>::Node* MultiSkipListKVDisk<Key, Value, Comparator>::FindGreaterOrEqual(const Key& key, Node** prev) {
	Node* x = head_;
	int level = *(parent_->da_.get_absolute_pos(max_height_)) - 1;
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
typename MultiSkipListKVDisk<Key, typename Value, Comparator>::Node*
	MultiSkipListKVDisk<Key, Value, Comparator>::FindLessThan(const Key& key) {
		Node* x = head_;
		int level = *(parent_->da_.get_absolute_pos(max_height_)) - 1;
		while (true)
		{
			Node* next = x->Next(level, this);
			//if (next == NULL || compare_(next->key, key) >= 0) {
			if (next == NULL || parent_->compare_(parent_->da_.get_absolute_pos(next)->key, key) >= 0) {
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
typename MultiSkipListKVDisk<Key, Value, Comparator>::Node* MultiSkipListKVDisk<Key, Value, Comparator>::FindLast()
{
	Node* x = head_;
	int level = *(parent_->da_.get_absolute_pos(max_height_)) - 1;
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
void MultiSkipListKVDisk<Key, Value, Comparator>::Insert(const Key& key, Value& value) {
	Node* prev[kMaxHeight];
	Node* x = FindGreaterOrEqual(key, prev);
	//不允许重复,重复的问题：反向遍历只遍历到一个值
	if (x != NULL && Equal(parent_->da_.get_absolute_pos(x)->key, key))
		return;
	
	int height = RandomHeight();
	int *max_height = parent_->da_.get_absolute_pos(max_height_);
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
	int *size = parent_->da_.get_absolute_pos(size_);
	++*size;
}
template<typename Key, typename Value, class Comparator>
bool MultiSkipListKVDisk<Key, Value, Comparator>::Erase(const Key& key) {
	Node* prev[kMaxHeight] = { 0 };
	Node* x = FindGreaterOrEqual(key, prev);
	if (x != NULL && Equal(key, parent_->da_.get_absolute_pos(x)->key))
	{
		int max_height = *(parent_->da_.get_absolute_pos(max_height_));
		for (int i = 0; i < max_height; i++)
		{
			if (prev[i] != NULL && prev[i]->Next(i, this) == x)
			{
				prev[i]->SetNext(i, x->Next(i, this), this);
			}
		}
		int *size = parent_->da_.get_absolute_pos(size_);
		--*size;
		//delete[] (char*)x;//==
		return true;
	}
	return false;
}
template<typename Key, typename Value, class Comparator>
int MultiSkipListKVDisk<Key, Value, Comparator>::Size()
{
	int *size = parent_->da_.get_absolute_pos(size_);
	return *size;
}
template<typename Key, typename Value, class Comparator>
bool MultiSkipListKVDisk<Key, Value, Comparator>::Contains(const Key& key) {
	Node* x = FindGreaterOrEqual(key, NULL);
	if (x != NULL && Equal(key, parent_->da_.get_absolute_pos(x)->key)) {
		return true;
	}
	else
	{
		return false;
	}
}

template<typename Key, typename Value, class Comparator>
void MultiSkipListKVDisk<Key, Value, Comparator>::ReMapFun(void* para)
{
	//使用相对地址，不再需要
}

template<typename Key, typename Value, class Comparator>
class MultiSkipListMgr{
public:
	int Init(const char* filename, int length, int expend_length);
	int GetSkipList(int skiplist_id, MultiSkipListKVDisk<Key, Value, Comparator> *sl);
	int CreateSkipList(int skiplist_id);
	void EraseSkipList(int skiplist_id);
	int Close();
	int ReBuild();
	template <typename Fun>
	void ScanElement(Fun f)
	{
		SkipListKVDisk<int, MultiSkipListKVDisk<Key, Value, Comparator>*, IntGreater>::Iterator iter(&sl_mgr_);
		for (iter.SeekToFirst(); iter.Valid(); iter.Next())
		{
			MultiSkipListKVDisk<Key, Value, Comparator> mskv;
			int *max_height = (int*)iter.value();
			mskv.max_height_ = max_height;
			mskv.size_ = max_height + 1;
			mskv.head_ = (MultiSL::Node *)(max_height+2);
			mskv.parent_ = this;
			f(iter.key(), mskv);
		}
	}
	class IntGreater
	{
	public:
		int operator()(int a, int b)const
		{
			if (a > b) return 1;
			if (a < b) return -1;
			return 0;
		}
	};

private:
	Comparator compare_;
	std::random_device rnd_;
	disk_alloc da_;
	typedef MultiSkipListKVDisk<Key, Value, Comparator> MultiSL;
	SkipListKVDisk<int, MultiSL*, IntGreater> sl_mgr_; 
	template<typename Key, typename Value, class Comparator>
	friend class MultiSkipListKVDisk;
};
template<typename Key, typename Value, class Comparator>
int MultiSkipListMgr<Key, Value, Comparator>::Init(const char* filename, int length, int expend_length)
{
	char fullfilename[MAX_PATH] = {0};
	const char *mid_pos = strrchr(filename, '/');
	if (mid_pos == NULL)
		mid_pos = filename;
	else
		mid_pos++;
	strncpy(fullfilename, filename, mid_pos-filename);
	strcat(fullfilename, "index_");
	strcat(fullfilename, mid_pos);
	int ret = da_.init(fullfilename, length, expend_length);
	if (ret) return ret;
	memset(fullfilename, 0, sizeof(fullfilename));
	strncpy(fullfilename, filename, mid_pos-filename);
	strcat(fullfilename, "indexmgr_");
	strcat(fullfilename, mid_pos);
	ret = sl_mgr_.Init(fullfilename, 0, 1024);
	if (ret) return ret;
	compare_ = Comparator();
	return ret;
}
template<typename Key, typename Value, class Comparator>
int MultiSkipListMgr<Key, Value, Comparator>::GetSkipList(int skiplist_id, MultiSkipListKVDisk<Key, Value, Comparator>* sl)
{
	SkipListKVDisk<int, MultiSL*, IntGreater>::Iterator iter(&sl_mgr_);
	iter.Seek(skiplist_id);
	if (iter.Valid() && iter.key() == skiplist_id)
	{
		int *max_height = (int*)iter.value();
		sl->max_height_ = max_height;
		sl->size_ = max_height + 1;
		sl->head_ = (MultiSL::Node *)(max_height+2);
		sl->parent_ = this;
		return 0;
	}
	else
	{
		return -1;
	}
}
template<typename Key, typename Value, class Comparator>
int MultiSkipListMgr<Key, Value, Comparator>::CreateSkipList(int skiplist_id)
{
	SkipListKVDisk<int, MultiSL*, IntGreater>::Iterator iter(&sl_mgr_);
	iter.Seek(skiplist_id);
	if (iter.Valid() && iter.key() == skiplist_id)
		return -1;
	//     4    |       x
	//max_height|     Node
	int *max_height = (int*)da_.my_relative_alloc(sizeof(int));
	int *absolute_max_height = da_.get_absolute_pos(max_height);
	*absolute_max_height = 1;
	int *size = (int*)da_.my_relative_alloc(sizeof(int));
	int *absolute_size = da_.get_absolute_pos(size);
	*absolute_size = 0;
	MultiSL::Node *head = (MultiSL::Node *)da_.my_relative_alloc(sizeof(MultiSL::Node) + sizeof(void*) * (MultiSL::kMaxHeight - 1));
	MultiSL::Node *absolute_head = da_.get_absolute_pos(head);
	absolute_head->key = 0;
	absolute_head->value = 0;
	MultiSL *pMultiSL = (MultiSL*)max_height;
	MultiSkipListKVDisk<Key, Value, Comparator> sl;
	sl.max_height_ = max_height;
	sl.head_ = head;
	sl.parent_ = this;
	sl.size_ = size;
	for (int i = 0; i < MultiSL::kMaxHeight; i++) {
		head->SetNext(i, NULL, &sl);
	}
	sl_mgr_.Insert(skiplist_id, pMultiSL);
	return 0;
}
template<typename Key, typename Value, class Comparator>
void MultiSkipListMgr<Key, Value, Comparator>::EraseSkipList(int skiplist_id)
{
	sl_mgr_.Erase(skiplist_id);
}
template<typename Key, typename Value, class Comparator>
int MultiSkipListMgr<Key, Value, Comparator>::Close()
{
	int da_ret = da_.close();
	int sl_ret = sl_mgr_.Close();
	if (da_ret != 0 || sl_ret != 0)
		return -1;
	return 0;
}
template<typename Key, typename Value, class Comparator>
int MultiSkipListMgr<Key, Value, Comparator>::ReBuild()
{
	int ret = 0;
	char file[MAX_PATH];
	char tmpfile[MAX_PATH];
	size_t filesize = da_.size();
	size_t expend_length = da_.get_expend_length();
	const char *filename = strstr(da_.filename(), "index_");
	if (filename == NULL)
		return -1;
	filename += strlen("index_");
	strcpy(file, filename);
	strcpy(tmpfile, file);
	strcat(tmpfile, "_tmp");
	
	MultiSkipListMgr<Key, Value, Comparator> tmpmgr;
	ret = tmpmgr.Init(tmpfile, 0, expend_length);
	if (ret)
		return ret;

	MultiSkipListKVDisk<Key, Value, Comparator> tmpskiplist;
	MultiSkipListKVDisk<Key, Value, Comparator> sl;
	SkipListKVDisk<int, MultiSL*, IntGreater>::Iterator iter(&sl_mgr_);
	
	for (iter.SeekToFirst(); iter.Valid(); iter.Next())
	{
		int id = iter.key();
		int *max_height = (int*)iter.value();
		sl.max_height_ = max_height;
		sl.head_ = (MultiSL::Node *)(max_height+1);
		sl.parent_ = this;
		tmpmgr.CreateSkipList(id);
		tmpmgr.GetSkipList(id, &tmpskiplist);
		MultiSkipListKVDisk<Key, Value, Comparator>::Iterator multi_iter(&sl);
		for (multi_iter.SeekToFirst(); multi_iter.Valid(); multi_iter.Next())
		{
			tmpskiplist.Insert(multi_iter.key(), multi_iter.value());
		}
	}
	ret = tmpmgr.Close();
	if (ret) return ret;
	ret = Close();
	if (ret) return ret;
	char fullfilename[MAX_PATH];
	char tmpfullfilename[MAX_PATH];
	sprintf(fullfilename, "index_%s", file);
	sprintf(tmpfullfilename, "index_%s", tmpfile);
	remove(fullfilename);
	rename(tmpfullfilename, fullfilename);
	sprintf(fullfilename, "indexmgr_%s", file);
	sprintf(tmpfullfilename, "indexmgr_%s", tmpfile);
	remove(fullfilename);
	rename(tmpfullfilename, fullfilename);
	ret = Init(file, 0, expend_length);
	return ret;
}