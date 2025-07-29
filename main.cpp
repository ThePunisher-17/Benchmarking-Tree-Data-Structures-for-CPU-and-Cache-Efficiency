#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <set>
#include <fstream>
#include <numeric>
#include <unordered_map>
#include <string>
#include <sys/resource.h>
#include <cstdlib>
#include <map>

using namespace std;
using namespace chrono;

// ================= Memory Profiler =================
size_t current_allocated_bytes = 0;
size_t peak_allocated_bytes = 0;

void *operator new(std::size_t size) noexcept(false)
{
    current_allocated_bytes += size;
    if (current_allocated_bytes > peak_allocated_bytes)
        peak_allocated_bytes = current_allocated_bytes;
    return malloc(size);
}

void operator delete(void *ptr) noexcept
{
    free(ptr);
}

long get_peak_rss_kb()
{
    struct rusage r;
    getrusage(RUSAGE_SELF, &r);
    return r.ru_maxrss;
}

void report_memory_usage(std::ostream &out = std::cout)
{
    out << "[Memory] Peak Allocated Bytes (C++ counters): " << peak_allocated_bytes << " bytes\n";
    out << "[Memory] Peak RSS (Resident Set Size): " << get_peak_rss_kb() << " KB\n";
}

// ================= Benchmark Struct =================
struct OperationStats
{
    double insert_time = 0;
    double search_time = 0;
    double update_time = 0;
    double delete_time = 0;
    double pred_time = 0;
    double succ_time = 0;
};

// ==================== BST ====================
struct BSTNode
{
    int key;
    BSTNode *left = nullptr, *right = nullptr;
    BSTNode(int k) : key(k) {}
};

BSTNode *bst_insert(BSTNode *root, int key)
{
    if (!root)
        return new BSTNode(key);
    if (key < root->key)
        root->left = bst_insert(root->left, key);
    else if (key > root->key)
        root->right = bst_insert(root->right, key);
    return root;
}

bool bst_search(BSTNode *root, int key)
{
    if (!root)
        return false;
    if (root->key == key)
        return true;
    return bst_search(key < root->key ? root->left : root->right, key);
}

BSTNode *bst_delete(BSTNode *root, int key)
{
    if (!root)
        return nullptr;
    if (key < root->key)
        root->left = bst_delete(root->left, key);
    else if (key > root->key)
        root->right = bst_delete(root->right, key);
    else
    {
        if (!root->left)
        {
            BSTNode *temp = root->right;
            delete root;
            return temp;
        }
        else if (!root->right)
        {
            BSTNode *temp = root->left;
            delete root;
            return temp;
        }
        else
        {
            BSTNode *succ = root->right;
            while (succ->left)
                succ = succ->left;
            root->key = succ->key;
            root->right = bst_delete(root->right, succ->key);
        }
    }
    return root;
}

int bst_predecessor(BSTNode *root, int key, int pred = -1)
{
    if (!root)
        return pred;
    if (key <= root->key)
        return bst_predecessor(root->left, key, pred);
    else
        return bst_predecessor(root->right, key, root->key);
}

int bst_successor(BSTNode *root, int key, int succ = -1)
{
    if (!root)
        return succ;
    if (key >= root->key)
        return bst_successor(root->right, key, succ);
    else
        return bst_successor(root->left, key, root->key);
}

// ==================== AVL Tree ====================
struct AVLNode
{
    int key, height;
    AVLNode *left, *right;
    AVLNode(int k) : key(k), height(1), left(nullptr), right(nullptr) {}
};

int height(AVLNode *node)
{
    return node ? node->height : 0;
}

int balanceFactor(AVLNode *node)
{
    return node ? height(node->left) - height(node->right) : 0;
}

void updateHeight(AVLNode *node)
{
    if (node)
        node->height = 1 + max(height(node->left), height(node->right));
}

AVLNode *rotateRight(AVLNode *y)
{
    AVLNode *x = y->left;
    AVLNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    updateHeight(y);
    updateHeight(x);
    return x;
}

AVLNode *rotateLeft(AVLNode *x)
{
    AVLNode *y = x->right;
    AVLNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    updateHeight(x);
    updateHeight(y);
    return y;
}

