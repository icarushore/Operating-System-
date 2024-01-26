#include <iostream>
#include <pthread.h>
#include <mutex>
#include <string>
#include <vector>
#include <semaphore.h>
using namespace std;

pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER; //initialize the mutex
sem_t teamA; sem_t teamB; 
pthread_barrier_t teamBarrier;
int fanOfTeamA = 0; int fanOfTeamB = 0; int carID = 0;  // to use inside of the function
pthread_t captain; //captain of the team


void* initTeamA(void* tid){
    
    pthread_mutex_lock(&myMutex);
    cout << "Thread ID: " << (pthread_t)tid << ", Team: A, I am looking for a car" << endl;
    fanOfTeamA++;
    


    
    // check for valid combination
    if (fanOfTeamA == 4) {
        // valid combination, wake up the remaining 3 members
        captain = (pthread_t) tid;
        
        
        sem_post(&teamA);
        sem_post(&teamA);
        sem_post(&teamA);
        
        fanOfTeamA = 0;
        
    
    }
    else if((fanOfTeamB >= 2 && fanOfTeamA >= 2)){
        captain = (pthread_t) tid;
        

        sem_post(&teamB);
        sem_post(&teamB);
        sem_post(&teamA);

        
        fanOfTeamA -= 2;
        fanOfTeamB -=2;
        
        
    }
    else{
        pthread_mutex_unlock(&myMutex);
        sem_wait(&teamA);
    }
  
    
    
    //print that they found a spot in the car
    
    
    cout << "Thread ID: " << (pthread_t)tid << ", Team: A, I have found a spot in a car" << endl;
   
    

    //barrier:
    pthread_barrier_wait(&teamBarrier);
    //phase 2: choose a driver and print that
    //1st to arrive is the captain of the team
    if(pthread_t(tid) == captain){
        
        cout << "Thread ID: " << to_string(captain) << ", Team: A, I am the captain and driving the car with ID " << carID << endl;
        captain = 0;
        carID++;
        pthread_mutex_unlock(&myMutex);
    }

    return nullptr;
}

void* initTeamB(void* tid){
    
    pthread_mutex_lock(&myMutex);
    cout << "Thread ID: " << to_string((pthread_t)tid) << ", Team: B, I am looking for a car" << endl;
    fanOfTeamB++;
    
    // check for valid combination
    if (fanOfTeamB == 4) {
        captain = (pthread_t) tid;
        
        // valid combination, wake up the remaining 3 members
        sem_post(&teamB);
        sem_post(&teamB);
        sem_post(&teamB);

        fanOfTeamB = 0;
        
    }
    else if((fanOfTeamA >= 2 && fanOfTeamB >= 2)){
        //// valid combination, wake up the remaining 3 members
        captain = (pthread_t) tid;
        
        sem_post(&teamA);
        sem_post(&teamA);
        sem_post(&teamB);

        fanOfTeamA -= 2;
        fanOfTeamB -=2;
        
    }
    else{
        pthread_mutex_unlock(&myMutex);
        sem_wait(&teamB);
    }
    
    
    //print that they found a spot in the car
    
    cout << "Thread ID: " << to_string((pthread_t)tid) << ", Team: B, I have found a spot in a car" << endl;
    //barrier:
    pthread_barrier_wait(&teamBarrier);

    
    if(pthread_t(tid) == captain){
        
        cout << "Thread ID: " << to_string(captain) << ", Team: B, I am the captain and driving the car with ID "<< carID << endl;
        captain = 0;
        carID++;
        pthread_mutex_unlock(&myMutex);
    }
    
    return nullptr;
}


int main(int argc, char* argv[]){

    // Convert command-line arguments to integers
    int numA = stoi(argv[1]);
    int numB = stoi(argv[2]);

    // Initialize semaphores
    sem_init(&teamA, 0, 0);
    sem_init(&teamB, 0, 0);
    

    //Initialize barriers
    pthread_barrier_init(&teamBarrier, nullptr, 4);
    
    
    bool isValid = true;
    //Each group size must be even
    if(numA%2 != 0){
        isValid = false;
    }
    if(numB%2 != 0){
        isValid = false;
    }
    //Total # of supporters must be multiply of four
    if((numA+numB)%4 != 0){
        isValid = false;
    }

    if(isValid){
        vector<pthread_t> teamA_threads(numA);
        vector<pthread_t> teamB_threads(numB);

        

        for (int i = 0; i < numA; ++i) {
            if (pthread_create(&teamA_threads[i], nullptr, initTeamA, &teamA_threads[i]) != 0) {
                cerr << "Error creating Team A thread" << endl;
                exit(1);
            }
        }
        for (int i = 0; i < numB; ++i) {
            if (pthread_create(&teamB_threads[i], nullptr, initTeamB, &teamB_threads[i]) != 0) {
                cerr << "Error creating Team B thread" << endl;
                exit(1);
            }
        }
        
        
        for (int i = 0; i < numA; ++i) {
            pthread_join(teamA_threads[i], nullptr);
        }

        for (int i = 0; i < numB; ++i) {
            pthread_join(teamB_threads[i], nullptr);
        }
    }

   

    cout << "The main terminates" << endl;
    
    
    return 0;
}

















