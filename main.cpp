#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Entry {
    std::string key;
    std::string value;
};

class KeyValueStore {
public:
    explicit KeyValueStore(const std::string& dbPath) : dbPath_(dbPath) {}

    bool LoadFromDisk() {
        std::ifstream in(dbPath_);
        if (!in.is_open()) {
            // First run: no database file yet.
            return true;
        }

        std::string line;
        while (std::getline(in, line)) {
            std::string key;
            std::string value;
            if (ParseSetRecord(line, key, value)) {
                // Preserve write order so reverse scan in Get() enforces
                // last-write-wins semantics.
                entries_.push_back(Entry{key, value});
            }
        }

        return true;
    }

    bool Set(const std::string& key, const std::string& value) {
        // Append-only persistence: every write is recorded.
        std::ofstream out(dbPath_, std::ios::app);
        if (!out.is_open()) {
            return false;
        }

        out << "SET\t" << key << "\t" << value << '\n';
        out.flush();
        if (!out.good()) {
            return false;
        }

        entries_.push_back(Entry{key, value});
        return true;
    }

    bool Get(const std::string& key, std::string& valueOut) const {
        // Read newest-to-oldest to return the latest value for a key.
        for (int i = static_cast<int>(entries_.size()) - 1; i >= 0; --i) {
            if (entries_[i].key == key) {
                valueOut = entries_[i].value;
                return true;
            }
        }

        return false;
    }

private:
    static bool ParseSetRecord(const std::string& line, std::string& keyOut, std::string& valueOut) {
        if (line.empty()) {
            return false;
        }

        // Accept "key=value" replay format.
        std::size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos && equalsPos > 0 && equalsPos + 1 < line.size()) {
            keyOut = line.substr(0, equalsPos);
            valueOut = line.substr(equalsPos + 1);
            return true;
        }

        // Normalize tabs so token parsing handles both tab/space delimiters.
        std::string normalized = line;
        for (char& c : normalized) {
            if (c == '\t') {
                c = ' ';
            }
        }

        std::istringstream iss(normalized);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }

        // Accept "key value" replay format.
        if (tokens.size() == 2) {
            keyOut = tokens[0];
            valueOut = tokens[1];
            return true;
        }

        if (tokens.size() < 3) {
            return false;
        }

        std::string command = tokens[0];
        for (char& c : command) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        if (command != "SET") {
            return false;
        }

        keyOut = tokens[1];
        valueOut = tokens[2];
        return true;
    }

    std::string dbPath_;
    std::vector<Entry> entries_;
};

static std::string ToUpper(std::string value) {
    for (char& c : value) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return value;
}

int main(int argc, char* argv[]) {
    KeyValueStore store("data.db");
    if (!store.LoadFromDisk()) {
        std::cout << "ERROR" << std::endl;
        return 1;
    }

    // Optional argv mode for runners that execute one command per process.
    if (argc > 1) {
        std::string command = ToUpper(argv[1]);

        if (command == "SET") {
            if (argc != 4) {
                std::cout << "ERROR" << std::endl;
                return 0;
            }
            if (!store.Set(argv[2], argv[3])) {
                std::cout << "ERROR" << std::endl;
            }
            return 0;
        }

        if (command == "GET") {
            if (argc != 3) {
                std::cout << "ERROR" << std::endl;
                return 0;
            }
            std::string value;
            if (store.Get(argv[2], value)) {
                std::cout << value << std::endl;
            } else {
                std::cout << "ERROR" << std::endl;
            }
            return 0;
        }

        if (command == "EXIT") {
            return 0;
        }

        std::cout << "ERROR" << std::endl;
        return 0;
    }

    std::string command;
    // Interactive/token-stream mode from stdin.
    while (std::cin >> command) {
        command = ToUpper(command);

        if (command == "SET") {
            std::string key;
            std::string value;
            if (!(std::cin >> key >> value)) {
                std::cout << "ERROR" << std::endl;
                continue;
            }

            if (!store.Set(key, value)) {
                std::cout << "ERROR" << std::endl;
            }
        } else if (command == "GET") {
            std::string key;
            if (!(std::cin >> key)) {
                std::cout << "ERROR" << std::endl;
                continue;
            }

            std::string value;
            if (store.Get(key, value)) {
                std::cout << value << std::endl;
            } else {
                std::cout << "ERROR" << std::endl;
            }
        } else if (command == "EXIT") {
            return 0;
        } else {
            std::cout << "ERROR" << std::endl;
        }
    }

    return 0;
}