AVLNode *avl_insert(AVLNode *node, int key)
{
    if (!node)
        return new AVLNode(key);
    if (key < node->key)
        node->left = avl_insert(node->left, key);
    else if (key > node->key)
        node->right = avl_insert(node->right, key);
    else
        return node;

    updateHeight(node);
    int balance = balanceFactor(node);

    if (balance > 1 && key < node->left->key)
        return rotateRight(node);
    if (balance < -1 && key > node->right->key)
        return rotateLeft(node);
    if (balance > 1 && key > node->left->key)
    {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    if (balance < -1 && key < node->right->key)
    {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

AVLNode *minValueNode(AVLNode *node)
{
    AVLNode *current = node;
    while (current->left != nullptr)
        current = current->left;
    return current;
}

AVLNode *avl_delete(AVLNode *root, int key)
{
    if (!root)
        return root;
    if (key < root->key)
        root->left = avl_delete(root->left, key);
    else if (key > root->key)
        root->right = avl_delete(root->right, key);
    else
    {
        if (!root->left || !root->right)
        {
            AVLNode *temp = root->left ? root->left : root->right;
            delete root;
            return temp;
        }
        AVLNode *temp = minValueNode(root->right);
        root->key = temp->key;
        root->right = avl_delete(root->right, temp->key);
    }

    updateHeight(root);
    int balance = balanceFactor(root);

    if (balance > 1 && balanceFactor(root->left) >= 0)
        return rotateRight(root);
    if (balance > 1 && balanceFactor(root->left) < 0)
    {
        root->left = rotateLeft(root->left);
        return rotateRight(root);
    }
    if (balance < -1 && balanceFactor(root->right) <= 0)
        return rotateLeft(root);
    if (balance < -1 && balanceFactor(root->right) > 0)
    {
        root->right = rotateRight(root->right);
        return rotateLeft(root);
    }

    return root;
}

bool avl_search(AVLNode *root, int key)
{
    if (!root)
        return false;
    if (root->key == key)
        return true;
    return avl_search(key < root->key ? root->left : root->right, key);
}

int avl_predecessor(AVLNode *root, int key, int pred = -1)
{
    if (!root)
        return pred;
    if (key <= root->key)
        return avl_predecessor(root->left, key, pred);
    else
        return avl_predecessor(root->right, key, root->key);
}

int avl_successor(AVLNode *root, int key, int succ = -1)
{
    if (!root)
        return succ;
    if (key >= root->key)
        return avl_successor(root->right, key, succ);
    else
        return avl_successor(root->left, key, root->key);
}

// ==================== Treap ====================
struct TreapNode
{
    int key, priority;
    TreapNode *left, *right;
    TreapNode(int k) : key(k), priority(rand()), left(nullptr), right(nullptr) {}
};

TreapNode *treap_rotateRight(TreapNode *y)
{
    TreapNode *x = y->left;
    TreapNode *T2 = x->right;
    x->right = y;
    y->left = T2;
    return x;
}

TreapNode *treap_rotateLeft(TreapNode *x)
{
    TreapNode *y = x->right;
    TreapNode *T2 = y->left;
    y->left = x;
    x->right = T2;
    return y;
}

TreapNode *treap_insert(TreapNode *root, int key)
{
    if (!root)
        return new TreapNode(key);
    if (key < root->key)
    {
        root->left = treap_insert(root->left, key);
        if (root->left->priority > root->priority)
            root = treap_rotateRight(root);
    }
    else if (key > root->key)
    {
        root->right = treap_insert(root->right, key);
        if (root->right->priority > root->priority)
            root = treap_rotateLeft(root);
    }
    return root;
}

TreapNode *treap_delete(TreapNode *root, int key)
{
    if (!root)
        return root;
    if (key < root->key)
        root->left = treap_delete(root->left, key);
    else if (key > root->key)
        root->right = treap_delete(root->right, key);
    else
    {
        if (!root->left)
        {
            TreapNode *temp = root->right;
            delete root;
            return temp;
        }
        else if (!root->right)
        {
            TreapNode *temp = root->left;
            delete root;
            return temp;
        }
        else
        {
            if (root->left->priority > root->right->priority)
            {
                root = treap_rotateRight(root);
                root->right = treap_delete(root->right, key);
            }
            else
            {
                root = treap_rotateLeft(root);
                root->left = treap_delete(root->left, key);
            }
        }
    }
    return root;
}

bool treap_search(TreapNode *root, int key)
{
    if (!root)
        return false;
    if (root->key == key)
        return true;
    return treap_search(key < root->key ? root->left : root->right, key);
}

int treap_predecessor(TreapNode *root, int key, int pred = -1)
{
    if (!root)
        return pred;
    if (key <= root->key)
        return treap_predecessor(root->left, key, pred);
    else
        return treap_predecessor(root->right, key, root->key);
}

int treap_successor(TreapNode *root, int key, int succ = -1)
{
    if (!root)
        return succ;
    if (key >= root->key)
        return treap_successor(root->right, key, succ);
    else
        return treap_successor(root->left, key, root->key);
}

// ==================== Red-Black Tree using std::set ====================
typedef std::set<int> RBTree;

void rbtree_insert(RBTree &tree, int key)
{
    tree.insert(key);
}

void rbtree_delete(RBTree &tree, int key)
{
    tree.erase(key);
}

bool rbtree_search(const RBTree &tree, int key)
{
    return tree.find(key) != tree.end();
}

int rbtree_predecessor(const RBTree &tree, int key)
{
    auto it = tree.lower_bound(key);
    if (it == tree.begin())
        return -1;
    --it;
    return *it;
}

int rbtree_successor(const RBTree &tree, int key)
{
    auto it = tree.upper_bound(key);
    if (it == tree.end())
        return -1;
    return *it;
}

// ==================== vEB Tree ====================
vector<int> vebTree;

void build_veb_layout(const vector<int> &sorted, int low, int high, int index)
{
    if (low > high || index >= (int)vebTree.size())
        return;
    int mid = (low + high) / 2;
    vebTree[index] = sorted[mid];
    build_veb_layout(sorted, low, mid - 1, 2 * index + 1);
    build_veb_layout(sorted, mid + 1, high, 2 * index + 2);
}

bool veb_search(int key)
{
    int i = 0, n = vebTree.size();
    while (i < n && vebTree[i] != -1)
    {
        if (vebTree[i] == key)
            return true;
        i = 2 * i + 1 + (key > vebTree[i]);
    }
    return false;
}

// ==================== Benchmark Dispatcher ====================
OperationStats benchmark_tree(const string &tree_type,
                              const vector<int> &data,
                              const vector<int> &update_data,
                              const vector<int> &delete_data,
                              const vector<int> &pred_succ_data)
{
    OperationStats stats;
    auto t1 = high_resolution_clock::now(), t2 = t1;

    if (tree_type == "BST")
    {
        BSTNode *root = nullptr;
        t1 = high_resolution_clock::now();
        for (int x : data)
            root = bst_insert(root, x);
        t2 = high_resolution_clock::now();
        stats.insert_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : data)
            bst_search(root, x);
        t2 = high_resolution_clock::now();
        stats.search_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : update_data)
        {
            root = bst_delete(root, x);
            root = bst_insert(root, x + 1);
        }
        t2 = high_resolution_clock::now();
        stats.update_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : delete_data)
            root = bst_delete(root, x);
        t2 = high_resolution_clock::now();
        stats.delete_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            bst_predecessor(root, x);
        t2 = high_resolution_clock::now();
        stats.pred_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            bst_successor(root, x);
        t2 = high_resolution_clock::now();
        stats.succ_time = duration_cast<milliseconds>(t2 - t1).count();
    }

    else if (tree_type == "AVL")
    {
        AVLNode *root = nullptr;
        t1 = high_resolution_clock::now();
        for (int x : data)
            root = avl_insert(root, x);
        t2 = high_resolution_clock::now();
        stats.insert_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : data)
            avl_search(root, x);
        t2 = high_resolution_clock::now();
        stats.search_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : update_data)
        {
            root = avl_delete(root, x);
            root = avl_insert(root, x + 1);
        }
        t2 = high_resolution_clock::now();
        stats.update_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : delete_data)
            root = avl_delete(root, x);
        t2 = high_resolution_clock::now();
        stats.delete_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            avl_predecessor(root, x);
        t2 = high_resolution_clock::now();
        stats.pred_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            avl_successor(root, x);
        t2 = high_resolution_clock::now();
        stats.succ_time = duration_cast<milliseconds>(t2 - t1).count();
    }

    else if (tree_type == "Treap")
    {
        TreapNode *root = nullptr;
        t1 = high_resolution_clock::now();
        for (int x : data)
            root = treap_insert(root, x);
        t2 = high_resolution_clock::now();
        stats.insert_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : data)
            treap_search(root, x);
        t2 = high_resolution_clock::now();
        stats.search_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : update_data)
        {
            root = treap_delete(root, x);
            root = treap_insert(root, x + 1);
        }
        t2 = high_resolution_clock::now();
        stats.update_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : delete_data)
            root = treap_delete(root, x);
        t2 = high_resolution_clock::now();
        stats.delete_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            treap_predecessor(root, x);
        t2 = high_resolution_clock::now();
        stats.pred_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            treap_successor(root, x);
        t2 = high_resolution_clock::now();
        stats.succ_time = duration_cast<milliseconds>(t2 - t1).count();
    }

    else if (tree_type == "RB")
    {
        RBTree tree;
        t1 = high_resolution_clock::now();
        for (int x : data)
            rbtree_insert(tree, x);
        t2 = high_resolution_clock::now();
        stats.insert_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : data)
            rbtree_search(tree, x);
        t2 = high_resolution_clock::now();
        stats.search_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : update_data)
        {
            rbtree_delete(tree, x);
            rbtree_insert(tree, x + 1);
        }
        t2 = high_resolution_clock::now();
        stats.update_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : delete_data)
            rbtree_delete(tree, x);
        t2 = high_resolution_clock::now();
        stats.delete_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            rbtree_predecessor(tree, x);
        t2 = high_resolution_clock::now();
        stats.pred_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : pred_succ_data)
            rbtree_successor(tree, x);
        t2 = high_resolution_clock::now();
        stats.succ_time = duration_cast<milliseconds>(t2 - t1).count();
    }

    else if (tree_type == "vEB")
    {
        vector<int> sorted = data;
        sort(sorted.begin(), sorted.end());
        vebTree.assign(sorted.size(), -1);

        t1 = high_resolution_clock::now();
        build_veb_layout(sorted, 0, sorted.size() - 1, 0);
        t2 = high_resolution_clock::now();
        stats.insert_time = duration_cast<milliseconds>(t2 - t1).count();

        t1 = high_resolution_clock::now();
        for (int x : data)
            veb_search(x);
        t2 = high_resolution_clock::now();
        stats.search_time = duration_cast<milliseconds>(t2 - t1).count();

        stats.update_time = 0;
        stats.delete_time = 0;
        stats.pred_time = 0;
        stats.succ_time = 0;
    }

    return stats;
}

