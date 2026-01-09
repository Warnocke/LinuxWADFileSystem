#include "Wad.h"

// destructor, closes file descriptor
Wad::~Wad() {
    close(file_descriptor);
}

// Constructor, reads the passed in WAD file to set up class variables and organize data from the file into proper hierarchy
Wad::Wad(const string& path) {
    this->file_descriptor = open(path.c_str(), O_RDWR);
    if (file_descriptor < 0) { return; }

    char header[12];
    read(file_descriptor, header, 12);

    Magic.assign(header, 4);
    memcpy(&num_descriptors, header + 4, 4);
    memcpy(&descriptor_offset, header + 8, 4);
    // now I need to loop through the file_descriptor to set up both of my maps and vector

    string map_marker;
    stack<string> name_marker;
    int map_el_counter = 0;
    
    pathToDescriptor["/"] = 0;
    pathToChildren["/"] = vector<string>();
    descriptors.push_back(Descriptor{0, 0, "/", true, -1});
    char desc[16];
    
    for (int i = 0; i < num_descriptors; i++) {

        lseek(file_descriptor, descriptor_offset + i * 16, SEEK_SET); // Changes the pointer start in fd
        read(file_descriptor, desc, 16);
        
        int el_offset;
        int el_length;
        string el_name;
        string path;
        char name_buff[9];
        name_buff[8] = '\0';

        memcpy(&el_offset, desc, 4);
        memcpy(&el_length, desc + 4, 4);
        memcpy(name_buff, desc + 8, 8);
        el_name = string(name_buff);

        if (el_name.size() >= 4 && el_name.compare(el_name.size() - 4, 4, "_END") == 0) {
            descriptors.push_back(Descriptor{el_offset, el_length, el_name, true, -1});

            string parent = pathMaker(name_marker, map_marker);
            string base = el_name.substr(0, el_name.size() - 4);
            if (!parent.empty()) {
                path = "/" + parent + "/" + base;
            } else {
                path = "/" + base;
            }

            auto it = pathToDescriptor.find(path);
            if (it != pathToDescriptor.end()) {
                int startIdx = it->second;
                if (startIdx >= 0 && startIdx < (int)descriptors.size()) {
                    descriptors[startIdx].endMarkerIndex = descriptors.size() - 1;
                }
            } 
            if (!name_marker.empty()) name_marker.pop();
            continue;
        }

        if (el_name.size() >= 6 && el_name.compare(el_name.size() - 6, 6, "_START") == 0) {
            descriptors.push_back(Descriptor{el_offset, el_length, el_name, true, -1});
            string parent = pathMaker(name_marker, map_marker);
            if (!parent.empty()) {
                path = "/" + parent + '/' + el_name.substr(0, el_name.size() - 6);
            } else {
                path = "/" + el_name.substr(0, el_name.size() - 6);
            }
            string parentKey = (parent.empty()) ? "/" : ("/" + parent);
            pathToDescriptor[path] = descriptors.size() - 1;
            pathToChildren[parentKey].push_back(el_name.substr(0, el_name.size() - 6));
            pathToChildren[path] = vector<string>();
            name_marker.push(el_name.substr(0, el_name.size() - 6));
            continue;
        }

        // holy chopped 
        if (el_name.size() == 4 &&
            el_name[0] == 'E' &&
            el_name[2] == 'M' &&
            isdigit(el_name[1]) &&
            isdigit(el_name[3])) {

            map_marker = el_name;
            map_el_counter = 0;
            string parent = pathMaker(name_marker, "");
            if (!parent.empty()) {
                path = "/" + parent + '/' + el_name;
            } else {
                path = "/" + el_name;
            }
            string parentKey = (parent.empty()) ? "/" : ("/" + parent);
            pathToChildren[parentKey].push_back(el_name);
            pathToChildren[path] = vector<string>();
            descriptors.push_back(Descriptor{el_offset, el_length, el_name, true, -1});
            pathToDescriptor[path] = descriptors.size() - 1;
            continue;
        }
        
        // logic for regular content now
        descriptors.push_back(Descriptor{el_offset, el_length, el_name, false, -1});
        string parent = pathMaker(name_marker, map_marker);

        if (!parent.empty()) {
            path = "/" + parent + "/" + el_name;
        } else {
            path = "/" + el_name;
        }
        string parentKey = (parent.empty()) ? "/" : ("/" + parent);
        pathToDescriptor[path] = descriptors.size() - 1;
        pathToChildren[parentKey].push_back(el_name);
        if (map_marker.size() != 0) {
            map_el_counter++;
            if (map_el_counter == 10) { map_marker = ""; }
        }
    }
}
// helper function to create paths based on current namespace stack and map marker
string Wad::pathMaker(stack<string> nameSpace, string mapMarker) {
    if (nameSpace.empty() && mapMarker.empty()) {
        return "";  // Root level
    }
    
    vector<string> parts;
    stack<string> temp = nameSpace;
    
    // Pop stack into vector to reverse order
    while (!temp.empty()) {
        parts.push_back(temp.top());
        temp.pop();
    }
    
    // Reverse to get correct order
    reverse(parts.begin(), parts.end());
    
    string result = "";
    for (const string& part : parts) {
        result += part + "/";
    }
    
    if (!mapMarker.empty()) {
        result += mapMarker;
    } else if (!result.empty()) {
        result.pop_back();  // Remove trailing slash
    }
    
    return result;
}
// lets us initialize a Wad object from outside the class
Wad* Wad::loadWad(const string &path) {
    Wad* my_wad = new Wad(path);
    return my_wad;
}

