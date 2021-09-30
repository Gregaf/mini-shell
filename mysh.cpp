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
using std::cin;
using std::vector;
using std::map;
using std::stringstream;
using std::ofstream;
using std::ifstream;


string current_directory = "/";
vector<string> temp_history;
bool quit_flag = false;

enum commands 
{
    WHEREAMI,
    MOVETODIR,
    BYEBYE,
    START,
    INVALID
};

map<string, commands> commandMap;


int tokenize(string& line, vector<string>& tokens)
{
    stringstream check(line);

    string temp_store;

    while (getline(check, temp_store, ' '))
    {
        tokens.push_back(temp_store);
    }

    return 0;
}

int list()
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (&current_directory[0])) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
        printf ("%s\n", ent->d_name);
    }
    closedir (dir);
    } else {
    /* could not open directory */
    perror ("");
    return EXIT_FAILURE;
    }

    return 0;
}

// Maybe I can create a path joining function.

int move_to_dir(string path)
{

    struct stat buffer;

    if(path.length() <= 0)
        return -1;

    string relative_path =(current_directory + '/' + path);

    string target_path = path[0] == '/' ? path : relative_path;

    int status = stat(&target_path[0], &buffer);

    if(-1 == status)
    {
        if (ENOENT == errno)
        {
            cout << "The specified directory " << '\"' << path << '\"' << " does not exist." << '\n';
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
            cout << "The specified directory " << '\"' << path << '\"' << " is a file, not a directory." << '\n';
            return -1;
        }
    }

    return 0;
}

int where_am_i()
{
    if (current_directory == "")
        return 1;


    cout << current_directory << std::endl;

    return 0;
}

int history(string arg = "")
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

int replay_number(string number)
{
    


    return 0;
}

int start_program(string program, char *const args[])
{
    if(program.length() <= 0)
        return -1;

    string target_path = "";
    
    if(program[0] == '/')
        target_path = program;
    else
        target_path = current_directory + '/' + program;
    
    int pid, status;

    if(!(pid = fork()))
        execv(&target_path[0], args);

    waitpid(-1, &status, 0);
    
    return 0;
}

int background_program()
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
        string stuff = tokens[1];
        char* args[tokens.size() - 1];

        for(int i = 2; i < tokens.size(); i++)
        {
            args[i - 2] = &tokens[i][0];
        }

        args[tokens.size() - 1] = NULL;

        start_program(stuff, args);

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
    ifstream history_file("history.log");
    
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
    ofstream history_file("history.log");
    
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

        getline(cin, input);

        // Save each line after inputs since we need this for the history.
        temp_history.push_back(input);

        tokenize(input, tokens);
                
        command_dispatcher(tokens);
    }
    
    save_history(temp_history);

    return 0;
}