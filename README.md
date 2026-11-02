System Monitor
A real-time terminal-based system monitor written in C, inspired by htop. Reads directly from the Linux /proc filesystem and renders a live-updating dashboard using ncurses.

Preview
                        SYSTEM MONITOR
────────────────────────────────────────────────────────────────
CPU:   [##------------------------------------------------] 4.2%
RAM:   [####----------------------------------------------] 364 / 7861 MB (4.6%)
Uptime: 01:03:38
────────────────────────────────────────────────────────────────
PID    NAME                    MEM%
────────────────────────────────────────────────────────────────
1      systemd                 0.2%
423    bash                    0.1%
...
────────────────────────────────────────────────────────────────
Press Q to quit  |  Processes: 35  |  Uptime: 01:03:38
Features
Live CPU usage with color-coded progress bar (green / yellow / red)
RAM usage showing used vs total in MB with percentage
System uptime in HH:MM:SS format
Full process list with PID, name, and memory usage
Color coding — green for low, yellow for medium, red for high usage
Refreshes every second
Press Q to quit
Tech Stack
C (C11)
ncurses
Linux /proc filesystem
CMake
How it works
All data is read directly from the Linux kernel through the /proc virtual filesystem:

Data	Source
CPU usage	/proc/stat (two samples 0.5s apart)
RAM usage	/proc/meminfo
Uptime	/proc/uptime
Process list	/proc/[pid]/comm + /proc/[pid]/status
Setup
Requirements
Linux or WSL (Windows Subsystem for Linux)
GCC
CMake
ncurses library
Install dependencies (Ubuntu/Debian)
sudo apt install gcc cmake libncurses-dev
Build and run
git clone https://github.com/enebac23/system-monitor.git
cd system-monitor
mkdir build && cd build
cmake ..
make
./SystemMonitor