// normalizes paths by removing trailing slashes (except for root)
string Wad::normalizePath(const string &p) {
    if (p.empty()) return p;
    if (p == "/") return "/";
    string s = p;
    while (s.size() > 1 && s.back() == '/') s.pop_back();
    return s;
}

// simple getter for Magic
string Wad::getMagic() {
    return Magic;
}

// traverses map to see if path is content
bool Wad::isContent(const string &path) {
    string p = normalizePath(path);
    if (p == "/") { return false; }
    if (auto it = pathToDescriptor.find(p); it != pathToDescriptor.end()) {
        return !descriptors[it->second].isDirectory;
    }
    return false;    
}

// traverses map to see if path is directory
bool Wad::isDirectory(const string &path) {
    string p = normalizePath(path);
    if (p == "/") { return true; }
    if (auto it = pathToDescriptor.find(p); it != pathToDescriptor.end()) {
        return descriptors[it->second].isDirectory;
    }
    return false;
}
// checks if path exists and is content, then returns size
int Wad::getSize(const string &path) {
    string p = normalizePath(path);
    if (p == "/") { return -1; }
    if (auto it = pathToDescriptor.find(p); it != pathToDescriptor.end()) {
        if (descriptors[it->second].isDirectory) return -1;
        return descriptors[it->second].length;
    }
    return -1;
}

// reads content from a file at a given path into a buffer
int Wad::getContents(const string &path, char *buffer, int length, int offset) {
    string p = normalizePath(path);
    if (!isContent(p)) return -1;

    int fd_offset = descriptors[pathToDescriptor[p]].offset;
    int fd_length = descriptors[pathToDescriptor[p]].length;
    
    int bytes_available = fd_length - offset;
    if (bytes_available <= 0) return 0;
    
    int bytes_to_read = min(bytes_available, length);
    
    lseek(file_descriptor, fd_offset + offset, SEEK_SET);
    int bytes_read = read(file_descriptor, buffer, bytes_to_read);
    return bytes_read;
}

// traverses map to see if path is directory, then copies child vector and returns size
int Wad::getDirectory(const string &path, vector<string> *directory) {
    string p = normalizePath(path);
    if (!isDirectory(p)) return -1;
    
    const vector<string>& children = pathToChildren[p];
    directory->assign(children.begin(), children.end());
    return children.size();
}

