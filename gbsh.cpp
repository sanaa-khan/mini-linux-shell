//OS Project Spring 2020
// Sana Ali Khan
// 18i-0439

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <dirent.h>
#include <cstring>
#include <sys/wait.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
using namespace std;

void listDirectory(vector<string>);
void changeDirectory(vector<string>);
void listEnvVars(vector<string>);
void setEnvVar(vector<string>);
void unsetEnvVar(vector<string>);
void pipeCommands(vector<string>);
void executeProgram(vector<string>);
void printWorkingDirectory(vector<string>);

template <typename T>
void displayVector(vector<T>);

// variables for controlling redirection of input/output
int output, input; 
bool oRedirect = false, iRedirect = false;

void handle_signal(int signum) {
  return;
}

int main(int argc, char *argv[]) {

	signal(SIGINT, handle_signal); // program will ignore ctrl+c

	int len = 50;
	char host[len], user[len], cwd[len];
	string command;

	gethostname(host, len);
	getlogin_r(user, len);

	while (true) {
		getcwd(cwd, len);
		cout << "\033[1;36m" << user << "@" << host << "\033[0m";
		cout << "\033[1;35m " << cwd << "\033[0m > ";
		
		getline(cin, command);

		if (command == "exit") {
			exit(0);
		}

		else if (command == "clear")
			cout << "\033c";

		// building vector of commands
		else {
			vector<string> tokens;
			string temp;

			for (int i = 0; i < command.size(); i++) {

				while (command[i] != ' ' and command[i] != '\n' and command[i] != '\t' and command[i] != '\0') {
					temp += command[i];
					i++;
				}

				tokens.push_back(temp);
				temp = "";
			}

			displayVector(tokens);

			string cmd = tokens.front();

			if (cmd == "ls")
				listDirectory(tokens);

			else if (cmd == "pwd")
				printWorkingDirectory(tokens);

			else if (cmd == "cd") 
				changeDirectory(tokens);

			else if (cmd == "environ")
				listEnvVars(tokens);

			else if (cmd == "setenv")
				setEnvVar(tokens);

			else if (cmd == "unsetenv")
				unsetEnvVar(tokens);

			else {
				bool background = false;

				// for background execution
				if (tokens[tokens.size() - 1].size() == 1 and tokens[tokens.size() - 1][0] == '&') {
					background = true;
					tokens.pop_back();
				}

				bool skip = false;

				/// detecting pipe
				for (int i = 0; i < tokens.size(); i++) {
					if (tokens[i].size() == 1 and tokens[i][0] == '|') {
						pipeCommands(tokens);
						skip = true;
					}
				}

				if (!skip) {
					pid_t pid = fork();

					if (pid > 0) { // parent process
						if (!background)
							waitpid(pid, 0, 0); // wait until child of pid is terminated

						if (oRedirect) { // redirect output back to stdout 
							fflush(stdout); 

							dup2(output, 1);
							close(output);

							oRedirect = false;
						}

						if (iRedirect) { // redirect input back to stdin
							fflush(stdin); 

							dup2(input, 0);
							close(input);

							iRedirect = false;
						}
					}

					else if (pid == 0) { // child process
						setenv("SHELL", cwd, 1);
						executeProgram(tokens);
					}

					else
						cout << "Fork unsuccessful";
				}
			}

			tokens.clear();

			pid_t anyChild = 1;

			while (anyChild > 0) { // reaping zombie processes (that may have been running in background previously)
				anyChild = waitpid(-1, 0, WNOHANG);
			}
		} 
	}

	return 0;
}

template <typename T>
void displayVector(vector<T> v) {
	for (int i = 0; i < v.size(); i++)
		cout << v[i] << "\n";
}

