// Team: Gregory Freitas, Anthony Jackson, Kensal Ramos

#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <bits/stdc++.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstdarg>
#include <map>
#include <dirent.h>

using std::string;
using std::cout;
using std::vector;
using std::map;
using std::stack;

string current_directory = "/";
vector<string> temp_history;
vector<int> pid_history;

bool quit_flag = false;

enum commands 
{
    WHEREAMI,
    MOVETODIR,
    BYEBYE,
    START,
    BACKGROUND,
    HISTORY,
    REPLAY,
    INVALID,
    DALEK,
    REPEAT,
    DALEK_ALL
};

map<string, commands> commandMap;

void command_dispatcher(const vector<string> tokens);

//worked on by Greg Frietas & Anthony Jackson
int tokenize(string& line, vector<string>& tokens)
{
    int counter = 0;
    string temp;
    std::stringstream stream_in(line);

    while (getline(stream_in, temp, '"'))
    {
        counter++;

        if(counter % 2 == 0)
        {
            if(!temp.empty())
                tokens.push_back(temp);
        }
        else
        {
            std::stringstream stream_segment(temp);
            while (getline(stream_segment, temp, ' '))
            {
                if(!temp.empty())
                    tokens.push_back(temp);
            }
            
        }
    }
    
    return 0;
}

// Anthony Jackson worked on this function.
// This handles relative path .. and . to allow for forward and backward traversing.
string fix_path(string& fixed_path)
{
    vector<string> temp_path_names;
    std::stringstream stream_in(fixed_path);
    string temp;
    string new_path = "/";
    
    // Parse based on /'s
    while (getline(stream_in, temp, '/'))
    {
        if(!temp.empty())
        {
            if (temp == "..")
            {
                temp_path_names.pop_back();
            }
            else if(temp == ".")
                continue;
            else
                temp_path_names.push_back(temp);
        }
    }
    
    for(auto& path_name : temp_path_names)
    {
        new_path += path_name + '/';
    }

    new_path = new_path.substr(0, new_path.size() - 1);

    return new_path;
}

// Gregory Freitas worked on this function.
int move_to_dir(string path)
{
    if(path.length() <= 0)
        return -1;

    string relative_path =(current_directory + '/' + path);

    // Determine if its absolute path or relative.
    string target_path = path[0] == '/' ? path : relative_path;

    target_path = fix_path(target_path);
    // /home/thing --> cd .. --> /home
    // home, . do nothing, .. pop the stack, valid push new name.
    // home/test/..

    struct stat buffer;
    int status = stat(&target_path[0], &buffer);

    if(-1 == status)
    {
        if (ENOENT == errno)
        {
            cout << "The specified directory " << '\"' << target_path << '\"' << " does not exist." << '\n';
            return -1;
        }
        else
        {
            perror("stat");
            exit(1);
        }
    }
    else
    {
        if(S_ISDIR(buffer.st_mode))
        {
            current_directory = target_path;
        }
        else
        {
            cout << "The specified directory " << '\"' << target_path << '\"' << " is a file, not a directory." << '\n';
            return -1;
        }
    }

    return 0;
}

// Gregory Freitas worked on this function.
int where_am_i()
{
    if (current_directory == "")
        return 1;
    

    cout << current_directory << '\n';

    return 0;
}

// Kensal Ramos worked on this function.
int history(const vector<string>& tokens)
{
    // We have more than one argument
    if(tokens.size() > 2)
    {
        std::cerr << "Invalid number of arguments." << '\n';
        return -1;
    }

    // Argument was passed, check if its -c
    if(tokens.size() == 2)
    {
        string arg = tokens[1];
        
        // May need to clear the log file too, not sure.
        if(arg == "-c")
        {
            temp_history.clear();
            cout << "History has been cleared." << '\n';
            return 0;
        }
        else
        {
            std::cerr << "Invalid argument passed" << '\n';
            return -1;
        }
    }

    // Cannot print history if no history exists.
    if(temp_history.size() <= 0)
    {
        cout << "There is no history to display." << '\n';
        return -1;
    }
    
    for(int i = 0; i < temp_history.size(); i++)
    {
        cout << i << ": " << temp_history[i] << '\n';
    }

    return 0;
}

// Anthony Jackson worked on this function.
int bye_bye()
{
    quit_flag = true;

    return 0;
}

// Anthony Jackson worked on this function.
int replay_number(const string& number)
{
    int target_number;
    
    try
    {
        // Conver the passed number to a integer.
        target_number = std::stoi(number);

        // Get the index of the target number within the history.
        string command = temp_history.at(target_number);

        vector<string> tokens;

        tokenize(command, tokens);

        command_dispatcher(tokens);
    }
    catch(const std::out_of_range& e)
    {  
        std::cerr << "Invalid number was passed as a parameter, check the history labels." << '\n';
        // std::cerr << e.what() << '\n';
        return -1;
    }
    catch(const std::invalid_argument& e)
    {
        std::cerr << "Invalid argument passed as paramter, pass a number." << '\n';
        // std::cerr << e.what() << '\n';
        return -2;
    }
    
    return 0;
}

