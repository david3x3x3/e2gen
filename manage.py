#!/usr/bin/env python3
"""manage.py — parallel solver manager with ncurses UI"""

import curses
import math
import os
import re
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path

SOLVER_NAME = "10x10-row-10-19864204"
BASE_DIR = Path(__file__).parent.resolve()
SOLVER_PATH = BASE_DIR / SOLVER_NAME
LOG_DIR = BASE_DIR / "logs"
REFRESH_INTERVAL = 1.0   # seconds between log reads
LOG_TAIL_LINES = 8       # lines shown in info panel
IMPROVED_TTL = 5.0       # seconds to highlight a cell after a new best
SHUTDOWN_TIMEOUT = 10    # seconds to wait for graceful solver exit

STATUS_RE = re.compile(
    r"status best=(\d+) \((\S+)\) nodes=(\d+) time=\d+ rate=(\d+) restarts=(\d+)"
)

# curses color pair IDs
CP_RUNNING    = 1
CP_DEAD       = 2
CP_SEL_RUN    = 3
CP_SEL_DEAD   = 4
CP_IMPROVED   = 5
CP_LABEL      = 6


def fmt_duration(secs: float) -> str:
    secs = int(secs)
    h, rem = divmod(secs, 3600)
    m, s = divmod(rem, 60)
    if h:
        return f"{h}h {m:02d}m {s:02d}s"
    return f"{m}m {s:02d}s"


def fmt_big(n: int) -> str:
    if n >= 1_000_000_000:
        return f"{n / 1e9:.1f}B"
    if n >= 1_000_000:
        return f"{n / 1e6:.1f}M"
    if n >= 1_000:
        return f"{n / 1e3:.1f}K"
    return str(n)


@dataclass
class Worker:
    core: int
    log_path: Path
    proc: subprocess.Popen | None = None
    start_time: float = 0.0
    best: int = 0
    best_node_disp: str = "0"
    nodes: int = 0
    rate: int = 0
    restarts: int = 0
    log_file_pos: int = 0
    last_log_lines: list = field(default_factory=list)
    alive: bool = False
    improved_at: float = 0.0  # time.time() of last best improvement