void listDirectory(vector<string> tokens) {

	string temp, filename;

	// detecting if output has to be redirected to a file
	if (tokens.size() > 1) {
		if (tokens[1].size() == 1 and tokens[1][0] == '>') {
			if (tokens.size() == 2) {
				cout << "\033[1;31m" << "Error: ";
				cout << "\033[0;38m" << "Output file not provided.\n";
				return;
			}

			else if (tokens.size() == 3) {
				temp = ".";
				filename = tokens[2];
				oRedirect = true;
			}
			
			else {
				cout << "\033[1;31m" << "Error: ";
				cout << "\033[0;38m" << "Too many arguments.\n";
				return;
			}
		}

		else if (tokens.size() == 2)
			temp = tokens[1];

		else if (tokens.size() > 2) {
			if (tokens[2].size() == 1 and tokens[2][0] == '>') {
				if (tokens.size() == 3) {
					cout << "\033[1;31m" << "Error: ";
					cout << "\033[0;38m" << "Output file not provided.\n";
					return;
				}

				else if (tokens.size() == 4) {
					temp = tokens[1];
					filename = tokens[3];
					oRedirect = true;
				}

				else {
					cout << "\033[1;31m" << "Error: ";
					cout << "\033[0;38m" << "Too many arguments.\n";
					return;
				}
			}

			else {
				cout << "\033[1;31m" << "Error: ";
				cout << "\033[0;38m" << "Too many arguments.\n";
				return;
			}
		}
	}

	else
		temp = "."; // in the case of just "ls" command i.e no directory mentioned

	// output redirection
	if (oRedirect) {
		int file_desc = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
		output = dup(1);

		close(1);
		dup(file_desc);
		close(file_desc);
	}
	
	if (tokens.size() > 4) {
		cout << "\033[1;31m" << "Error: ";
		cout << "\033[0;38m" << "Too many arguments.\n";
		return;
	}

	const char* path;
	path = temp.c_str();

	DIR* directory = NULL;
	dirent* current = NULL;
	unsigned int type;
	unsigned short length;
	char *name;

	directory = opendir(path);

	if (directory != NULL) {
		current = readdir(directory);
		
		while (current != NULL) {
			type = current->d_type;
			length = current->d_reclen;

			name = new char[length];

			strcpy(name, current->d_name);

			if ( (name[0] == '.' and name[1] == '\0') or (name[0] == '.' and name[1] == '.' and name[2] == '\0') ) {
				// do nothing in the case of '.' or ".."
			}

			else
			{
				switch (type) {
					case 4:
						if (oRedirect)
							cout << name << "\t";
						else
							cout << "\033[1;34m" << name << "\033[0m" << "\t";
						break;
					case 8:
						if (oRedirect)
							cout << name << "\t";
						else
							cout << "\033[24;37m" << name << "\033[0m" << "\t";
						break;
					default:
						cout << name << "\t";
				}
			}
			

			delete []name;
			current = readdir(directory);
		}

		closedir(directory);
		cout << "\n";

		if (oRedirect) {
			fflush(stdout); 

			dup2(output, 1);
			close(output);

			oRedirect = false;
		}
	}
}

void changeDirectory(vector<string> tokens) {

	string temp;

	if (tokens.size() > 1)
		temp = tokens[1];
	else
		temp = "/home";

	if (tokens.size() > 3) {
		cout << "\033[1;31m" << "Error: ";
		cout << "\033[0;38m" << "Too many arguments.\n";
		return;
	}

	const char* path = temp.c_str();

	chdir(path);
}

void listEnvVars(vector<string> tokens) {

	string filename;

	char cwd[100];
	getcwd(cwd, 100);

	setenv("SHELL", cwd, 1);

	// for output redirection
	if (tokens.size() > 1) {
		if (tokens[1].size() == 1 and tokens[1][0] == '>') {
			if (tokens.size() == 2) {
				cout << "\033[1;31m" << "Error: ";
				cout << "\033[0;38m" << "Output file name not provided.\n";
				return;
			}

			else if (tokens.size() == 3) {
				filename = tokens[2];
				oRedirect = true;
			}

			else {
				cout << "\033[1;31m" << "Error: ";
				cout << "\033[0;38m" << "Too many arguments.\n";
				return;
			}
		}

		else {
			cout << "\033[1;31m" << "Error: ";
			cout << "\033[0;38m" << "Too many arguments.\n";
			return;
		}
	}

	if (oRedirect) {
		int file_desc = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
		output = dup(1);

		close(1);
		dup(file_desc);
		close(file_desc);
	}

	// outputting variables
	int index = 0;
	char* current = environ[index];

	while (current != NULL) {
		cout << current << "\n";
		current = environ[++index];
	}

	// redirecting output back
	if (oRedirect) {
		fflush(stdout); 

		dup2(output, 1);
		close(output);

		oRedirect = false;
	}
}