// Gregory Freitas worked on this.
int program_validation(vector<string> program_tokens, char* args[])
{
    // [0] = executing file
    // [1...n] = Arguments
    // [n] = NULL
    int n = program_tokens.size();

    for(int i = 0; i < n; i++)
    {   
        // Allocate space for argument based on the length of the string.
        // Plus 1 space is necessary for the '\0' operator to be copied from the c_str()
        args[i] = (char *) malloc((sizeof(char) * program_tokens[i].length()) + 1);
        strcpy(args[i], program_tokens[i].c_str());
        
        /*
        // Used for testing purposes, to verify the contents of our stored strings.
        for(int j = 0; j < tokens[i + 1].length() + 1; j++)
        {
            printf("%c", args[i][j]);
            cout << '\n';
        }
        */
    }

    // Execv expects NULL or nullptr to be the final element in the list.
    args[n] = nullptr;
    
    return 0;
}

// Anthony Jackson
int file_exists(const char* path)
{
    struct stat buffer;

    return (stat(path, &buffer) == 0);
}

// Gregory Freitas worked on this function.
string resolve_path(const string& path)
{
    // Absolute path, no need to modify.
    if(path[0] == '/')
    {
        return path;
    }
    else
    {
        // Either relative, or Path variable.
        string relative = current_directory + '/' + path;

        int code = file_exists(relative.c_str());

        if(code == 1) return relative;
    }

    return path;
}

// Gregory Freitas worked on this function.
int start_program(const vector<string> program_tokens)
{
    
    char* passed_args[(program_tokens.size() + 1)];
    
    // Formats the args to be passed to exec with a nullptr to terminate the list. 
    if(program_validation(program_tokens, passed_args) != 0)
        return -1;
    
    string program = program_tokens[0];

    // Path gets resolved based on whether it is Absolute, relative, or in PATH
    string target_path = resolve_path(program);

    //cout << target_path << '\n';

    int pid, status;

    // If PID == 0, it is the child process
    if(!(pid = fork()))
    {
        // Copy the child process with the new executable based on our specified path and args.
        execvp(&target_path[0], passed_args);
        perror("execvp");
        exit(-1);
    }

    pid_history.push_back(pid);

    // Parent process waits for the child process to finish.
    waitpid(-1, &status, 0);
    
    // Must free the memory allocated for the arguments
    for (size_t i = 0; i < program_tokens.size() - 1; i++)
    {
        free(passed_args[i]);
    }

    return 0;
}

// Kensal Ramos worked on this function.
int background_program(const vector<string> program_tokens, bool silent = false)
{
    if(program_tokens.size() == 0)
    {
        std::cerr << "This command requires arguments to be passed!" << '\n';
        return 1;
    }

    char* passed_args[(program_tokens.size() + 1)];
    
    if(program_validation(program_tokens, passed_args) != 0)
        return -1;
    
    string program = program_tokens[0];
    
    string target_path = resolve_path(program);

    int exitCode = 0;

    pid_t pid;
    int status;

    pid = fork();

    // One process is your Parent which has the PID of the child.
    // Other process is the Child with a PID of 0
    
    if(0 == pid)
    {
        execvp(target_path.c_str(), passed_args);
        perror("execvp");
        exit(-1);
    }
    else 
    {
        pid_history.push_back(pid);

        if (!silent)
            cout << "PID: " << pid << '\n';
    }

    // Must free the memory allocated for the arguments
    for (size_t i = 0; i < program_tokens.size() - 1; i++)
    {
        free(passed_args[i]);
    }

    return exitCode;
}

// Kensal Ramos worked on this function.
int dalek(const string pid, bool silent = false)
{   
    try
    {
        pid_t pidVal = std::stoi(pid);
        int signal = kill(pidVal, SIGKILL);
        
        if (signal == 0)
        {

            for (int i = 0; i < pid_history.size(); i++) 
            {
                if (pid_history[i] == pidVal) 
                    pid_history.erase(pid_history.begin() + i);
            }

            if(!silent)
                cout << "Successfully killed PID: " << pid << '\n';

            return 0;
        }
        else
        {
            if(!silent)
                cout << "Failed to kill PID: " << pid << '\n';

            return 1;
        }

       
    }
    catch(const std::exception& e)
    {
        // std::cerr << e.what() << '\n';
        std::cerr << "Invalid argument type passed, try a number." << '\n';
        return 1;
    }

    return 0;
}

