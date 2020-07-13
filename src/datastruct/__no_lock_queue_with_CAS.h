#ifndef __NO_LOCK_QUEUE_WITH_CAS
#define __NO_LOCK_QUEUE_WITH_CAS

#include <atomic>
#include <iostream>

template<typename T>
class __pointer_to_entry{
    typedef unsigned long       size_t;
    typedef T*                  _ptr;
    public:
        _ptr                    ptr;
        size_t                  counter;

        __pointer_to_entry(_ptr _p=NULL,
                            size_t _c=0) noexcept
                            :ptr(_p),
                             counter(_c){};

        bool operator==(const __pointer_to_entry& _p) const{
            return _p.ptr==ptr&&_p.counter==counter;
        }

        __pointer_to_entry(const __pointer_to_entry& _p)=default;
        __pointer_to_entry& operator=(const __pointer_to_entry& _p)=default;
};

template<typename T>
class __entry_in_queue{
    public:
        T                                         value;
        __pointer_to_entry<__entry_in_queue<T>>   next; 
};

template<typename T>
static bool CAS(T& _ptr,T _oldv,T _newv){
    std::atomic<T> _atom;
    _atom.store(_ptr);
    bool _res=atomic_compare_exchange_weak(&_atom,&_oldv,_newv); 
    _ptr=_atom.load();
    return _res;
}

template<typename T>
class __no_lock_queue_with_CAS{
    typedef T                            Type;
    typedef __entry_in_queue<T>          Entry;
    typedef __pointer_to_entry<Entry>    Ptr;
    public:
        __no_lock_queue_with_CAS(){
            Entry *node=new Entry();
            node->next.ptr=NULL;
            Head.ptr=node;
            Tail.ptr=node;
            _size=0;
        }
        ~__no_lock_queue_with_CAS(){
            T   tmp;
            while(_size!=0){
                dequeue(tmp);
            }
        }
        int                     size();
        void                    enqueue(T _value);
        bool                    dequeue(T& _res);
        Ptr                     Head;
        Ptr                     Tail;
    private:
        std::atomic<int>         _size;
};

template<typename T>
void __no_lock_queue_with_CAS<T>::enqueue(T _value){
    Ptr         tail,next;
    Entry *node=new Entry();
    node->value=_value;
    node->next.ptr=NULL;
    while(1){
        tail=Tail;
        next=tail.ptr->next;
        if(tail==Tail){
            if(next.ptr==NULL){
                if(CAS<Ptr>(tail.ptr->next,next,Ptr(node,next.counter+1)))
                    break;
            }
            else{
                CAS<Ptr>(Tail,tail,Ptr(next.ptr,tail.counter+1));
            }
        }
    }

    CAS<Ptr>(Tail,tail,Ptr(node,tail.counter+1));
    _size++;
}

template<typename T>
bool __no_lock_queue_with_CAS<T>::dequeue(T& _res){
    Ptr     head,tail,next,*nodep;
    while(1){
        head=Head;
        tail=Tail;
        next=head.ptr->next;
        if(head==Head){
            if(head.ptr==tail.ptr){
                if(next.ptr==NULL)
                    return false;
                CAS<Ptr>(Tail,tail,Ptr(next.ptr,tail.counter+1));
            }
            else{
                _res=next.ptr->value;
                if(CAS<Ptr>(Head,head,Ptr(next.ptr,head.counter+1)))
                    break;
            }
        }
    }
    delete head.ptr;
    _size--;
    return true;
}

template<typename T>
int __no_lock_queue_with_CAS<T>::size(){
    return _size;
}
#endif