void setEnvVar(vector<string> tokens) {

	char cwd[100];
	getcwd(cwd, 100);

	setenv("SHELL", cwd, 1);

	string name, value;

	if (tokens.size() == 1) {
		cout << "\033[1;31m" << "Error: ";
		cout << "\033[0;38m" << "Name of environment variable not given.\n";
		return;
	}

	name = tokens[1];

	if (tokens.size() == 2)
		value = "";

	else if (tokens.size() == 3)
		value = tokens[2];

	else {
		cout << "\033[1;31m" << "Error: ";
		cout << "\033[0;38m" << "Too many arguments provided.\n";
		return;
	}

	char* var = getenv(name.c_str());

	if (var != NULL)
		cout << "'" << name << "' is already defined.\n";

	else
		setenv(name.c_str(), value.c_str(), 0);
}

void unsetEnvVar(vector<string> tokens) {

	char cwd[100];
	getcwd(cwd, 100);

	setenv("SHELL", cwd, 1);

	string name;

	if (tokens.size() == 1) {
		cout << "\033[1;31m" << "Error: ";
		cout << "\033[0;38m" << "Name of environment variable not given.\n";
		return;
	}

	if (tokens.size() == 2)
		name = tokens[1];

	else {
		cout << "\033[1;31m" << "Error: ";
		cout << "\033[0;38m" << "Too many arguments provided.\n";
		return;
	}

	char* var = getenv(name.c_str());

	if (var == NULL)
		cout << "'" << name << "' is undefined.\n";

	else
		unsetenv(name.c_str());
}

void pipeCommands(vector<string> tokens) {

	string cmd1, cmd2;

	for (int i = 0; i < tokens.size(); i++) {
		if (tokens[i].size() == 1 and tokens[i][0] == '|') {
			if (i == 0 or i == tokens.size() - 1) {
				cout << "\033[1;31m" << "Error: ";
				cout << "\033[0;38m" << "Command missing.\n";
				return;
			}

			else {
				cmd1 = tokens[0];
				cmd2 = tokens[i + 1];
				break;
			}
		}
	}

	return; // error here fix later

	int fd[2];
	pipe(fd);

	if (!fork()) {
		close(1);
		dup(fd[1]);
		close(fd[0]);
		
		if (!fork()) {
			execl(cmd1.c_str(), cmd1.c_str(), NULL);
		}

		else {
			wait(NULL);
		}
	}

	else {
		close(0);
		dup(fd[0]);
		close(fd[1]);
		execl(cmd2.c_str(), cmd2.c_str(), NULL);
	} 
}