// Anthony Jackson worked on this function.
int repeat(const vector<string> tokens)
{
    if(tokens.size() < 3)
    {
        std::cerr << "Not enough arguments passed!" << '\n';
        return 1;
    }

    vector<string> newToken;

    for(int i = 2; i < tokens.size(); i++)
        newToken.emplace_back(tokens[i]);
    
    int repeatnum = std::stoi(tokens[1]);

    for (int i = 0; i < repeatnum; i++)
    {
        int code = background_program(newToken, true);
        
        if(code) break;
        
        if (i == 0)
            cout << "PIDs: ";
        
        cout << pid_history[pid_history.size() - 1] << ' ';
    }

    cout << '\n';

    return 0;
}

// Kensal Ramos worked on this function.
int dalek_all()
{
    if (pid_history.size() == 0) {
        cout << "0 processes terminated." << '\n';
        return 0;
    }
    cout << "Terminating " << pid_history.size() << " processes: ";
    
    int i = 0;

    while(pid_history.size() != 0) 
    {
        cout << pid_history[i] << " ";
        // If dalek fails, move on
        if (dalek(std::to_string(pid_history[i]), true) == 1) {
            i++;
        }
    }

    cout << '\n';

    return 0;
}

// We all contributed to this function.
void command_dispatcher(vector<string> tokens)
{
    if(tokens.size() < 1)
       return;

    string command_s = tokens[0];

    commands command = INVALID;

    // Try to find a mapping of the supplied command.
    try
    {
        command = commandMap.at(command_s);
    }
    catch(const std::exception& e)
    {
        //std::cerr << e.what() << '\n';
    }
    

    switch (command)
    {
    case WHEREAMI:
        where_am_i();
        break;
    case MOVETODIR:
        if(tokens.size() < 2)
        {
            std::cerr << "Invalid number of arguments!" << '\n';
            break;
        }

        move_to_dir(tokens[1]);
        break;
    case BYEBYE:
        bye_bye();
        break;
    case START:
    {   
        if(tokens.size() < 2)
        {
            std::cerr << "Invalid number of arguments!" << '\n';
            break;
        }
        
        tokens.erase(tokens.begin());

        start_program(tokens);

        break;
    }
    case BACKGROUND:
    {   
        if(tokens.size() < 2)
        {
            std::cerr << "Invalid number of arguments!" << '\n';
            break;
        }
        // First argument is the command itself. Second is the program, rest are the args.
        tokens.erase(tokens.begin());

        background_program(tokens);

        break;
    }
    case HISTORY:
    {
        history(tokens);
        break;
    }
    case REPLAY:
    {
        replay_number(tokens[1]);
        break;
    }
    case DALEK:
    {
        if(tokens.size() < 2)
        {
            std::cerr << "Invalid number of arguments!" << '\n';
            break;
        }

        string arg = tokens[1];
            
        dalek(arg);   
        break;
    }
    case REPEAT:
    {
        repeat(tokens);
        break;
    }
    case DALEK_ALL:
    {
        if(tokens.size() > 1)
        {
            std::cerr << "This command does not take any arguments." << '\n';
            break;
        }

        dalek_all();
        break;
    }
    default:
        cout << "Unkown command: " << tokens[0] << '\n';
        break;
    }
}

// Kensal Ramos worked on this function.
int load_history(vector<string>& history)
{
    string line;
    std::ifstream history_file("history.log");
    
    if(history_file.is_open())
    {
        while (getline(history_file, line))
        {
            history.push_back(line);
        }
        
        history_file.close();
    }
    else
    {
        std::cerr << "Unable to open file." << '\n';
        std::ofstream NewFile("history.log");
        cout << "Creating 'history.log' file..." << '\n';
        return 1;
    }

    return 0;
}  

// Kensal Ramos worked on this function.
int save_history(vector<string>& history)
{
    std::ofstream history_file("history.log");
    
    if(history_file.is_open())
    {
        for(string line : history)
        {
            history_file << line << '\n';
        }

        history_file.close();
    }
    else
    {
        std::cerr << "Unable to open the file.";
        return 1;
    }

    return 0;
}

int main () {

    string input = "";

    vector<string> tokens;

    commandMap["whereami"] = WHEREAMI;
    commandMap["movetodir"] = MOVETODIR;
    commandMap["byebye"] = BYEBYE;
    commandMap["start"] = START;
    commandMap["history"] = HISTORY;
    commandMap["replay"] = REPLAY;
    commandMap["background"] = BACKGROUND;
    commandMap["dalek"] = DALEK;
    commandMap["repeat"] = REPEAT;
    commandMap["dalekall"] = DALEK_ALL;

    load_history(temp_history);

    while (!quit_flag)
    {
        // Clear the tokens each cycle.
        tokens.clear();

        // To show the currentDirectory to view navigation.
        // cout << current_directory;
        // Once accepting input and what not, it will always lead with a #
        cout << "# ";
        cout.flush();
        input = "";

        getline(std::cin, input);

        tokenize(input, tokens);

        // for(auto& token : tokens)
        // {
        //     cout << token << '\n';
        // }

        command_dispatcher(tokens);

        // Save each line after inputs since we need this for the history.
        temp_history.push_back(input);
    }
    
    save_history(temp_history);

    return 0;
}