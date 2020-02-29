#include <vector>
#include "Limiter.h"
using std::vector;
using token_bucket::Limiter;

class ExecArgs {
public:
    const char *m_path;
    char *const *m_argv;
    char *const *m_env;
};

class ProcOptions {
public:
    // kill child when parent die
    bool m_kill_when_parent_die;
    int m_execute_cnt;
    int m_freq_denominator;
    int m_freq_numerator;
    vector<ExecArgs> m_exec_args;
    Limiter *m_limiter;
    ProcOptions() : m_kill_when_parent_die(false), m_execute_cnt(0),
                    m_freq_denominator(-1), m_freq_numerator(-1),
                    m_limiter(nullptr){}
};
