#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <bits/stdc++.h>
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

string current_directory = "/home/gregaf300/OS-Class/Assignment_2";
vector<string> temp_history;
stack<string> curent_dir_s;
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
    INVALID
};

map<string, commands> commandMap;

void command_dispatcher(vector<string>& tokens);

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

int where_am_i()
{
    if (current_directory == "")
        return 1;


    cout << current_directory << '\n';

    return 0;
}

int history(const vector<string>& tokens)
{
    // We have more than one argument
    if(tokens.size() > 2)
    {
        cout << "Invalid number of arguments." << '\n';
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
            cout << "Invalid argument passed" << '\n';
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

int bye_bye()
{
    quit_flag = true;

    return 0;
}

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
        cout << "Invalid number was passed as a parameter, check the history labels." << '\n';
        std::cerr << e.what() << '\n';
        return -1;
    }
    catch(const std::invalid_argument& e)
    {
        cout << "Invalid argument passed as paramter, pass a number." << '\n';
        std::cerr << e.what() << '\n';
        return -2;
    }
    
    return 0;
}

int program_validation(vector<string> tokens, char* args[])
{
    // [0] = command name
    // [1] = executing file
    // [1...n] = Arguments
    // [n] = NULL
    int n = tokens.size() - 1;

    for(int i = 0; i < n; i++)
    {   
        // Allocate space for argument based on the length of the string.
        // Plus 1 space is necessary for the '\0' operator to be copied from the c_str()
        args[i] = (char *) malloc((sizeof(char) * tokens[i + 1].length()) + 1);
        strcpy(args[i], tokens[i + 1].c_str());
        
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

int start_program(const vector<string>& tokens)
{
 
    char* args[tokens.size() - 1];
    
    if(program_validation(tokens, args) != 0)
        return -1;
    
    string program = tokens[1];

    string target_path = "";

    if(program[0] == '/')
        target_path = program;
    else
        target_path = current_directory + '/' + program;
    
    int pid, status;

    if(!(pid = fork()))
        execvp(&target_path[0], args);

    perror("execvp");

    waitpid(-1, &status, 0);

    // Must free the memory allocated for the arguments
    for (size_t i = 0; i < tokens.size() - 2; i++)
    {
        free(args[i]);
    }

    return 0;
}

int background_program(const vector<string>& tokens)
{
    char* args[tokens.size() - 1];
    
    if(program_validation(tokens, args) != 0)
        return -1;
    
    string program = tokens[1];

    string target_path = "";

    if(program[0] == '/')
        target_path = program;
    else
        target_path = current_directory + '/' + program;
    
    int pid, status;

    pid = fork();

    if(pid == 0)
        execvp(&target_path[0], args);

    cout << "PID: " << pid << '\n'; 
    //perror("execvp");

    // Must free the memory allocated for the arguments
    for (size_t i = 0; i < tokens.size() - 2; i++)
    {
        free(args[i]);
    }

    return 0;
}

int dalek()
{
    return 0;
}

int repeat()
{
    return 0;
}

int dalek_all()
{
    return 0;
}

void command_dispatcher(vector<string>& tokens)
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
        std::cerr << e.what() << '\n';
    }
    
    
    switch (command)
    {
    case WHEREAMI:
        where_am_i();
        break;
    case MOVETODIR:
        move_to_dir(tokens[1]);
        break;
    case BYEBYE:
        bye_bye();
        break;
    case START:
    {   
        start_program(tokens);

        break;
    }
    case BACKGROUND:
    {   
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
    default:
        cout << "Unkown command: " << tokens[0] << '\n';
        break;
    }
}

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
        return 1;
    }


    return 0;
}  

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

    commandMap["pwd"] = WHEREAMI;
    commandMap["move"] = MOVETODIR;
    commandMap["bye"] = BYEBYE;
    commandMap["start"] = START;
    commandMap["history"] = HISTORY;
    commandMap["replay"] = REPLAY;
    commandMap["bg"] = BACKGROUND;

    load_history(temp_history);


    while (!quit_flag)
    {
        // Clear the tokens each cycle.
        tokens.clear();

        // To show the currentDirectory to view navigation.
        cout << current_directory;
        // Once accepting input and what not, it will always lead with a #
        cout << "# ";

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