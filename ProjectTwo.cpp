// ProjectTwo.cpp
// CS 300 Project Two - ABCU Advising Assistance Program
// Binary search tree implementation based on Project One pseudocode.
// Build: g++ -std=c++11 -Wall -Wextra -pedantic ProjectTwo.cpp -o ProjectTwo
// Uses only C++ standard library headers.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

struct Course {
    string courseNumber;
    string title;
    vector<string> prerequisites;
};

string trim(const string& value) {
    const string whitespace = " \t\r\n";
    const size_t start = value.find_first_not_of(whitespace);
    if (start == string::npos) return "";
    const size_t finish = value.find_last_not_of(whitespace);
    return value.substr(start, finish - start + 1);
}

string toUpper(string value) {
    for (char& character : value) {
        character = static_cast<char>(toupper(static_cast<unsigned char>(character)));
    }
    return value;
}

// Split a comma-separated course record, retaining empty fields.
// The supplied ABCU file uses unquoted comma-separated values.
vector<string> splitFields(const string& line) {
    vector<string> fields;
    size_t start = 0;
    while (true) {
        const size_t comma = line.find(',', start);
        fields.push_back(trim(line.substr(start, comma == string::npos
                                                 ? string::npos : comma - start)));
        if (comma == string::npos) break;
        start = comma + 1;
    }
    return fields;
}

class BinarySearchTree {
private:
    struct Node {
        Course course;
        unique_ptr<Node> left;
        unique_ptr<Node> right;
        explicit Node(const Course& data) : course(data) {}
    };

    unique_ptr<Node> root;

    // Building from the middle of sorted records keeps the tree balanced,
    // including when the input file happens to be alphabetically ordered.
    static unique_ptr<Node> buildBalanced(const vector<Course>& sorted,
                                          size_t begin, size_t end) {
        if (begin >= end) return unique_ptr<Node>();
        const size_t middle = begin + (end - begin) / 2;
        unique_ptr<Node> node(new Node(sorted[middle]));
        node->left = buildBalanced(sorted, begin, middle);
        node->right = buildBalanced(sorted, middle + 1, end);
        return node;
    }

    static void printInOrder(const Node* node) {
        if (node == nullptr) return;
        printInOrder(node->left.get());
        cout << node->course.courseNumber << ", " << node->course.title << '\n';
        printInOrder(node->right.get());
    }

public:
    void buildFromCourses(vector<Course> courses) {
        sort(courses.begin(), courses.end(),
             [](const Course& a, const Course& b) {
                 return a.courseNumber < b.courseNumber;
             });
        root = buildBalanced(courses, 0, courses.size());
    }

    const Course* find(const string& courseNumber) const {
        const Node* current = root.get();
        while (current != nullptr) {
            if (courseNumber == current->course.courseNumber)
                return &current->course;
            current = courseNumber < current->course.courseNumber
                          ? current->left.get() : current->right.get();
        }
        return nullptr;
    }

    void printCourseList() const { printInOrder(root.get()); }
};

