#pragma once

#include<iostream>
#include<vector>
#include<cstring>
#include<string>
#include<unistd.h>
#include<fcntl.h>
#include<map>
#include<stack>
#include<algorithm>

using namespace std;

class Wad {
    struct Descriptor{
        int offset;
        int length;
        string name;
        bool isDirectory;
        int endMarkerIndex;
    };

    vector<Descriptor> descriptors; // stores in order descriptor objects
    map<string,int> pathToDescriptor; // key is full file path, value is int position of descriptor object in vec
    map<string,vector<string>> pathToChildren; // key is full file path, value is vector of string names of children

    int file_descriptor;
    int descriptor_offset;
    string Magic;
    int num_descriptors;

    Wad(const string& path);

    public:

    ~Wad();

    static Wad* loadWad(const string &path);
    string getMagic();
    bool isContent(const string &path);
    bool isDirectory(const string &path);
    int getSize(const string &path);
    int getContents(const string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const string &path, vector<string> *directory);
    void createDirectory(const string &path);
    void createFile(const string &path);
    int writeToFile(const string &path, const char *buffer, int length, int offset = 0);

    // helpers
    string normalizePath(const string &p);
    string pathMaker(stack<string> nameSpace, string mapMarker);
    bool containsMapMarker(const string &path);
    void writeHeaderToFile();
    void writeDescriptorsToFile();
};