// validates path and name, parses parent directory, then inserts new directory descriptors and updates maps/ vectors. Calls both write functions to update file
void Wad::createDirectory(const string &path) {
    string p = normalizePath(path);
    
    int lastSlash = p.find_last_of('/');
    string name = p.substr(lastSlash + 1);
    string parent = (lastSlash == 0) ? "/" : p.substr(0, lastSlash);

    if (!isDirectory(parent)) { return; }
    if (containsMapMarker(parent)) { return; }
    if (name.size() > 2) { return; }
    if (pathToDescriptor.find(p) != pathToDescriptor.end()) { return; }

    int idx;
    if (parent == "/") {
        idx = descriptors.size();
    } else {
        idx = descriptors[pathToDescriptor[parent]].endMarkerIndex;
    }    
    
    descriptors.insert(descriptors.begin() + idx, Descriptor{0, 0, name + "_START", true, -1});
    descriptors.insert(descriptors.begin() + idx + 1, Descriptor{0, 0, name + "_END", true, -1});
    
    descriptors[idx].endMarkerIndex = idx + 1;
    pathToChildren[parent].push_back(name);

    for (auto& it : pathToDescriptor) {
        if (it.second >= idx) {
            it.second += 2; 
        }
    }
    
    for (int i = 0; i < (int)descriptors.size(); i++) {
        if (i != idx && i != idx + 1) {
            if (descriptors[i].endMarkerIndex >= idx) {
                descriptors[i].endMarkerIndex += 2;
            }
        }
    }

    pathToDescriptor[p] = idx;
    pathToChildren[p] = vector<string>();
    
    num_descriptors += 2;
    writeDescriptorsToFile();
    writeHeaderToFile();
}
// helper function to check if any segment of the path is a map marker
bool Wad::containsMapMarker(const string &path) {
    size_t start = 1; 
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == string::npos) end = path.size();
        
        string segment = path.substr(start, end - start);
        
        if (segment.size() == 4 &&
            segment[0] == 'E' &&
            isdigit(segment[1]) &&
            segment[2] == 'M' &&
            isdigit(segment[3])) {
            return true;
        }
        
        start = end + 1;
    }
    return false;
}
// validates path and name, parses parent directory, then inserts new file descriptor and updates maps/ vectors. Calls both write functions to update file
void Wad::createFile(const string &path) {
    string p = normalizePath(path);
    
    if (pathToDescriptor.find(p) != pathToDescriptor.end()) {
        return;
    }

    size_t lastSlash = p.find_last_of('/');
    string name = p.substr(lastSlash + 1);
    string parent = (lastSlash == 0) ? "/" : p.substr(0, lastSlash);

    if (!isDirectory(parent)) { return; }
    if (containsMapMarker(parent)) { return; }
    if (name.size() > 8) { return; }
    
    if (name.find("_START") != string::npos || name.find("_END") != string::npos) {
        return;
    }
    if (name.size() == 4 && name[0] == 'E' && isdigit(name[1]) && 
        name[2] == 'M' && isdigit(name[3])) {
        return;
    }

    int idx;
    if (parent == "/") {
        idx = descriptors.size();
    } else {
        idx = descriptors[pathToDescriptor[parent]].endMarkerIndex;
    }  

    descriptors.insert(descriptors.begin() + idx, Descriptor{0, 0, name, false, -1});
    pathToChildren[parent].push_back(name);

    for (auto& it : pathToDescriptor) {
        if (it.second >= idx) {
            it.second += 1; 
        }
    }
    

    for (auto& desc : descriptors) {
        if (desc.endMarkerIndex >= idx) {
            desc.endMarkerIndex += 1;
        }
    }

    pathToDescriptor[p] = idx;
    
    num_descriptors += 1;
    writeDescriptorsToFile();
    writeHeaderToFile();
}

// writes content from buffer to a file at a given path, updates descriptor, and calls both write functions to update file
int Wad::writeToFile(const string &path, const char *buffer, int length, int offset) {
    string p = normalizePath(path);
    if (!isContent(p)) { return -1; }
    if (getSize(p) != 0) { return 0; }
    
    int idx = pathToDescriptor[p];
    
    lseek(file_descriptor, descriptor_offset + offset, SEEK_SET);
    int written = write(file_descriptor, buffer, length);
    
    descriptors[idx].offset = descriptor_offset + offset;
    descriptors[idx].length = written;
    
    descriptor_offset = descriptor_offset + offset + written;
    
    writeDescriptorsToFile();
    writeHeaderToFile();
    
    return written;
}

// Updates the WAD file header based on current class variables
void Wad::writeHeaderToFile() {
    lseek(file_descriptor, 0, SEEK_SET);
    write(file_descriptor, Magic.c_str(), 4);
    write(file_descriptor, &num_descriptors, 4);
    write(file_descriptor, &descriptor_offset, 4);
}
// Updates the descriptors in the WAD file based on the current state of the descriptors vector
void Wad::writeDescriptorsToFile() {
    lseek(file_descriptor, descriptor_offset, SEEK_SET);
    for (int i = 1; i < (int)descriptors.size() && i <= num_descriptors; ++i) {
        const auto &desc = descriptors[i];
        write(file_descriptor, &desc.offset, 4);
        write(file_descriptor, &desc.length, 4);
        char name_buf[8] = {0};
        strncpy(name_buf, desc.name.c_str(), 8);
        write(file_descriptor, name_buf, 8);
    }
}