// Parse and validate the complete file before replacing the active tree.
// Blank fields at the END of a row are permitted by the provided ABCU CSV.
bool loadCourses(const string& filename, BinarySearchTree& destination,
                 string& error) {
    ifstream input(filename.c_str());
    if (!input) {
        error = "Unable to open file: " + filename;
        return false;
    }

    vector<Course> pending;
    set<string> knownNumbers;
    string line;
    size_t lineNumber = 0;

    while (getline(input, line)) {
        ++lineNumber;
        const vector<string> fields = splitFields(line);
        if (fields.size() < 2 || fields[0].empty() || fields[1].empty()) {
            error = "Line " + to_string(lineNumber) +
                    ": course number and title are required.";
            return false;
        }

        Course course;
        course.courseNumber = toUpper(fields[0]);
        course.title = fields[1];
        if (!knownNumbers.insert(course.courseNumber).second) {
            error = "Line " + to_string(lineNumber) +
                    ": duplicate course number " + course.courseNumber + ".";
            return false;
        }

        set<string> seenPrerequisites;
        for (size_t i = 2; i < fields.size(); ++i) {
            if (fields[i].empty()) {
                // Trailing empty columns are placeholders, but an empty
                // column between prerequisites is a malformed record.
                for (size_t j = i + 1; j < fields.size(); ++j) {
                    if (!fields[j].empty()) {
                        error = "Line " + to_string(lineNumber) +
                                ": empty field between prerequisites.";
                        return false;
                    }
                }
                break;
            }
            const string prerequisite = toUpper(fields[i]);
            if (prerequisite == course.courseNumber) {
                error = "Line " + to_string(lineNumber) +
                        ": a course cannot require itself.";
                return false;
            }
            if (!seenPrerequisites.insert(prerequisite).second) {
                error = "Line " + to_string(lineNumber) +
                        ": duplicate prerequisite " + prerequisite + ".";
                return false;
            }
            course.prerequisites.push_back(prerequisite);
        }
        pending.push_back(course);
    }

    if (input.bad()) {
        error = "Error while reading the course file.";
        return false;
    }
    if (pending.empty()) {
        error = "The course file is empty.";
        return false;
    }

    // Validate references only after every course number has been collected.
    for (const Course& course : pending) {
        for (const string& prerequisite : course.prerequisites) {
            if (knownNumbers.count(prerequisite) == 0) {
                error = "Course " + course.courseNumber +
                        " references missing prerequisite " + prerequisite + ".";
                return false;
            }
        }
    }

    destination.buildFromCourses(std::move(pending));
    return true;
}

void printCourseInformation(const BinarySearchTree& tree, const string& input) {
    const string number = toUpper(trim(input));
    const Course* course = tree.find(number);
    if (course == nullptr) {
        cout << "Course " << number << " was not found.\n";
        return;
    }

    cout << course->courseNumber << ", " << course->title << '\n';
    if (course->prerequisites.empty()) {
        cout << "Prerequisites: None\n";
        return;
    }

    cout << "Prerequisites:\n";
    for (const string& prerequisiteNumber : course->prerequisites) {
        const Course* prerequisite = tree.find(prerequisiteNumber);
        // Every prerequisite was checked during file loading.
        if (prerequisite != nullptr) {
            cout << "  " << prerequisite->courseNumber << ", "
                 << prerequisite->title << '\n';
        }
    }
}

int main() {
    BinarySearchTree courses;
    bool dataLoaded = false;
    string choice;

    cout << "Welcome to the ABCU Advising Assistance Program!\n";
    while (true) {
        cout << "\n1. Load Course Data\n"
             << "2. Print Course List\n"
             << "3. Print Course Information\n"
             << "9. Exit\n"
             << "Enter your choice: ";

        if (!getline(cin, choice)) break;
        choice = trim(choice);

        if (choice == "1") {
            cout << "Enter the course data filename: ";
            string filename;
            if (!getline(cin, filename)) break;
            filename = trim(filename);
            if (filename.empty()) {
                cout << "Error: A filename is required.\n";
                continue;
            }

            BinarySearchTree replacement;
            string error;
            if (loadCourses(filename, replacement, error)) {
                courses = std::move(replacement);
                dataLoaded = true;
                cout << "Course data loaded successfully.\n";
            } else {
                cout << "Error: " << error << '\n'
                     << "Previously loaded course data, if any, is unchanged.\n";
            }
        } else if (choice == "2") {
            if (!dataLoaded) {
                cout << "Please load course data first (Option 1).\n";
            } else {
                cout << "\nComputer Science and Math Courses:\n";
                courses.printCourseList();
            }
        } else if (choice == "3") {
            if (!dataLoaded) {
                cout << "Please load course data first (Option 1).\n";
            } else {
                cout << "Enter the course number: ";
                string courseNumber;
                if (!getline(cin, courseNumber)) break;
                if (trim(courseNumber).empty()) {
                    cout << "Error: A course number is required.\n";
                } else {
                    printCourseInformation(courses, courseNumber);
                }
            }
        } else if (choice == "9") {
            cout << "Thank you for using the ABCU Advising Assistance Program.\n";
            break;
        } else {
            cout << "Invalid option. Please enter 1, 2, 3, or 9.\n";
        }
    }
    return 0;
}
