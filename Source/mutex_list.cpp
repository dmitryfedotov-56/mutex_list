    
    #define _CRT_SECURE_NO_WARNINGS

    #include <mutex>
    #include <functional>
    #include <iostream>
    using namespace std;

    /***********************************************************/

    #define COUNTERS   1       // counting

#if COUNTERS
    unsigned list_counter = 0;  // number of conflicts for the possession of the list
    unsigned node_counter = 0;  // number of conflicts for the possession of a node
#endif

    /***********************************************************/
    /*                             Node                        */
    /***********************************************************/

    template <typename T>   // template of a node containing data of type T
    class Node {
    private:
         mutex node_mutex;
    public:

        T value;
        Node<T>* next;

        Node(T data) : value(data), next(nullptr) {};

        void lock() 
        {
#if COUNTERS                                       
            bool locked = node_mutex.try_lock();    // in debugging mode 
            if (!locked)  {                         // if locked
                node_counter++;                     // increase counter
                node_mutex.lock();                  // and lock
            };
#else
            node_mutex.lock();
#endif
           
        };

        void unlock() { node_mutex.unlock(); };
    };

    /***********************************************************/
    /*                      List of Nodes                      */
    /***********************************************************/

    template <typename T>   // template of a list of Node<T> 
    class List{
    public:
        List() : head(nullptr) {};
        bool remove(T value, std::function<bool(T, T)> found);
        void insert(T value, std::function<bool(T, T)> found);
        void show();
    private:
        Node<T>* head;
        mutex list_mutex;

        void lock()
        {
#if COUNTERS 
            bool locked = list_mutex.try_lock();    // in debugging mode
            if (!locked) {                          // if locked
                list_counter++;                     // increase counter
                list_mutex.lock();                  // and lock
            };
#else
            list_mutex.lock();
#endif

        };

        void unlock() { list_mutex.unlock(); };
    };

    /***********************************************************/
    /*                      List Implementatin                 */
    /***********************************************************/

    void List<int>::show() 
    {
        Node<int>* curr;
        curr = head;
        unsigned size = 0;
        while (curr != nullptr) 
        {
            std::cout << "value = " << curr->value << std::endl;
            size++;
            curr = curr->next;
        };
        std::cout << "list size = " << size << std::endl;
    };

    /***********************************************************/

    template<typename T>
    void List<T>::insert(T value, std::function<bool(T, T)> found)
    {
        Node<T> * prev, * curr, * node, * temp;

        node = new Node<T>(value);      // create node

        this->lock();                   // lock the the entire list

        prev = this->head;              // the initial node

        if (prev == nullptr)            // if the list is empty
        {
            this->head = node;          // insert node to the head
            this->unlock();             // unlock the entire list;
            return;                     // nothing else to do
        };

        if (found(value, prev->value)) // insert to the top of the list?
        {
            node->next = prev;          // node is linked to prev
            this->head = node;          // node is the initial node 
            this->unlock();             // unlock the entire list;
            return;                     // nothing else to do
        };

        prev->lock();                   // lock initial node
        curr = prev->next;              // the node following the initial one 

        this->unlock();                 // unlock the entire list

        // prev is the initial node and it is locked 
        // curr is the node following the initial

        while (curr != nullptr)         // until the end of thre list
        {
            curr->lock();    // lock curr

            if (found(value, curr->value))  // insert before curr?
            {
                prev->next = node;          // prev is linked to the node
                node->next = curr;          // node is linked to the curr
                prev->unlock();             // unlock
                curr->unlock();             // both of them
                return;                     // nothing else to do
            };

            // go to the next node

            temp = prev;                    // store prev
            prev = curr;                    // to the next
            curr = curr->next;              // pair of nodes
            temp->unlock();                 // unlock old prev
        };

        // position is not found, insert to the end

        prev->next = node;                  // insert to the end of the list
        prev->unlock();                     // unlock prev
     };

    /***********************************************************/

    template<typename T>
    bool List<T>::remove(T value, std::function<bool(T, T)> found)
    {
        Node<T> * prev, * curr, * temp;

        this->lock();                   // lock the the entire list

        prev = this->head;              // initial node

        if (prev == nullptr)            // if the list is empty
        {
            this->unlock();             // unlock the entire list;
            return false;               // nothing else to do
        };

        prev->lock();                   // lock initial node
        curr = prev->next;              // // the node following the initial one
  
        if (found(value, prev->value))  // deleting the initial node?
        {
            this->head = curr;          // now curr is the intial node         
            prev->unlock();             // unlocking the node being deleted
            delete prev;                // delete node
            this->unlock();             // unlocking the entire list
            return true;                // nothing else to do
        };

        this->unlock();                 // unlock the the entire list

        // delete a node that is not the initial one
        // prev is the initial node and it is locked 
        // curr is the node following the initial

        while (curr != nullptr)         // until the end of thre list
        {
            curr->lock();                   // lock curr
            
            if(found(value, curr->value))   // deleting curr?
            {
                prev->next = curr->next;    // prev is linked to the next node
                prev->unlock();             // unlock
                curr->unlock();             // both of them
                delete curr;                // delete curr
                return true;                // nothing else to do
            };

            // go to the next node

            temp = prev;                    // store old prev
            prev = curr;                    // to the next
            curr = curr->next;              // pair of nodes
            temp->unlock();                 // unlock old prev
        };
        prev->unlock();
        return false;
    };

    /***********************************************************/
    /*                             Test                        */
    /***********************************************************/

    #define DEBUG   0

    List<int> list;

    #define INIT    10    
    #define SIZE    1000 

    #include <string>
    #include <cstdlib>
    #include <ctime>

