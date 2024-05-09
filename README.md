# dbl_link_list_project

Create a C++ program that implements a linked list. The linked list should be a doubly linked list, allowing movement forward and backward. This program should allow you to add and remove nodes from the list. Each node should contain a reference to application data. The program does not have to provide user interaction. Please include units tests for the program. The program can be submitted by including a link to your solution in Github.

Uses catch2 for unit tests:

https://github.com/catchorg/Catch2


Build commandline for creating binary with unit test runner:

g++ -std=c++20 -o dbl_link_list dbl_link_list.cpp catch_amalgamated.cpp


To run unit tests:

./dbl_link_list


