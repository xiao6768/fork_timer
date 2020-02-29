#include <iostream>
#include <signal.h>
#include <string>
#include <sys/prctl.h>
#include <unistd.h>
#include <vector>
#include <wait.h>

#include "../include/Limiter.h" // https://github.com/yo123abxd/cpp_rate_limiter
#include "../include/execl_args.h"

using std::vector;
using std::string;
using token_bucket::Limiter;

void sigchld_handler(int signo) {
    if (signo == SIGCHLD) {
        wait(nullptr);
    }
}

void start_fork_and_exec(ProcOptions& proc_options) {
    //execve
    int exec_args_idx = 0;
    int exec_agrs_len = static_cast<int>(proc_options.m_exec_args.size());
    if (exec_agrs_len == 0) {
        std::cerr << "has no program to execute" << std::endl;
        exit(-1);
    }
    for (int i = 0; i < proc_options.m_execute_cnt; ++i) {
        bool limiter_ok = proc_options.m_limiter->wait(
                1.0, std::chrono::hours(INT32_MAX) /* unlimited */);

        if (!limiter_ok) {
            /* will never arriving at here*/
            std::cerr << "Limiter error, exit program" << std::endl;
            exit(-1);
        }

        pid_t pid = fork();
        if (pid == 0) { // child
            if (proc_options.m_kill_when_parent_die) {
                prctl(PR_SET_PDEATHSIG, SIGKILL);
            }
            int exec_ret = 0;
            if (proc_options.m_exec_args[exec_args_idx].m_env == nullptr) {
                exec_ret = execv(proc_options.m_exec_args[exec_args_idx].m_path, 
                        proc_options.m_exec_args[exec_args_idx].m_argv);
            } else {
                exec_ret = execve(proc_options.m_exec_args[exec_args_idx].m_path, 
                        proc_options.m_exec_args[exec_args_idx].m_argv,
                        proc_options.m_exec_args[exec_args_idx].m_env);
            }
            if (exec_ret != 0) {
                std::cerr << "exec program failed errno = "
                        << errno << std::endl;
                exit(-1);
            }
        } else if (pid < 0) { // error
            std::cerr << "fork error, errno = " << errno << std::endl;
            exit(-1);
        }
        // parent
        exec_args_idx = (exec_args_idx + 1) % exec_agrs_len;
    }
    
}

int main(int argc, const char* argv[]) {

    /*
                static_cast<double>(proc_options.m_freq_numerator) / 
                proc_options.m_freq_denominator, 
                */
    string help_str = "";
    if (argc < 2) {
        std::cout << help_str << std::endl;
    }

    ProcOptions proc_options;
    for (int i = 1; i < argc; ++i) {
        size_t str_pos = -1;
        string arg = argv[i];
        if (arg.find("--help") != string::npos) {
            std::cout << help_str << std::endl;
            return 0;
        } else if ((str_pos = arg.find("--exec_cnt=")) != string::npos) {
            proc_options.m_execute_cnt = 
                atoi(arg.substr(str_pos + string("--exec_cnt=").size()).c_str());

        } else if (arg.find("--freq_dnmtr=") != string::npos) {
            proc_options.m_freq_denominator = 
                atoi(arg.substr(str_pos + string("--freq_dnmtr=").size()).c_str());

        } else if (arg.find("--freq_nmrtr=") != string::npos) {
            proc_options.m_freq_denominator = 
                atoi(arg.substr(str_pos + string("--freq_nmrtr=").size()).c_str());

        } else if (arg.find("--kill_when_parent_die") != string::npos) {
            proc_options.m_kill_when_parent_die = true;
        } else if (arg.find("--exec_args_file=") != string::npos) {
            string exec_args_file = 
                arg.substr(str_pos + string("--exec_args_file=").size());

        } else if (arg.find("--exec_args_str=") != string::npos) {
            string exec_args_str = 
                arg.substr(str_pos + string("--exec_args_str=").size());
        } else {
            std::cerr << "args err" << std::endl;
            std::cout << help_str << std::endl;
            return -1;
        }
    }

    if (proc_options.m_freq_denominator <= 0 
            || proc_options.m_freq_numerator <= 0) {
        std::cerr << "freq_dnmtr or freq_nmrtr not set or setted as invalid value"
                << std::endl;
        std::cout << help_str << std::endl;
        return -1;
    }
    if (proc_options.m_execute_cnt <= 0) {
        std::cerr << "exec_cnt not set or setted as invalid value"
                << std::endl;
        std::cout << help_str << std::endl;
    }
    signal(SIGCHLD, sigchld_handler);

    return 0;
}