class Manager:
    def __init__(self, nworkers: int):
        self.nworkers = nworkers
        # grid_cols chosen to make the grid roughly square
        self.grid_cols = math.ceil(math.sqrt(nworkers))
        self.workers = [
            Worker(core=i, log_path=LOG_DIR / f"solver-{i}.log")
            for i in range(nworkers)
        ]
        self.lock = threading.Lock()
        self.cursor = 0
        self.running = True
        self.global_best = 0
        self.global_best_core = -1

    # ------------------------------------------------------------------ #
    #  Process management                                                  #
    # ------------------------------------------------------------------ #

    def start_workers(self):
        LOG_DIR.mkdir(exist_ok=True)
        for w in self.workers:
            self._start_worker(w)

    def _start_worker(self, w: Worker):
        log_fp = open(w.log_path, "a")
        proc = subprocess.Popen(
            [str(SOLVER_PATH), str(w.core)],
            stdout=log_fp,
            stderr=log_fp,
            cwd=str(BASE_DIR),
        )
        log_fp.close()  # child inherited the fd; parent can close its copy
        w.proc = proc
        w.start_time = time.time()
        w.alive = True

    def stop_all(self):
        self.running = False
        for w in self.workers:
            if w.proc and w.alive:
                try:
                    w.proc.send_signal(signal.SIGTERM)
                except (ProcessLookupError, OSError):
                    pass
        deadline = time.time() + SHUTDOWN_TIMEOUT
        for w in self.workers:
            if w.proc:
                remaining = max(0.1, deadline - time.time())
                try:
                    w.proc.wait(timeout=remaining)
                except subprocess.TimeoutExpired:
                    try:
                        w.proc.kill()
                    except OSError:
                        pass
            w.alive = False

    # ------------------------------------------------------------------ #
    #  Log reader (background thread)                                      #
    # ------------------------------------------------------------------ #

    def _log_reader(self):
        while self.running:
            with self.lock:
                for w in self.workers:
                    self._read_new_lines(w)
                    if w.alive and w.proc and w.proc.poll() is not None:
                        w.alive = False
                    if not w.alive and self.running:
                        self._start_worker(w)
            time.sleep(REFRESH_INTERVAL)

    def _read_new_lines(self, w: Worker):
        try:
            with open(w.log_path, "r", errors="replace") as f:
                f.seek(w.log_file_pos)
                new_lines = f.readlines()
                w.log_file_pos = f.tell()
        except FileNotFoundError:
            return
        for raw in new_lines:
            line = raw.rstrip()
            w.last_log_lines.append(line)
            m = STATUS_RE.search(line)
            if m:
                new_best = int(m.group(1))
                if new_best > w.best:
                    w.improved_at = time.time()
                    w.best = new_best
                    if new_best > self.global_best:
                        self.global_best = new_best
                        self.global_best_core = w.core
                w.best_node_disp = m.group(2)
                w.nodes = int(m.group(3))
                w.rate = int(m.group(4))
                w.restarts = int(m.group(5))
        # rolling buffer — keep the last 500 lines per worker
        if len(w.last_log_lines) > 500:
            w.last_log_lines = w.last_log_lines[-500:]

    # ------------------------------------------------------------------ #
    #  ncurses UI                                                          #
    # ------------------------------------------------------------------ #

    def run_ui(self, stdscr):
        curses.curs_set(0)
        stdscr.timeout(500)  # getch() blocks at most 500 ms; drives redraw rate
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(CP_RUNNING,  curses.COLOR_GREEN,  -1)
        curses.init_pair(CP_DEAD,     curses.COLOR_RED,    -1)
        curses.init_pair(CP_SEL_RUN,  curses.COLOR_BLACK,  curses.COLOR_GREEN)
        curses.init_pair(CP_SEL_DEAD, curses.COLOR_BLACK,  curses.COLOR_RED)
        curses.init_pair(CP_IMPROVED, curses.COLOR_YELLOW, -1)
        curses.init_pair(CP_LABEL,    curses.COLOR_CYAN,   -1)

        while self.running:
            key = stdscr.getch()
            if key in (ord('q'), ord('Q')):
                self.running = False
                break
            elif key == curses.KEY_UP:
                self.cursor = max(0, self.cursor - self.grid_cols)
            elif key == curses.KEY_DOWN:
                self.cursor = min(self.nworkers - 1, self.cursor + self.grid_cols)
            elif key == curses.KEY_LEFT:
                self.cursor = max(0, self.cursor - 1)
            elif key == curses.KEY_RIGHT:
                self.cursor = min(self.nworkers - 1, self.cursor + 1)
            elif key == ord('v'):
                self._view_log(stdscr, self.workers[self.cursor])

            self._draw(stdscr)

    def _view_log(self, stdscr, w: Worker):
        """Suspend ncurses, open a pager on the worker's log, then restore."""
        curses.endwin()
        pager = os.environ.get("PAGER", "less")
        try:
            subprocess.run([pager, str(w.log_path)])
        except FileNotFoundError:
            # $PAGER not found; try less then more
            try:
                subprocess.run(["less", str(w.log_path)])
            except FileNotFoundError:
                subprocess.run(["more", str(w.log_path)])
        # Calling refresh() re-enters curses mode after endwin()
        stdscr.refresh()

    def _draw(self, stdscr):
        stdscr.erase()
        rows, cols = stdscr.getmaxyx()

        cell_w = 4            # " NNN" per grid cell (space + 3-digit core number)
        grid_w = self.grid_cols * cell_w
        sep_x = grid_w        # column of the vertical separator
        info_x = sep_x + 1   # info panel starts here
        info_w = max(0, cols - info_x - 1)

        # Title bar
        title = f" Solver Manager — {SOLVER_NAME}  ({self.nworkers} workers)"
        self._puts(stdscr, 0, 0, title.ljust(cols)[:cols], curses.A_REVERSE)

        now = time.time()
        with self.lock:
            # Grid of worker cells
            for idx, w in enumerate(self.workers):
                grow = idx // self.grid_cols
                gcol = idx % self.grid_cols
                y = 1 + grow
                x = gcol * cell_w
                if y >= rows - 1:
                    break
                is_sel = (idx == self.cursor)
                is_improved = (now - w.improved_at) < IMPROVED_TTL
                cell = f"{w.core:3d} "
                if is_sel:
                    attr = curses.color_pair(CP_SEL_RUN if w.alive else CP_SEL_DEAD)
                elif is_improved:
                    attr = curses.color_pair(CP_IMPROVED) | curses.A_BOLD
                elif w.alive:
                    attr = curses.color_pair(CP_RUNNING)
                else:
                    attr = curses.color_pair(CP_DEAD)
                self._puts(stdscr, y, x, cell, attr)

            # Vertical separator between grid and info panel
            for y in range(1, rows - 1):
                self._puts(stdscr, y, sep_x, "│")

            # Info panel — selected worker
            sel = self.workers[self.cursor]
            runtime = fmt_duration(now - sel.start_time) if sel.start_time else "—"
            pid_s = str(sel.proc.pid) if sel.proc else "—"

            fields = [
                ("Core",     str(sel.core)),
                ("PID",      pid_s),
                ("Status",   "running" if sel.alive else "stopped"),
                ("Runtime",  runtime),
                ("Best",     f"{sel.best}  (node {sel.best_node_disp})"),
                ("Nodes",    fmt_big(sel.nodes)),
                ("Rate",     f"{fmt_big(sel.rate)}/s"),
                ("Restarts", f"{sel.restarts:,}"),
            ]
            for i, (label, value) in enumerate(fields):
                y = 1 + i
                if y >= rows - 1:
                    break
                lbl = f"  {label:<10}: "
                self._puts(stdscr, y, info_x, lbl[:info_w], curses.color_pair(CP_LABEL))
                self._puts(stdscr, y, info_x + len(lbl), value[:max(0, info_w - len(lbl))])

            # Log tail
            log_hdr_y = 1 + len(fields) + 1
            if log_hdr_y < rows - 1:
                hdr = f"  — log: {sel.log_path.name} —"
                self._puts(stdscr, log_hdr_y, info_x, hdr[:info_w],
                           curses.color_pair(CP_LABEL))
            for i, line in enumerate(sel.last_log_lines[-LOG_TAIL_LINES:]):
                y = log_hdr_y + 1 + i
                if y >= rows - 1:
                    break
                self._puts(stdscr, y, info_x + 2, line[:max(0, info_w - 2)])

        # Status bar
        running_count = sum(1 for w in self.workers if w.alive)
        gb = (f"Best-ever: {self.global_best} (core {self.global_best_core})"
              if self.global_best_core >= 0 else "Best-ever: —")
        bar = (f"  Running: {running_count}/{self.nworkers}   {gb}"
               f"   [q] quit  [v] view log  [←→↑↓] navigate")
        self._puts(stdscr, rows - 1, 0, bar.ljust(cols)[:cols], curses.A_REVERSE)

        curses.doupdate()

    @staticmethod
    def _puts(win, y: int, x: int, s: str, attr: int = 0):
        rows, cols = win.getmaxyx()
        if y < 0 or y >= rows or x < 0 or x >= cols:
            return
        s = s[: cols - x]
        if not s:
            return
        try:
            win.addstr(y, x, s, attr)
        except curses.error:
            pass


def main():
    nworkers = int(subprocess.check_output(["nproc"]).strip())
    if nworkers == 0:
        sys.exit("nproc returned 0")

    manager = Manager(nworkers)

    def _sig(sig, frame):
        manager.running = False

    signal.signal(signal.SIGTERM, _sig)
    signal.signal(signal.SIGINT, _sig)

    manager.start_workers()

    reader = threading.Thread(target=manager._log_reader, daemon=True)
    reader.start()

    try:
        curses.wrapper(manager.run_ui)
    finally:
        manager.stop_all()


if __name__ == "__main__":
    main()
