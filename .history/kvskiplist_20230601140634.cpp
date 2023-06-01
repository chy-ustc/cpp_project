#include <iostream>
#include <string>
#include <set>
#include <time.h>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>
using namespace std;
template <typename T>
struct Less
{
    bool operator()(const T &a, const T &b) const
    {
        return a < b;
    }
};
template <typename K, typename V, typename Comp = Less<K>>
class skip_list
{
private:
    struct skip_list_node
    {
        int level;
        const K key;
        V value;
        skip_list_node **forward;
        skip_list_node() : key{0}, value{0}, level{0}, forward{0} {}
        skip_list_node(K k, V v, int l, skip_list_node *nxt = nullptr) : key(k), value(v), level(l)
        {
            forward = new skip_list_node *[level + 1];
            for (int i = 0; i <= level; ++i)
                forward[i] = nxt;
        }
        ~skip_list_node() { delete[] forward; }
    };
    using node = skip_list_node;
    void init()
    {
        srand((uint32_t)time(NULL));
        level = length = 0;
        head->forward = new node *[MAXL + 1];
        for (int i = 0; i <= MAXL; i++)
            head->forward[i] = tail;
    }
    int randomLevel()
    {
        int lv = 1;
        while ((rand() & S) < PS)
            ++lv;
        return MAXL > lv ? lv : MAXL;
    }
    int level;
    int length;
    static const int MAXL = 32;
    static const int P = 4;
    static const int S = 0xFFFF;
    static const int PS = S / P;
    static const int INVALID = INT_MAX;
    node *head, *tail;
    Comp less;
    node *find(const K &key, node **update)
    {
        node *p = head;
        for (int i = level; i >= 0; i--)
        {
            while (p->forward[i] != tail && less(p->forward[i]->key, key))
            {
                p = p->forward[i];
            }
            update[i] = p;
        }
        p = p->forward[0];
        return p;
    }

public:
    struct Iter
    {
        node *p;
        Iter() : p(nullptr){};
        Iter(node *rhs) : p(rhs) {}
        node *operator->() const { return (p); }
        node &operator*() const { return *p; }
        bool operator==(const Iter &rhs) { return rhs.p == p; }
        bool operator!=(const Iter &rhs) { return !(rhs.p == p); }
        void operator++() { p = p->forward[0]; }
        void operator++(int) { p = p->forward[0]; }
    };

    skip_list() : head(new node()), tail(new node()), less{Comp()}
    {
        init();
    }
    skip_list(Comp _less) : head(new node()), tail(new node()), less{_less}
    {
        init();
    }
    void insert(const K &key, const V &value)
    {
        node *update[MAXL + 1];
        node *p = find(key, update);
        if (p->key == key)
        {
            p->value = value;
            return;
        }
        int lv = randomLevel();
        if (lv > level)
        {
            lv = ++level;
            update[lv] = head;
        }
        node *newNode = new node(key, value, lv);
        for (int i = lv; i >= 0; --i)
        {
            p = update[i];
            newNode->forward[i] = p->forward[i];
            p->forward[i] = newNode;
        }
        ++length;
    }

    bool erase(const K &key)
    {
        node *update[MAXL + 1];
        node *p = find(key, update);
        if (p->key != key)
            return false;
        for (int i = 0; i <= p->level; ++i)
        {
            update[i]->forward[i] = p->forward[i];
        }
        delete p;
        while (level > 0 && head->forward[level] == tail)
            --level;
        --length;
        return true;
    }
    Iter find(const K &key)
    {
        node *update[MAXL + 1];
        node *p = find(key, update);
        if (p == tail)
            return tail;
        if (p->key != key)
            return tail;
        return Iter(p);
    }
    bool count(const K &key)
    {
        node *update[MAXL + 1];
        node *p = find(key, update);
        if (p == tail)
            return false;
        return key == p->key;
    }
    Iter end()
    {
        return Iter(tail);
    }
    Iter begin()
    {
        return Iter(head->forward[0]);
    }
};

// 修改后的 skip_list 类，在原有基础上添加了互斥量以支持线程安全
template <typename K, typename V, typename Comp = Less<K>>
class thread_safe_skip_list : public skip_list<K, V, Comp>
{
private:
    std::mutex mtx;

public:
    thread_safe_skip_list() : skip_list<K, V, Comp>() {}
    thread_safe_skip_list(Comp _less) : skip_list<K, V, Comp>(_less) {}
    void insert(const K &key, const V &value)
    {
        std::unique_lock<std::mutex> lock(mtx);
        skip_list<K, V, Comp>::insert(key, value);
    }
    bool erase(const K &key)
    {
        std::unique_lock<std::mutex> lock(mtx);
        return skip_list<K, V, Comp>::erase(key);
    }
    // ... 如果需要，为其他成员函数添加互斥量 ...
};

// 多线程测试函数
void test_thread(thread_safe_skip_list<int, int> &list)
{
    for (int i = 0; i < 1000; i++)
    {
        list.insert(rand() % 100, rand());
    }
}

int main()
{
    // 高并发测试
    {
        thread_safe_skip_list<int, int> list;
        const int num_threads = 10;
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; i++)
        {
            threads.emplace_back(test_thread, std::ref(list));
        }
        for (auto &t : threads)
        {
            t.join();
        }
        
        for (auto it = list.begin(); it != list.end(); it++)
        {
            
        }
    }
    /*
    {
        //使用lambda
        auto cmp = [](const string& a, const string& b) {return a.length() < b.length(); };
        skip_list < string, int, decltype(cmp)> list(cmp);
        list.insert("aab", 1321);
        list.insert("hello", 54342);
        list.insert("world", 544);
        for (auto it = list.begin(); it != list.end(); it++) {
            cout << it->key << " " << it->value << endl;
        }
    }

    cout << "==================================" << endl;

    {
        //使用仿函数
        struct cmp {
            bool operator()(int a, int b) {
                return a > b;
            }
        };
        skip_list < int, int, cmp> list{};
        for (int i = 1; i <= 10; i++)list.insert(rand()%20, rand());
        for (auto it = list.find(10); it != list.end(); it++) {
            cout << it->key << " " << it->value << endl;
        }
    }

    cout << "==================================" << endl;

    {
        //默认小于号
        skip_list<int, int>list;
        list.insert(1, 3);
        list.insert(1, 3);
        list.insert(4, 3);
        list.insert(5, 3);
        list.insert(1, 3);
        list.insert(4, 3);
        for (auto it = list.begin(); it != list.end();it++) {
            cout << it->key << " " << it->value << endl;
        }

    }

    {
        //可以添加 T && 实现move语义
        //添加重载 []
    }
    */
}