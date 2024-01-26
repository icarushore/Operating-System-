#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <mutex>
using namespace std;



pthread_mutex_t mutexForPrint = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexForMalloc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexForDealloc = PTHREAD_MUTEX_INITIALIZER;


struct Node {
    int ID;
    int SIZE;
    int INDEX;
    Node* next;
    Node* previous;

    Node(int id, int size, int index) : ID(id), SIZE(size), INDEX(index), next(nullptr), previous(nullptr){}
};

class HeapManager {
private:
    Node* head; //head of the doubly linked list

public:

    int initHeap(int size) {
        head = new Node(-1,size,0);
        
        pthread_mutex_lock(&mutexForPrint);
        cout << "Memory initialized" << endl;
        cout << "[" << head->ID << "]" << "[" << head ->SIZE << "]" << "["<< head->INDEX << "]" << endl;
        pthread_mutex_unlock(&mutexForPrint);
        return 1;
    }

    int myMalloc(int ID, int size) {
        pthread_mutex_lock(&mutexForMalloc);

        Node* current = head;

        while(current != nullptr){
            
            if(current ->ID == -1 && current->SIZE >= size){
                
                if(size < current ->SIZE){ // the node will be splitted into two
                    Node* newNode = new Node(-1, current->SIZE - size, current->INDEX + size); //allocated memory
                    
                    // current is now the allocated memory
                    current->SIZE = size;
                    current->ID = ID;
                    
                    //updating the pointers 
                    if(current->next != nullptr){
                        newNode-> next = current->next;
                        current->next->previous = newNode;
                    }
                    current->next = newNode;
                    newNode->previous = current;
                    
                    
                }
                else { // no new node needed, all space is allocated 

                    current->ID = ID;        
                }

                cout << "Allocated for thread " << ID << endl;
                print(); // Print the linked list

                pthread_mutex_unlock(&mutexForMalloc);
                return current->INDEX; 
            }
            else{
                if(current->next != nullptr){
                    current = current->next;
                }
            }
            
        }
        // No suitable free node found in the entire linked list
        cout << "Can not allocate, requested size " << size << " for thread " << ID << " is bigger than remaining size" << endl;
        pthread_mutex_unlock(&mutexForMalloc);
        return -1;
    }

    int myFree(int ID, int index) {
        pthread_mutex_lock(&mutexForDealloc);

        
        Node* current = head;
        
        while(current->ID != ID){
            current = current ->next;

            // there are no more nodes in the list
            // and we couldn't find a matching node
            if(current -> next == nullptr){ 
                return -1;
            }
        }
        
        if(current->next != nullptr){
            
            if(current -> next ->ID == -1){
                
                Node* temp = current->next;
                
                
                current->SIZE += current ->next->SIZE;
                if(current->next ->next != nullptr){
                    current -> next = current ->next ->next;
                    current->next->next->previous = current;
                }
                else{
                    current->next = nullptr;
                }
                current-> ID = -1;
                delete temp;
                
            }
            else{
                current-> ID = -1;
            }
        }
        
        if(current->previous != nullptr){
           if(current -> previous ->ID == -1){
                Node* temp = current;

                current->previous -> SIZE += current -> SIZE;
                if(current ->next != nullptr){
                    current -> previous-> next = current -> next;
                    current ->next->previous = current->previous;
                }
                else{
                    current->previous->next = nullptr;
                }
                current->previous->ID = -1;
                delete temp;
            }
            else{
                current-> ID = -1;
            }
        }

        
        cout << "Freed for thread " << ID << endl;
        print();
        pthread_mutex_unlock(&mutexForDealloc);

        return 1;
       
        
    }

    void print() {
        pthread_mutex_lock(&mutexForPrint);
        Node* current = head;

        cout <<"[" << current->ID << "]" << "[" << current->SIZE << "]" << "[" << current ->INDEX << "]";
        while(current->next != nullptr){
            current = current->next;
            cout  << "---" <<"[" << current->ID << "]" << "[" << current->SIZE << "]" << "[" << current ->INDEX << "]";
        }
        cout << endl;
        pthread_mutex_unlock(&mutexForPrint);
    }
};



