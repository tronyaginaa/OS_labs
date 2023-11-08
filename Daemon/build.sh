sudo touch /var/run/Daemon.pid
sudo chmod o+rw /var/run/Daemon.pid
g++ -o daemon_executable main.cpp Daemon.cpp -Wall -Werror