#if COUNTERS
    unsigned insert_cntr = 0;   // number of inserted nodes
    unsigned remove_cntr = 0;   // number of removed nodes
    unsigned not_found = 0;     // number of nodes that were not found 
    unsigned after_finish = 0;  // number of nodes deleted after adder finish
#endif
 
    /***********************************************************/

    class Pool {    // pool class
    private:
        int p = 0;  // index to put
        int g = 0;  // index to get
        int* val;   // array of values
    public:
        Pool() { val = new int[INIT + SIZE];};

        void put(int n) { val[p++] = n; };

        int get() { if(g < p) return val[g++]; else return -1; };
    };

    Pool pool;

    /***********************************************************/
#if DEBUG
    std::mutex console_mutex;

    void safe_Output(std::string s) 
    {
        unique_lock<std::mutex> l(console_mutex);
        std::cout << s << std::endl;
    };
#endif
    /***********************************************************/

    static bool finished = false;

    void add_Nodes(unsigned numb)           // adder thread
    {
        int n; unsigned size; std::string s;
        for (int i = 0; i < numb; i++)      // insert in ascending order
        {
            n = rand() % 100;               // random value
#if DEBUG
            s = "inserting " + to_string(n);
            safe_Output(s);
#endif
            list.insert(n, [](int x, int y) { return y >= x; }); // insert in ascending order
            pool.put(n);                                         // add to the pool 
#if COUNTERS
            insert_cntr++;
#endif
        };
        finished = true;
#if DEBUG
        safe_Output("adder finish");
#endif
    };

    /***********************************************************/

    void remove_Nodes(unsigned numb)             // remover thread
    {
        bool removed; int n; std::string s;

        unsigned cntr = 0;

        while(cntr < numb)       
        {
            n = pool.get();                     // get from the pool
#if DEBUG
            s = "removing " + to_string(n);
            safe_Output(s);
#endif
            // remove from the list
            removed = list.remove(n, [](int x, int y) { return x == y; });
            if (!removed) {
#if COUNTERS
                not_found++;
#endif
            } else{
                cntr++;
                if (finished)after_finish++;
#if COUNTERS
                remove_cntr++;
#endif
            };
        };
    };

    /***********************************************************/

    #include <conio.h>

    int main() 
    {
        srand(unsigned(time(0)));   // starting position for the random number generator

        add_Nodes(INIT);        // add 10 nodes
 
        std::cout << "starting test" << std::endl;
 
        finished = false;
        std::thread adder(add_Nodes, SIZE);          // starting a thread adding SIZE nodes
        std::thread remover(remove_Nodes, SIZE);     // starting a thread removing SIZE nodes

        adder.join();           // wait for adder completion
        remover.join();         // wait for remover completion
  
        list.show();            // show the list after execution

#if COUNTERS
        std::cout << "inserted nodes  : " << insert_cntr << std::endl;
        std::cout << "removed nodes   : " << remove_cntr << std::endl;
        std::cout << "not found nodes : " << not_found << std::endl;
        std::cout << "after finish    : " << after_finish << std::endl;

        std::cout << "list counter    : " << list_counter << std::endl;
        std::cout << "node counter    : " << node_counter << std::endl;
#endif
        std::cout << "test finished, press any key" << std::endl;
        char c = _getch();


        return 0;
    }; 

    /***********************************************************/



