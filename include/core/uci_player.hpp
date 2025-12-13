#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>

#include "core/ai_player.hpp"
#include "core/registry.hpp"

void registerUciPlayer();

#ifndef _WIN32

/*
 Minimal UCI engine wrapper implementing AIPlayer. It spawns the external engine and exchanges UCI text.
 Config fields:
   - cmd        : string (command to execute, required)
   - args       : string (optional additional args)
   - movetime   : int (default 1000 ms per move)
*/
class UciEngine : public AIPlayer {
public:
    explicit UciEngine(const std::vector<ConfigField>& cfg);
    ~UciEngine();

private:
    void _set_board(const FEN& fen) override;
    void _apply_move(const UCI& move) override;
    void _undo_move() override;
    UCI  _compute_move() override;

    // process management and io helpers
    void _start_process();
    void _stop_process();
    void _send_line(const std::string& line);
    std::string _read_line(bool& eof_reached);
    void _expect_ok_or_timeout(const std::string& token, int timeout_ms = 2000);

    // other helpers
    std::string _position_command() const;
    UCI _parse_bestmove(const std::string& line) const;

private:
    // parameters
    std::string m_cmd;
    bool m_enable_info = false;
    int m_default_movetime_ms = 5000;

    // board state tracking
    FEN m_fen;
    std::vector<UCI> m_move_list;

    // process handles
    pid_t m_pid = -1;
    int m_stdin_fd = -1;
    int m_stdout_fd = -1;
    std::string m_read_buffer;
};

#endif
