#include "core/uci_player.hpp"

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#endif

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <iostream>

#ifdef _WIN32
void registerUciPlayer() {
    // UCI player not supported on Windows currently
}
#else

static std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

void registerUciPlayer() {
    auto factory = [](const std::vector<ConfigField>& cfg){
        return std::make_unique<UciEngine>(cfg);
    };

    std::vector<ConfigField> fields = {
        {"cmd", "Command", FieldType::String, std::string("")},
        {"time_limit", "Thinking time (s)", FieldType::Double, 5.0},
        {"enable_info", "Enable info output", FieldType::Bool, false},
    };

    AIRegistry::registerAI("UCI engine", fields, factory);
}

UciEngine::UciEngine(const std::vector<ConfigField>& cfg) 
    : m_cmd(get_config_field_value<std::string>(cfg, "cmd")),
      m_enable_info(get_config_field_value<bool>(cfg, "enable_info")),
      m_default_movetime_ms(static_cast<int>(get_config_field_value<double>(cfg, "time_limit") * 1000))
{}

UciEngine::~UciEngine() {
    _stop_process();
}

void UciEngine::_start_process() {
    int inpipe[2];
    int outpipe[2];
    if (pipe(inpipe) != 0 || pipe(outpipe) != 0) {
        throw std::runtime_error("UciEngine: pipe() failed");
    }

    // build argv
    std::vector<std::string> parts;
    {
        std::istringstream iss(m_cmd);
        std::string tok;
        while (iss >> tok) parts.push_back(tok);
    }
    if (parts.empty()) {
        throw std::runtime_error("UciEngine: command is empty");
    }

    std::vector<char*> argv;
    for (auto &p : parts) argv.push_back(const_cast<char*>(p.c_str()));
    argv.push_back(nullptr);

    // check executable
    auto is_executable = [](const std::string &path)->bool {
        return access(path.c_str(), X_OK) == 0;
    };
    std::string exe = parts[0];
    if (exe.find('/') == std::string::npos) {
        const char *path_env = getenv("PATH");
        if (path_env) {
            std::string path(path_env);
            size_t start = 0;
            bool found = false;
            while (start < path.size()) {
                size_t pos = path.find(':', start);
                std::string dir = path.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
                std::string cand = dir + "/" + exe;
                if (is_executable(cand)) { exe = cand; found = true; break; }
                if (pos == std::string::npos) break;
                start = pos + 1;
            }
            if (!found) {
                throw std::runtime_error("UciEngine: '" + parts[0] + "' not found in PATH");
            }
        } else {
            throw std::runtime_error("UciEngine: PATH not set");
        }
    } else {
        if (!is_executable(exe)) {
            throw std::runtime_error("UciEngine: '" + exe + "' not executable");
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("UciEngine: fork() failed");
    }
    if (pid == 0) { // child process
        // stdin <- inpipe[0], stdout -> outpipe[1]
        dup2(inpipe[0], STDIN_FILENO);
        dup2(outpipe[1], STDOUT_FILENO);
        dup2(outpipe[1], STDERR_FILENO); // redirect stderr to stdout for easier debugging

        close(inpipe[0]); close(inpipe[1]);
        close(outpipe[0]); close(outpipe[1]);

        execvp(argv[0], argv.data());
        // if exec fails:
        _exit(127);
    }

    // parent process
    m_pid = pid;
    // close unused ends
    close(inpipe[0]);
    close(outpipe[1]);
    m_stdin_fd = inpipe[1];
    m_stdout_fd = outpipe[0];

    // make stdout non-blocking reads with select-based blocking
    int flags = fcntl(m_stdout_fd, F_GETFL, 0);
    fcntl(m_stdout_fd, F_SETFL, flags & ~O_NONBLOCK);
}
    
void UciEngine::_stop_process() {
    if (m_pid <= 0) return;
    // attempt graceful quit
    _send_line("quit");
    // give some time then kill
    int status = 0;
    for (int i = 0; i < 5; ++i) {
        pid_t r = waitpid(m_pid, &status, WNOHANG);
        if (r == m_pid) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (waitpid(m_pid, &status, WNOHANG) == 0) {
        kill(m_pid, SIGKILL);
        waitpid(m_pid, &status, 0);
    }
    if (m_stdin_fd >= 0) { close(m_stdin_fd); m_stdin_fd = -1; }
    if (m_stdout_fd >= 0) { close(m_stdout_fd); m_stdout_fd = -1; }
    m_pid = -1;
}

void UciEngine::_send_line(const std::string& line) {
    if (m_stdin_fd < 0) throw std::runtime_error("UciEngine: stdin closed");
    std::string s = line + "\n";
    ssize_t wrote = ::write(m_stdin_fd, s.c_str(), s.size());
    if (wrote < 0) throw std::runtime_error(std::string("UciEngine: write failed: ") + strerror(errno));
    fsync(m_stdin_fd);
}

std::string UciEngine::_read_line(bool& eof_reached) {
    if (m_stdout_fd < 0) throw std::runtime_error("UciEngine: stdout closed");

    eof_reached = false;
    char buf[512];
    for (;;) {
        // if there is a full line in the buffer, return it
        auto pos = m_read_buffer.find('\n');
        if (pos != std::string::npos) {
            std::string line = m_read_buffer.substr(0, pos);
            m_read_buffer.erase(0, pos + 1);
            return trim(line);
        }

        // read more data
        ssize_t r = ::read(m_stdout_fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error(std::string("UciEngine: read error: ") + strerror(errno));
        }
        if (r == 0) {
            // EOF: return any remaining data, else empty
            if (m_read_buffer.empty()) return std::string();
            std::string line = m_read_buffer;
            m_read_buffer.clear();
            eof_reached = true;
            return trim(line);
        }
        m_read_buffer.append(buf, buf + r);
    }
}

void UciEngine::_set_board(const FEN& fen) {
    m_fen = fen;
    m_move_list.clear();
}

void UciEngine::_apply_move(const UCI& move) {
    m_move_list.push_back(move);
}

void UciEngine::_undo_move() {
    if (m_move_list.empty()) throw std::invalid_argument("UciEngine::undo_move() - no move to undo");
    m_move_list.pop_back();
}

std::string UciEngine::_position_command() const {
    std::ostringstream oss;
    if (!m_fen.empty()) {
        oss << "position fen " << m_fen;
    } else {
        oss << "position startpos";
    }
    if (!m_move_list.empty()) {
        oss << " moves";
        for (auto &m : m_move_list) oss << " " << m;
    }
    return oss.str();
}

UCI UciEngine::_parse_bestmove(const std::string& line) const {
    // bestmove <move> [ponder <move>]
    std::istringstream iss(line);
    std::string tok;
    iss >> tok;
    if (tok != "bestmove") throw std::runtime_error("UciEngine: expected bestmove, got: " + line);
    std::string move;
    iss >> move;
    if (move.empty() || move == "(none)") throw std::runtime_error("UciEngine: engine returned no bestmove");
    return move;
}

UCI UciEngine::_compute_move() {
    bool is_eof = false;

    if (m_pid <= 0) {
        _start_process();

        // UCI handshake
        _send_line("uci");
        std::string line;
        while (true) {
            line = _read_line(is_eof);
            if (is_eof) throw std::runtime_error("UciEngine: engine closed during uci handshake");
            if (trim(line) == "uciok") break;
            // options ignored for now
        }
        _send_line("isready");
        // wait for readyok
        while (true) {
            line = _read_line(is_eof);
            if (is_eof) throw std::runtime_error("UciEngine: engine closed during isready");
            if (trim(line) == "readyok") break;
        }
    }

    // send current position
    _send_line(_position_command());
    // ensure engine ready
    _send_line("isready");
    // drain until readyok
    while (true) {
        auto line = _read_line(is_eof);
        if (is_eof) throw std::runtime_error("UciEngine: engine closed while waiting readyok");
        if (line == "readyok") break;
    }

    // issue go with movetime
    std::ostringstream go;
    go << "go movetime " << m_default_movetime_ms;
    _send_line(go.str());

    // wait for bestmove
    while (true) {
        auto line = _read_line(is_eof);
        if (is_eof) {
            throw std::runtime_error("UciEngine: engine closed while waiting bestmove");
        }
        if (line.rfind("info", 0) == 0) {
            if (m_enable_info) {
                std::cout << line << std::endl;
            }
            continue;
        }
        else if (line.rfind("bestmove", 0) == 0) {
            return _parse_bestmove(line);
        } 
        else {
            // ignore other lines
            continue;
        }
    }
}

#endif
