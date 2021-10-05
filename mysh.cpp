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
using std::unordered_set;

string current_directory = "";
vector<string> temp_history;
bool quit_flag = false;

enum commands 
{
    WHEREAMI,
    MOVETODIR,
    BYEBYE,
    START,
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

// Maybe I can create a path joining function.

int move_to_dir(string path)
{
    if(path.length() <= 0)
        return -1;

    string relative_path =(current_directory + '/' + path);

    // Determine if its absolute path or relative.
    string target_path = path[0] == '/' ? path : relative_path;

    // /home/thing --> cd .. --> /home
    // home, . do nothing, .. pop the stack, valid push new name.

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

int history(const string& arg = "")
{
    // Argument passed to clear the history.
    if(arg == "-c")
    {
        temp_history.clear();
        return 0;
    }

    // Cannot print history if no history exists.
    if(temp_history.size() <= 0)
        return 1;
    
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
    int target_number = std::stoi(number);
    
    
    try
    {
        string command = temp_history[target_number];

        vector<string> tokens;

        tokenize(command, tokens);

        command_dispatcher(tokens);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }
    
    return 0;
}

int program_validation(const vector<string>& tokens, string args[])
{
    // [0] = command name
    // [1] = executing file
    // [1...n] = Arguments
    // [n] = NULL

    int n = tokens.size();

    for(int i = 1; i < n - 1; i++)
    {
        args[i] = tokens[i];
    }

    return 0;
}

int start_program(const vector<string>& tokens)
{
    // Need to validate input here instead
    // Makes more sense to handle its own validation based on what it expects.
    string args[tokens.size()];

    // if(program_validation(tokens, args) != 0)
    //     return -1;
    
    args->c_str();

    for(auto& arg : args)
    {
        cout << arg << '\n';
    }

    string program = tokens[0];

    string target_path = "";
    
    if(program[0] == '/')
        target_path = program;
    else
        target_path = current_directory + '/' + program;
    
    int pid, status;

    //if(!(pid = fork()))
        //execvp(&target_path[0], args->c_str());

    perror("execvp");

    waitpid(-1, &status, 0);
    
    return 0;
}

int background_program(const vector<string>& tokens)
{



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
    case HISTORY:
    {
        history();
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

        // Save each line after inputs since we need this for the history.
        temp_history.push_back(input);

        tokenize(input, tokens);

        command_dispatcher(tokens);
    }
    
    save_history(temp_history);

    return 0;
}