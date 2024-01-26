#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>

using namespace std;

mutex myMutex;
vector<pthread_t> threadIDVector; // to store the thread ids
vector<int> processIDVector; // to store process ids for the background part


struct ThreadArgs {
    int fdToRead;
    mutex& myMutex;
};

void* ListenerThread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    FILE* pipeFile = fdopen(args->fdToRead, "r"); // get the read end

    args->myMutex.lock(); // lock the critical section
    cout << "----  " << pthread_self() << endl;


    char buff[1024];
    while (fgets(buff, sizeof(buff), pipeFile) != nullptr) { // go till EOF
        cout << buff;
        cout.flush();
    }
    cout << "----  " << pthread_self() << endl;


    args->myMutex.unlock(); // unlock the critical section once the job is finished
    fclose(pipeFile);

    return nullptr;
}

void parseAndPrintCommandToFile(const string &commandLine, ofstream &outputFile, vector<string> &command,char& backgroundJob) {
    // Output 10 dashes to indicate the start of processing a new command
    outputFile << "----------\n";

    istringstream iss(commandLine);
    string info, commandName, input = "", option = "", redirection = "", fileLocation = "";
    backgroundJob = 'n';

    // Process each token
    int count = 0;
    while (iss >> info) {
        if (count == 0) {
            // First token is the command name
            commandName = info;
            count++;
        } else if (info == "<" || info == ">") {
            // Redirection symbol found
            redirection = info;
            iss >> info;
            fileLocation = info;
        } else if (info[0] == '-') {
            // Option found
            option = info;
        } else if (info.find('&') != string::npos) {
            // Background job symbol found
            backgroundJob = 'y';
        } else {
            // Command or input found
            if (input.empty()) {
                input = info;
            }
        }
    }

    // Handle the case when there is no redirection symbol
    if (redirection != "<" && redirection != ">") {
        redirection = "-";
    }
    command.push_back(commandName);    // index 0
    command.push_back(input);          // index 1
    command.push_back(option);         // index 2
    command.push_back(redirection);    // index 3
    command.push_back(fileLocation);   // index 4

    // Print the parsed information to the file
    outputFile << "Command : " << commandName << "\n";
    outputFile << "Inputs: " << input << "\n";
    outputFile << "Options: " << option << "\n";
    outputFile << "Redirection : " << redirection << "\n";
    outputFile << "Background Job : " << backgroundJob << "\n";

    // Output 10 dashes to indicate the end of processing the command
    outputFile << "----------\n";
    outputFile.flush();
}

int main() {
    ifstream inputFile;
    inputFile.open("commands.txt"); // open the input stream
    string commandLine;

    // Open the output file
    ofstream outputFile("parse.txt");

    vector<vector<string>> commandVector; // stores commands
    char backgroundJob;
    

    // Read and process commands
    if (!inputFile.is_open()) {
        cerr << "Error opening commands.txt" << endl;
        return 1; // or handle the error appropriately
    }

    while (getline(inputFile, commandLine)) {
        vector<string> command;
        // parse and print the arguments
        parseAndPrintCommandToFile(commandLine, outputFile, command,backgroundJob);
        commandVector.push_back(command);
    }
    inputFile.close();

    for (int i = 0; i < commandVector.size(); i++) {
        vector<string> command = commandVector.at(i);

        // if the command we found is not the "wait" command
        if (command[0] != "wait") {
            
            int fd[2];
            pipe(fd);
            
            int commandProcess = fork();

            if (commandProcess == 0) { // if we are in the child process
                
                close(fd[0]); // Close the unused read end in the child process

                // If the command does not contain any redirection or background operator part
                if (((command[3] == "-" )&& (backgroundJob == 'n' ))) {
                    // Handling escaping special characters
                    for (auto &arg : command) {
                        if (!arg.empty() && arg[0] == '-') {
                            arg = "\"" + arg + "\"";
                        }
                    }

                    // Converting command line arguments to a char array for execvp
                    vector<char *> args;
                    for (size_t i = 0; i < command.size(); ++i) {
                        args.push_back(strdup(command[i].c_str()));
                    }
                    args.push_back(nullptr);

                    // Redirect the standard output to the write end of the pipe
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[0]);
                    close(fd[1]);

                    // Execute the command
                    execvp(args[0], args.data());
                    
                    // If the command contains a redirection part
                } else if (command[3] != "-") {
                    
                    
                    // get the file location
                    const char *fileLocation = command[4].c_str();

                    // Close the standard output
                    if (command[3] == ">") {
                        close(STDOUT_FILENO);
                    } else if (command[3] == "<") {
                        close(STDIN_FILENO);
                    }

                    // Open the file using the open system call
                    int fileDescriptor = open(fileLocation, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                    // failed to open the file descriptor
                    if (fileDescriptor == -1) {
                        cout << "Failed to open the file descriptor." << endl;
                    }

                    // Redirect the standard output to the file
                    if (command[3] == ">") {
                        dup2(fileDescriptor, STDOUT_FILENO);
                    } else if (command[3] == "<") {
                        dup2(fileDescriptor, STDIN_FILENO);
                    }

                    // Close the file descriptor
                    close(fileDescriptor);
                    
                    if (backgroundJob == 'n') { // if there is no background job
                        waitpid(commandProcess, nullptr, 0);
                    } else { // if there exist background jobs
                        processIDVector.push_back(commandProcess); // should keep track of these
                        exit(0); // Make sure the child process exits after its job
                    }
                    
                
                }
                else { // output should be on the console
                    close(fd[1]); // Close the write end of the pipe in the child process

                    ThreadArgs args = {fd[0], myMutex};
                    pthread_t myThread;
                    pthread_create(&myThread, nullptr, ListenerThread, &args);
                    threadIDVector.push_back(myThread);


                }

            } else {
                
                if (backgroundJob != 'y') { // if there is no background job
                    waitpid(commandProcess, nullptr, 0);
                }
            }
        }
        else{ // wait exists
        
            for (int i = 0; i < processIDVector.size(); i++){
                waitpid(processIDVector[i], nullptr, 0);
            }
            processIDVector.clear(); //clearing the memory
            
            for(int i = 0; i < threadIDVector.size(); i++) {
                pthread_join(threadIDVector.at(i), nullptr);
            }

            threadIDVector.clear();
        }
    }
    return 0;
}