// =========================== MAIN ===============================
int main(int argc, char **argv)
{
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <TreeType> <Size> <OutputCSV>\n";
        return 1;
    }

    string tree_type = argv[1];
    int N = stoi(argv[2]);
    string output_file = argv[3];

    srand(42);
    vector<int> data(N);
    iota(data.begin(), data.end(), 0);
    shuffle(data.begin(), data.end(), default_random_engine(42));

    vector<int> update_data(data.begin(), data.begin() + N * 60 / 100);
    vector<int> delete_data(data.begin(), data.begin() + N * 10 / 100);
    vector<int> pred_succ_data(data.begin(), data.begin() + N * 5 / 100);

    OperationStats stats = benchmark_tree(tree_type, data, update_data, delete_data, pred_succ_data);

    // Always run benchmarks, only suppress writing
    if (getenv("DISABLE_CSV_WRITE") == nullptr)
    {
        ofstream out(output_file, ios::app);
        if (!out)
        {
            cerr << "Failed to open " << output_file << endl;
            return 1;
        }

        if (out.tellp() == 0)
            out << "Tree,Size,Insert,Search,Update,Delete,Predecessor,Successor,PeakAllocatedBytes,PeakRSS_KB\n";

        out << tree_type << "," << N << ","
            << stats.insert_time << "," << stats.search_time << ","
            << stats.update_time << "," << stats.delete_time << ","
            << stats.pred_time << "," << stats.succ_time << ","
            << peak_allocated_bytes << "," << get_peak_rss_kb() << "\n";
    }

    report_memory_usage();

    return 0;
}

