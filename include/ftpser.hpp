#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <csignal>
#include <sstream>
#include <sys/epoll.h>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <iomanip> 
#include <unistd.h>
#include <sys/wait.h>
#include <netinet/in.h> //sockaddr_in, htons() 等
#include <arpa/inet.h> //inet_pton(), inet_ntoa() 等