void executeProgram(vector<string> tokens) {

	bool outputRedirection = false, inputRedirection = false;

	for (int i = 0; i < tokens.size(); i++) { // detecting if output/input redirection required

		if (tokens[i].size() == 1) {

			if (tokens[i][0] == '>')
				outputRedirection = true;

			else if (tokens[i][0] == '<')
				inputRedirection = true;
		}
	}

	string cmd = tokens[0];
	int noOfArguments = 0;
	string outputFile, inputFile;
	char** arg;

	if (!inputRedirection and outputRedirection) { 
		for (int i = 1; i < tokens.size(); i++) {
			if (tokens[i].size() == 1 and tokens[i][0] == '>') {
				if (i == tokens.size() - 1) {
					cout << "\033[1;31m" << "Error: ";
					cout << "\033[0;38m" << "Output file not provided.\n";
					return;
				}

				else {
					outputFile = tokens[i + 1];
					break;
				}
			}

			else {
				noOfArguments++;
			}
		}

		arg = new char*[noOfArguments + 2];
		arg[0] = new char[20];

		strcpy(arg[0], cmd.c_str());
		arg[noOfArguments + 1] = nullptr;

		for (int i = 1; i <= noOfArguments; i++) {
			arg[i] = new char[20];
			strcpy(arg[i], tokens[i].c_str());
		}

		int file_desc = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
		output = dup(1);

		close(1);
		dup(file_desc);
		close(file_desc);

		oRedirect = true;
	}

	else if (inputRedirection and outputRedirection) {
		int i;
		for (i = 1; i < tokens.size(); i++) {
			if (tokens[i].size() == 1 and tokens[i][0] == '<') {
				if (i == tokens.size() - 1) {
					cout << "\033[1;31m" << "Error: ";
					cout << "\033[0;38m" << "Input file not provided.\n";
					return;
				}

				else {
					i++;
					inputFile = tokens[i];
					i++;
					break;
				}
			}

			else {
				noOfArguments++;
			}
		}

		for (i = 1; i < tokens.size(); i++) {
			if (tokens[i].size() == 1 and tokens[i][0] == '>') {
				if (i == tokens.size() - 1) {
					cout << "\033[1;31m" << "Error: ";
					cout << "\033[0;38m" << "Output file not provided.\n";
					return;
				}

				else {
					outputFile = tokens[i + 1];
					break;
				}
			}
		}

		arg = new char*[noOfArguments + 2];
		arg[0] = new char[20];

		strcpy(arg[0], cmd.c_str());
		arg[noOfArguments + 1] = nullptr;

		for (int j = 1; j <= noOfArguments; j++) {
			arg[j] = new char[20];
			strcpy(arg[j], tokens[j].c_str());
		}

		int file_desc = open(outputFile.c_str(), O_WRONLY | O_CREAT |  O_TRUNC, 0777);
		output = dup(1);

		close(1);
		dup(file_desc);
		close(file_desc);
		
		oRedirect = true;

		int file_desc2 = open(inputFile.c_str(), O_WRONLY | 0777);
		input = dup(0);

		close(0);
		dup(file_desc2);
		close(file_desc2);

		iRedirect = true;
	}

	else if (!inputRedirection and !outputRedirection) {
		noOfArguments = tokens.size() - 1;
		char* arg[noOfArguments + 2];

		for (int i = 0; i <= noOfArguments; i++)
			arg[i] = new char[20];
			
		strcpy(arg[0], cmd.c_str());

		int i;
		for (i = 1; i <= noOfArguments; i++) {
			strcpy(arg[i], tokens[i].c_str());
		}

		arg[i] = NULL;
	}

	if (cmd == "top") {
		char path[50];
		getcwd(path, 50);

		chdir("/usr/bin");
		execl("top", "top", NULL);
		chdir(path);
	}

	else if (cmd == "man") {
		char path[50];
		getcwd(path, 50);

		chdir("/usr/bin");

		if (tokens.size() > 1)
			execl("man", "man", tokens[1].c_str(), NULL);
		else
			execl("man", "man", NULL);

		chdir(path);
	}

	else if (cmd == "ps") {
		char path[50];
		getcwd(path, 50);

		chdir("/usr/bin");

		if (tokens.size() > 1)
			execl("ps", "ps", tokens[1].c_str(), NULL);
		else
			execl("ps", "ps", NULL);

		chdir(path);
	}

	else {
		if (execvp(arg[0], arg) < 0) {
			cout << "\033[1;31m" << "Error: ";
			cout << "\033[0;38m" << "'" << arg[0] << "' not found.\n";
			
			for (int i = 0; i < noOfArguments + 2; i++)
				delete []arg[i];

			exit(0);
		}
	}
}

void printWorkingDirectory(vector<string> tokens) {

	int len = 50;
	char cwd[len];

	getcwd(cwd, len);

	if (tokens.size() == 1)
		cout << cwd << "\n";

	// for redirecting output
	else if (tokens[1].size() == 1 and tokens[1][0] == '>') {
		if (tokens.size() == 3) {
			string filename = tokens[2];

			int file_desc = open(filename.c_str(), O_WRONLY | O_CREAT |  O_TRUNC, 0777);
			output = dup(1);

			close(1);
			dup(file_desc);
			close(file_desc);
			
			cout << cwd;
			
			fflush(stdout); 

			dup2(output, 1);
			close(output);
		}

		else if (tokens.size() < 3) {
			cout << "\033[1;31m" << "Error: ";
			cout << "\033[0;38m" << "Output file not provided.\n";
		}

		else {
			cout << "\033[1;31m" << "Error: ";
			cout << "\033[0;38m" << "Too many arguments.\n";
		}
	}
}