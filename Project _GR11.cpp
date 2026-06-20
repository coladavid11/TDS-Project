/*

 Project title: STUDENT ACADEMIC RECORDS SYSTEM
 Subject : TDS4223 - Data Structures and Algorithms
 Group   : 11
 Members : LIU JIUN LE, TAN LE YONG, NG ZHE JUN, ANG QI YANG, LIM MING XUAN

*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>    // for system(), function clearScreen()
#include <stdexcept>  // for runtime_error (try/catch demonstration)
#include <iomanip>    // for setw() and setprecision() in report formatting
using namespace std;

// =====================================================================
// GLOBAL CONSTANTS
// =====================================================================
const int MAX_STUDENTS = 100;
const int MAX_COURSES  = 50;
const int MAX_STAFF    = 20;
const int MAX_TEMP_GRADES = 100; // scratch array size for sort/search extraction

// =====================================================================
// UTILITY FUNCTIONS (general purpose, no class dependency)
// =====================================================================

// Clears the console screen. Dev C++ on Windows -> system("cls")
void clearScreen() {
    system("cls");
    }

// Pauses until Enter is pressed, then returns control to the caller.
void pauseScreen() {
    cout << "\nPress Enter to continue...";
    cin.ignore();
    cin.get();
}

// Safely reads a menu choice as an integer. If the user types something
// non-numeric, this clears the error state and re-prompts instead of
// letting the program spin into a broken infinite loop.
int readIntInput() {
    int value;
    while (!(cin >> value)) {
        if (cin.eof()) {
            cout << "\nInput stream ended unexpectedly. Closing program." << endl;
            exit(0);
        }
        cin.clear();
        cin.ignore(1000, '\n');
        cout << "Invalid input. Please enter a number: ";
    }
    return value;
}

// Splits "line" on "delimiter" into "result[]" (max maxFields). Returns
// the number of fields actually found. Used by every .txt file loader
// instead of any STL parsing function.
int splitLine(string line, string result[], int maxFields, char delimiter) {
    int fieldIndex = 0;
    string current = "";
    for (int i = 0; i < (int)line.length(); i++) {
        if (line[i] == delimiter) {
            if (fieldIndex < maxFields) {
                result[fieldIndex] = current;
                fieldIndex++;
            }
            current = "";
        } else {
            current += line[i];
        }
    }
    if (fieldIndex < maxFields) {
        result[fieldIndex] = current;
        fieldIndex++;
    }
    return fieldIndex;
}

// Manual string -> int conversion (handles optional leading '-').
int stringToInt(string s) {
    int result = 0;
    int sign = 1;
    int i = 0;
    if ((int)s.length() > 0 && s[0] == '-') {
        sign = -1;
        i = 1;
    }
    for (; i < (int)s.length(); i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            result = result * 10 + (s[i] - '0');
        }
    }
    return result * sign;
}

// Pads an integer with leading zeros to a fixed width (e.g. 16 -> "016").
string intToPaddedString(int num, int width) {
    string result = "";
    int temp = num;
    if (temp == 0) {
        result = "0";
    } else {
        while (temp > 0) {
            int digit = temp % 10;
            result = (char)('0' + digit) + result;
            temp /= 10;
        }
    }
    while ((int)result.length() < width) {
        result = "0" + result;
    }
    return result;
}

// Returns true only if every character in s is a digit (and s isn't empty).
bool isAllDigits(string s) {
    if (s.length() == 0) return false;
    for (int i = 0; i < (int)s.length(); i++) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
    }
    return true;
}

// IC NUMBER RULE: must be exactly 12 digits.
bool isValidIC(string ic) {
    return isAllDigits(ic) && (int)ic.length() == 12;
}

// PHONE NUMBER RULE: must be numeric and BELOW 12 digits (i.e. 9-11 digits).
// Lower bound of 9 is a reasonable real-world minimum (assumption - tell
// me if you want a different minimum).
bool isValidPhone(string phone) {
    return isAllDigits(phone) && (int)phone.length() >= 9 && (int)phone.length() < 12;
}

// EMAIL RULE (manual character scan only - NO string::find / STL algorithm):
//   - exactly one '@' symbol
//   - that '@' is not the first or last character
//   - somewhere AFTER the '@' there must be a '.' character
//   - that '.' is not the last character either
//   - the '@' and the '.' cannot be immediately adjacent to each other
bool isValidEmail(string email) {
    int len = (int)email.length();
    if (len < 5) return false; // shortest sane shape: a@b.c -> 5 characters

    // PASS 1: walk every character once to count '@' occurrences and
    // remember its position. A plain indexed for-loop over the raw
    // string - no STL container or algorithm involved.
    int atCount = 0;
    int atPos = -1;
    for (int i = 0; i < len; i++) {
        if (email[i] == '@') {
            atCount++;
            atPos = i;
        }
    }

    if (atCount != 1) return false;                    // must be exactly one '@'
    if (atPos == 0 || atPos == len - 1) return false;   // can't be first/last char

    // PASS 2: walk forward from just after '@' looking for the first '.'.
    int dotPos = -1;
    for (int i = atPos + 1; i < len; i++) {
        if (email[i] == '.') {
            dotPos = i;
            break;
        }
    }

    if (dotPos == -1) return false;        // no '.' found after the '@'
    if (dotPos == len - 1) return false;   // '.' can't be the last character
    if (dotPos == atPos + 1) return false; // '@' and '.' can't sit next to each other

    return true;
}

// SEMESTER FORMAT RULE: must look like "SemN-YYYY" (e.g. "Sem1-2026").
// Validated with a manual character walk (mirrors the style already used
// by semesterToKey() above) - no STL parsing/regex of any kind.
bool isValidSemesterFormat(string semester) {
    int len = (int)semester.length();
    if (len < 6) return false; // shortest valid shape: "Sem1-1" has 6 chars (sanity floor)

    if (semester[0] != 'S' || semester[1] != 'e' || semester[2] != 'm') return false;

    int i = 3;
    int semDigits = 0;
    while (i < len && semester[i] != '-') {
        if (semester[i] < '0' || semester[i] > '9') return false;
        semDigits++;
        i++;
    }
    if (semDigits == 0) return false;          // no semester number before the '-'
    if (i >= len || semester[i] != '-') return false; // missing the '-' separator
    i++; // skip past '-'

    int yearDigits = 0;
    while (i < len) {
        if (semester[i] < '0' || semester[i] > '9') return false;
        yearDigits++;
        i++;
    }
    return yearDigits == 4; // year part must be exactly 4 digits, e.g. 2026
}

// Repeatedly prompts for a semester string until it matches the SemN-YYYY
// shape or the user cancels. Returns "0" as a sentinel meaning "cancelled"
// (a real semester string can never equal "0", so the sentinel is safe)
// and otherwise returns the validated semester text.
string promptSemesterInput(string promptLabel) {
    string semester;
    int attempts = 0;
    while (true) {
        cout << promptLabel << " (format SemN-YYYY, e.g. Sem1-2026, or 0 to cancel): ";
        cin >> semester;
        if (semester == "0") return "0";
        if (isValidSemesterFormat(semester)) return semester;

        cout << "Invalid semester format. Please use SemN-YYYY, e.g. Sem1-2026." << endl;
        attempts++;
        if (attempts >= 5) {
            cout << "Too many invalid attempts. Operation cancelled." << endl;
            return "0";
        }
    }
}

// GRADE WHITELIST RULE: only these exact letter-grade codes (plus the
// "Pending" placeholder used for unapproved enrollments) are accepted.
// Checked via plain string equality comparisons one at a time - no
// STL std::set/std::map is used to hold or search the whitelist.
bool isValidGradeFormat(string grade) {
    if (grade == "A+" || grade == "A"  || grade == "A-") return true;
    else if (grade == "B+" || grade == "B"  || grade == "B-")
    return true;
    else if (grade == "C+" || grade == "C"  || grade == "C-") 
    return true;
    else if (grade == "D+" || grade == "D"  || grade == "D-") 
    return true;
    else if (grade == "F")                                   
    return true;
    else if (grade == "Pending")                              
    return true;
    else    
    return false;
}

// Converts "SemX-YYYY" into a single comparable integer key, e.g.
// "Sem1-2024" -> 20241. Used for sorting grade records by semester
// without needing any date library.
int semesterToKey(string semester) {
    int i = 3; // skip the literal "Sem"
    string semPart = "";
    while (i < (int)semester.length() && semester[i] != '-') {
        semPart += semester[i];
        i++;
    }
    i++; // skip '-'
    string yearPart = "";
    while (i < (int)semester.length()) {
        yearPart += semester[i];
        i++;
    }
    int semNum = stringToInt(semPart);
    int year = stringToInt(yearPart);
    return year * 10 + semNum;
}

// Converts a letter grade into a GPA value on a 4.0 scale.
// Returns -1.0 for non-graded entries (e.g. "Pending") so they can be
// excluded from GPA calculations and sorted to the bottom.
double gradeToGPA(string grade) {
    if (grade == "A+" || grade == "A")  return 4.00;
    if (grade == "A-")                  return 3.67;
    if (grade == "B+")                  return 3.33;
    if (grade == "B")                   return 3.00;
    if (grade == "B-")                  return 2.67;
    if (grade == "C+")                  return 2.33;
    if (grade == "C")                   return 2.00;
    if (grade == "C-")                  return 1.67;
    if (grade == "D+")                  return 1.33;
    if (grade == "D")                   return 1.00;
    if (grade == "D-")                  return 0.67;
    if (grade == "F")                   return 0.00;
    return -1.00; // "Pending" or unrecognised
}

// =====================================================================
// STRUCT DATA TYPES (2 required minimum - 3 provided)
// =====================================================================
struct ContactInfo {
    string phone;
    string email;
};

struct DateInfo {
    string semester; // raw text e.g. "Sem1-2024"
    int sortKey;      // numeric key e.g. 20241, built by semesterToKey()
};

struct Course {
    string code;
    string name;
    int credit;
    string year;
    string department;
};

// =====================================================================
// BASE CLASS 1: Person
// =====================================================================
class Person {
protected:
    string id;
    string name;
    string password;
    ContactInfo contact;
public:
    Person();
    Person(string id, string name, string password, string phone, string email);
    virtual ~Person();

    virtual bool login(string inputID, string inputPassword);
    virtual void displayProfile();

    string getID() const;
    string getName() const;
    string getPhone() const;
    string getEmail() const;
    string getPassword() const;

    void setName(string n);
    void setPhone(string p);
    void setEmail(string e);
    void setPassword(string p);

    friend bool validatePassword(Person &p, string inputPassword); // friend #1
};

Person::Person() {
    id = "";
    name = "";
    password = "";
}

Person::Person(string id, string name, string password, string phone, string email) {
    this->id = id;
    this->name = name;
    this->password = password;
    contact.phone = phone;
    contact.email = email;
}

Person::~Person() {
    // nothing dynamic at this level - derived classes handle their own cleanup
}

bool Person::login(string inputID, string inputPassword) {
    return (id == inputID && password == inputPassword);
}

void Person::displayProfile() {
    cout << "ID: " << id << ", Name: " << name << endl;
}

string Person::getID() const       { return id; }
string Person::getName() const     { return name; }
string Person::getPhone() const    { return contact.phone; }
string Person::getEmail() const    { return contact.email; }
string Person::getPassword() const { return password; }

void Person::setPhone(string p)    { contact.phone = p; }
void Person::setEmail(string e)    { contact.email = e; }
void Person::setPassword(string p) { password = p; }
void Person::setName(string n)     { name = n; }

bool validatePassword(Person &p, string inputPassword) {
    return p.password == inputPassword; // friend accessing protected member directly
}

// =====================================================================
// BASE CLASS 2: AcademicRecord
// =====================================================================
class AcademicRecord {
protected:
    string courseCode;
    string courseName;
    int credit;
public:
    AcademicRecord();
    AcademicRecord(string code, string name, int credit);
    virtual ~AcademicRecord();

    virtual void displayRecord();

    string getCourseCode() const;
    string getCourseName() const;
    int getCredit() const;

    friend void exportRecordToFile(AcademicRecord &rec, ofstream &outFile); // friend #2
};

AcademicRecord::AcademicRecord() {
    courseCode = "";
    courseName = "";
    credit = 0;
}

AcademicRecord::AcademicRecord(string code, string name, int credit) {
    courseCode = code;
    courseName = name;
    this->credit = credit;
}

AcademicRecord::~AcademicRecord() {
}

void AcademicRecord::displayRecord() {
    cout << courseCode << " - " << courseName << " (" << credit << " credit(s))" << endl;
}

string AcademicRecord::getCourseCode() const { return courseCode; }
string AcademicRecord::getCourseName() const { return courseName; }
int AcademicRecord::getCredit() const        { return credit; }

void exportRecordToFile(AcademicRecord &rec, ofstream &outFile) {
    outFile << rec.courseCode << "|" << rec.courseName << "|" << rec.credit;
}

// =====================================================================
// DERIVED CLASS 1: GradeEntry (inherits AcademicRecord)
// =====================================================================
class GradeEntry : public AcademicRecord {
private:
    string studentID;
    string grade;
    DateInfo term;
public:
    GradeEntry();
    GradeEntry(string sid, string code, string name, int credit, string grade, string semester);

    void displayRecord() override;

    string getStudentID() const;
    string getGrade() const;
    void setGrade(string g);
    DateInfo getTerm() const;
    void setSemester(string semester);

    friend void printGradeDetails(GradeEntry &g); // friend #3
};

GradeEntry::GradeEntry() : AcademicRecord() {
    studentID = "";
    grade = "";
    term.semester = "";
    term.sortKey = 0;
}

GradeEntry::GradeEntry(string sid, string code, string name, int credit, string grade, string semester)
    : AcademicRecord(code, name, credit) {
    studentID = sid;
    this->grade = grade;
    term.semester = semester;
    term.sortKey = semesterToKey(semester);
}

void GradeEntry::displayRecord() {
    cout << left
     << setw(10) << getCourseCode()
     << setw(30) << getCourseName()
     << setw(5)  << getCredit()
     << setw(12) << "credit(s)"
     << setw(8)  << grade
     << term.semester
     << endl;
}

string GradeEntry::getStudentID() const { return studentID; }
string GradeEntry::getGrade() const     { return grade; }
void GradeEntry::setGrade(string g)     { grade = g; }
DateInfo GradeEntry::getTerm() const    { return term; }

// Updates the semester text AND recomputes the numeric sortKey together,
// so a record edited via assignGradeScreen()/approveEnrollmentScreen()
// still sorts/GPA-groups correctly afterwards.
void GradeEntry::setSemester(string semester) {
    term.semester = semester;
    term.sortKey = semesterToKey(semester);
}

void printGradeDetails(GradeEntry &g) {
    cout << left
     << setw(12) << g.studentID
     << setw(12) << g.courseCode
     << setw(35) << g.courseName
     << setw(8)  << g.credit
     << setw(10) << g.grade
     << setw(15) << g.term.semester
     << endl;
}

// =====================================================================
// DYNAMIC NON-PRIMITIVE STRUCTURE: Singly Linked List of GradeEntry
// =====================================================================
struct GradeNode {
    GradeEntry data;
    GradeNode* next;
};

class GradeLinkedList {
private:
    GradeNode* head;
    int count;
public:
    GradeLinkedList();
    ~GradeLinkedList();

    void insert(GradeEntry e);
    bool removeByCourse(string code);
    bool updateGradeByCourse(string code, string newGrade);
    bool updateGradeAndSemesterByCourse(string code, string newGrade, string newSemester);
    void displayAll();
    bool isEmpty() const;
    int getCount() const;
    void toArray(GradeEntry arr[], int &outCount);

    friend int countGradeNodes(GradeLinkedList &list); // friend #4
};

GradeLinkedList::GradeLinkedList() {
    head = nullptr;
    count = 0;
}

GradeLinkedList::~GradeLinkedList() {
    GradeNode* current = head;
    while (current != nullptr) {
        GradeNode* temp = current;
        current = current->next;
        delete temp; // dynamic memory operation: delete
    }
    head = nullptr;
}

void GradeLinkedList::insert(GradeEntry e) {
    GradeNode* newNode = new GradeNode; // dynamic memory operation: new
    newNode->data = e;
    newNode->next = nullptr;

    if (head == nullptr) {
        head = newNode;
    } else {
        GradeNode* current = head;
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = newNode;
    }
    count++;
}

bool GradeLinkedList::removeByCourse(string code) {
    if (head == nullptr) return false;

    if (head->data.getCourseCode() == code) {
        GradeNode* temp = head;
        head = head->next;
        delete temp;
        count--;
        return true;
    }

    GradeNode* current = head;
    while (current->next != nullptr) {
        if (current->next->data.getCourseCode() == code) {
            GradeNode* temp = current->next;
            current->next = current->next->next;
            delete temp;
            count--;
            return true;
        }
        current = current->next;
    }
    return false;
}

bool GradeLinkedList::updateGradeByCourse(string code, string newGrade) {
    GradeNode* current = head;
    while (current != nullptr) {
        if (current->data.getCourseCode() == code) {
            current->data.setGrade(newGrade);
            return true;
        }
        current = current->next;
    }
    return false;
}

// Updates both the Grade AND the Semester of an existing record in one
// pass over the linked list (single pointer walk, no STL search helper).
// Used by assignGradeScreen()/approveEnrollmentScreen() so the admin can
// correct the semester at the same time as the grade.
bool GradeLinkedList::updateGradeAndSemesterByCourse(string code, string newGrade, string newSemester) {
    GradeNode* current = head;
    while (current != nullptr) {
        if (current->data.getCourseCode() == code) {
            current->data.setGrade(newGrade);
            current->data.setSemester(newSemester);
            return true;
        }
        current = current->next;
    }
    return false;
}

void GradeLinkedList::displayAll() {
    if (head == nullptr) {
        cout << "No grade records found." << endl;
        return;
    }
    GradeNode* current = head;
    while (current != nullptr) {
        printGradeDetails(current->data);
        current = current->next;
    }
}

bool GradeLinkedList::isEmpty() const { return head == nullptr; }
int GradeLinkedList::getCount() const { return count; }

void GradeLinkedList::toArray(GradeEntry arr[], int &outCount) {
    outCount = 0;
    GradeNode* current = head;
    while (current != nullptr && outCount < MAX_TEMP_GRADES) {
        arr[outCount] = current->data;
        outCount++;
        current = current->next;
    }
}

int countGradeNodes(GradeLinkedList &list) {
    return list.count;
}

// =====================================================================
// DERIVED CLASS 2: Student (inherits Person)
// =====================================================================
class Student : public Person {
private:
    string ic;
    string program;
    string intake;
    GradeLinkedList* gradeList;
public:
    Student();
    Student(string id, string name, string password, string ic, string phone,
            string email, string program, string intake);
    ~Student();

    bool login(string inputID, string inputPassword) override;
    void displayProfile() override;

    GradeLinkedList* getGradeList();
    string getProgram() const;
    string getIC() const;
    string getIntake() const;
    void setProgram(string p);
};

Student::Student() : Person() {
    ic = "";
    program = "";
    intake = "";
    gradeList = new GradeLinkedList(); // dynamic memory operation #1
}

Student::Student(string id, string name, string password, string ic, string phone,
                  string email, string program, string intake)
    : Person(id, name, password, phone, email) {
    this->ic = ic;
    this->program = program;
    this->intake = intake;
    gradeList = new GradeLinkedList(); // dynamic memory operation #1
}

Student::~Student() {
    delete gradeList; // dynamic memory operation #2
    gradeList = nullptr;
}

bool Student::login(string inputID, string inputPassword) {
    return (getID() == inputID) && validatePassword(*this, inputPassword);
}

void Student::displayProfile() {
    cout << "-----------------------------------" << endl;
    cout << "Student ID : " << getID() << endl;
    cout << "Name       : " << getName() << endl;
    cout << "IC Number  : " << ic << endl;
    cout << "Email      : " << getEmail() << endl;
    cout << "Phone      : " << getPhone() << endl;
    cout << "Program    : " << program << endl;
    cout << "Intake     : " << intake << endl;
    cout << "-----------------------------------" << endl;
}

GradeLinkedList* Student::getGradeList() { return gradeList; }
string Student::getProgram() const       { return program; }
string Student::getIC() const            { return ic; }
string Student::getIntake() const        { return intake; }
void Student::setProgram(string p)       { program = p; }

// =====================================================================
// DERIVED CLASS 3: Staff (inherits Person)
// =====================================================================
class Staff : public Person {
private:
    string role;
    string department;
    string position;
public:
    Staff();
    Staff(string id, string name, string password, string role, string department, string position);
    ~Staff();

    bool login(string inputID, string inputPassword) override;
    void displayProfile() override;

    string getRole() const;
    string getDepartment() const;
    string getPosition() const;

    friend void printStaffDetails(Staff &s); // additional friend function
};

Staff::Staff() : Person() {
    role = "";
    department = "";
    position = "";
}

Staff::Staff(string id, string name, string password, string role, string department, string position)
    : Person(id, name, password, "", "") {
    this->role = role;
    this->department = department;
    this->position = position;
}

Staff::~Staff() {
    // nothing dynamic owned by Staff - no cleanup needed
}

bool Staff::login(string inputID, string inputPassword) {
    return (getID() == inputID) && validatePassword(*this, inputPassword);
}

void Staff::displayProfile() {
    cout << "-----------------------------------" << endl;
    cout << "Staff ID   : " << getID() << endl;
    cout << "Name       : " << getName() << endl;
    cout << "Role       : " << role << endl;
    cout << "Department : " << department << endl;
    cout << "Position   : " << position << endl;
    cout << "-----------------------------------" << endl;
}

string Staff::getRole() const       { return role; }
string Staff::getDepartment() const { return department; }
string Staff::getPosition() const   { return position; }

void printStaffDetails(Staff &s) {
    cout << left
         << setw(10) << s.id
         << setw(20) << s.name
         << setw(12) << s.role
         << setw(18) << s.department
         << setw(18) << s.position
         << endl;
}

// =====================================================================
// GLOBAL DATA STORES (fixed-size arrays - no STL containers)
// =====================================================================
Student* studentArray[MAX_STUDENTS];
int studentCount = 0;

Course courseArray[MAX_COURSES];
int courseCount = 0;

Staff* staffArray[MAX_STAFF];
int staffCount = 0;

// Computes a student's cumulative GPA on a 4.0 scale from their grade
// list, excluding "Pending" entries. Shared by the sort, search, and
// report features so the calculation only lives in one place.
double calculateStudentGPA(Student* s) {
    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    s->getGradeList()->toArray(tempArr, n);

    double totalPoints = 0.0;
    int totalCredits = 0;

    for (int i = 0; i < n; i++) {
        double gpaPoint = gradeToGPA(tempArr[i].getGrade());
        if (gpaPoint >= 0.0) {
            totalPoints += gpaPoint * tempArr[i].getCredit();
            totalCredits += tempArr[i].getCredit();
        }
    }

    return (totalCredits > 0) ? (totalPoints / totalCredits) : 0.0;
}

// =====================================================================
// SORTING ALGORITHM: Quick Sort (manually implemented)
// Overloaded for GradeEntry[] (3 keys) and for Course[] (by code)
// =====================================================================
int partitionByGrade(GradeEntry arr[], int low, int high) {
    double pivot = gradeToGPA(arr[high].getGrade());
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (gradeToGPA(arr[j].getGrade()) >= pivot) { // descending: best grade first
            i++;
            GradeEntry temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
        }
    }
    GradeEntry temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
    return i + 1;
}

int partitionBySemester(GradeEntry arr[], int low, int high) {
    int pivot = arr[high].getTerm().sortKey;
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j].getTerm().sortKey <= pivot) { // ascending: oldest semester first
            i++;
            GradeEntry temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
        }
    }
    GradeEntry temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
    return i + 1;
}

int partitionByCode(GradeEntry arr[], int low, int high) {
    string pivot = arr[high].getCourseCode();
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j].getCourseCode() <= pivot) { // ascending alphabetical
            i++;
            GradeEntry temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
        }
    }
    GradeEntry temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
    return i + 1;
}

// sortKey accepts "GRADE", "SEMESTER", or "CODE"
void quickSort(GradeEntry arr[], int low, int high, string sortKey) {
    if (low < high) {
        int pi;
        if (sortKey == "GRADE") {
            pi = partitionByGrade(arr, low, high);
        } else if (sortKey == "SEMESTER") {
            pi = partitionBySemester(arr, low, high);
        } else {
            pi = partitionByCode(arr, low, high);
        }
        quickSort(arr, low, pi - 1, sortKey);
        quickSort(arr, pi + 1, high, sortKey);
    }
}

// Overload: sorts a Course[] array by course code (used before searching
// the course catalog with binary search).
void quickSort(Course arr[], int low, int high) {
    if (low < high) {
        string pivot = arr[high].code;
        int i = low - 1;
        for (int j = low; j < high; j++) {
            if (arr[j].code <= pivot) {
                i++;
                Course temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
            }
        }
        Course temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
        int pi = i + 1;
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

int partitionStudentsByGPA(Student* arr[], int low, int high) {
    double pivot = calculateStudentGPA(arr[high]);
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (calculateStudentGPA(arr[j]) >= pivot) { // descending: highest GPA first
            i++;
            Student* temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
        }
    }
    Student* temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
    return i + 1;
}

int partitionStudentsByID(Student* arr[], int low, int high) {
    string pivot = arr[high]->getID();
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j]->getID() <= pivot) { // ascending
            i++;
            Student* temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
        }
    }
    Student* temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
    return i + 1;
}

// Overload: sorts a Student*[] array. sortKey accepts "GPA" or "ID".
void quickSort(Student* arr[], int low, int high, string sortKey) {
    if (low < high) {
        int pi;
        if (sortKey == "GPA") {
            pi = partitionStudentsByGPA(arr, low, high);
        } else {
            pi = partitionStudentsByID(arr, low, high);
        }
        quickSort(arr, low, pi - 1, sortKey);
        quickSort(arr, pi + 1, high, sortKey);
    }
}

// =====================================================================
// SEARCHING ALGORITHM: Binary Search (manually implemented)
// Overloaded for GradeEntry[] and Course[]
// =====================================================================
int binarySearch(GradeEntry arr[], int size, string targetCode) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid].getCourseCode() == targetCode) return mid;
        else if (arr[mid].getCourseCode() < targetCode) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

int binarySearch(Course arr[], int size, string targetCode) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid].code == targetCode) return mid;
        else if (arr[mid].code < targetCode) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

int binarySearch(Student* arr[], int size, string targetID) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid]->getID() == targetID) return mid;
        else if (arr[mid]->getID() < targetID) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

// =====================================================================
// LOOKUP HELPERS (linear scans - utility only, NOT the graded search feature)
// =====================================================================
int findStudentIndexByID(string id) {
    for (int i = 0; i < studentCount; i++) {
        if (studentArray[i]->getID() == id) return i;
    }
    return -1;
}

// Scans the fixed-size native studentArray[] for a duplicate email,
// skipping the record whose ID equals excludeID (so a student editing
// their OWN profile, or an admin editing one record, isn't falsely
// flagged against itself). Pass excludeID = "" when checking a brand
// new registration that has no ID yet. Plain indexed for-loop only -
// no STL container or <algorithm> search routine is used.
bool isEmailUnique(string email, string excludeID) {
    for (int i = 0; i < studentCount; i++) {
        if (studentArray[i]->getID() == excludeID) continue;
        if (studentArray[i]->getEmail() == email) return false;
    }
    return true;
}

// Same pattern as isEmailUnique(), but checks IC number uniqueness.
bool isICUnique(string ic, string excludeID) {
    for (int i = 0; i < studentCount; i++) {
        if (studentArray[i]->getID() == excludeID) continue;
        if (studentArray[i]->getIC() == ic) return false;
    }
    return true;
}

int findCourseIndexByCode(string code) {
    for (int i = 0; i < courseCount; i++) {
        if (courseArray[i].code == code) return i;
    }
    return -1;
}

int findStaffIndexByID(string id) {
    for (int i = 0; i < staffCount; i++) {
        if (staffArray[i]->getID() == id) return i;
    }
    return -1;
}

string generateNextStudentID() {
    int maxNum = 0;
    for (int i = 0; i < studentCount; i++) {
        string idPart = studentArray[i]->getID().substr(1); // strip leading 'S'
        int num = stringToInt(idPart);
        if (num > maxNum) maxNum = num;
    }
    return "S" + intToPaddedString(maxNum + 1, 3);
}

// =====================================================================
// FILE I/O - LOADING
// =====================================================================
void loadCoursesFromFile() {
    ifstream inFile("courses.txt");
    if (!inFile) {
        cout << "Warning: courses.txt not found. Starting with no courses." << endl;
        return;
    }
    courseCount = 0;
    string line;
    while (getline(inFile, line) && courseCount < MAX_COURSES) {
        if (line.length() == 0) continue;
        string fields[5];
        splitLine(line, fields, 5, '|');
        courseArray[courseCount].code = fields[0];
        courseArray[courseCount].name = fields[1];
        courseArray[courseCount].credit = stringToInt(fields[2]);
        courseArray[courseCount].year = fields[3];
        courseArray[courseCount].department = fields[4];
        courseCount++;
    }
    inFile.close();
}

void loadStudentsFromFile() {
    ifstream inFile("students.txt");
    if (!inFile) {
        cout << "Warning: students.txt not found. Starting with no students." << endl;
        return;
    }
    studentCount = 0;
    string line;
    while (getline(inFile, line) && studentCount < MAX_STUDENTS) {
        if (line.length() == 0) continue;
        string fields[8];
        splitLine(line, fields, 8, '|');
        // fields: 0 ID,1 Name,2 IC,3 Email,4 Password,5 Phone,6 Program,7 Intake
        studentArray[studentCount] = new Student(fields[0], fields[1], fields[4],
                                                   fields[2], fields[5], fields[3],
                                                   fields[6], fields[7]);
        studentCount++;
    }
    inFile.close();
}

void loadGradesFromFile() {
    ifstream inFile("grades.txt");
    if (!inFile) {
        cout << "Warning: grades.txt not found. Starting with no grade records." << endl;
        return;
    }
    string line;
    while (getline(inFile, line)) {
        if (line.length() == 0) continue;
        string fields[6];
        splitLine(line, fields, 6, '|');
        // fields: 0 StudentID,1 CourseCode,2 CourseName,3 Credit,4 Grade,5 Semester
        int idx = findStudentIndexByID(fields[0]);
        if (idx != -1) {
            GradeEntry entry(fields[0], fields[1], fields[2], stringToInt(fields[3]),
                              fields[4], fields[5]);
            studentArray[idx]->getGradeList()->insert(entry);
        }
    }
    inFile.close();
}

void loadStaffFromFile() {
    ifstream inFile("staff.txt");
    if (!inFile) {
        cout << "Warning: staff.txt not found. Starting with no staff accounts." << endl;
        return;
    }
    staffCount = 0;
    string line;
    while (getline(inFile, line) && staffCount < MAX_STAFF) {
        if (line.length() == 0) continue;
        string fields[6];
        splitLine(line, fields, 6, '|');
        // fields: 0 StaffID,1 Name,2 Role,3 Password,4 Department,5 Position
        staffArray[staffCount] = new Staff(fields[0], fields[1], fields[3], fields[2],
                                            fields[4], fields[5]);
        staffCount++;
    }
    inFile.close();
}

// =====================================================================
// FILE I/O - SAVING
// =====================================================================
void appendStudentToFile(Student* s) {
    // The existing students.txt may not end with a newline - check first,
    // otherwise the new record would be glued onto the end of the last line.
    bool needsLeadingNewline = false;
    ifstream checkFile("students.txt");
    if (checkFile) {
        checkFile.seekg(0, ios::end);
        long fileSize = (long)checkFile.tellg();
        if (fileSize > 0) {
            checkFile.seekg(-1, ios::end);
            char lastChar = '\0';
            checkFile.get(lastChar);
            if (lastChar != '\n') {
                needsLeadingNewline = true;
            }
        }
    }
    checkFile.close();

    ofstream outFile("students.txt", ios::app);
    if (!outFile) {
        cout << "Error: could not open students.txt for writing." << endl;
        return;
    }
    if (needsLeadingNewline) {
        outFile << endl;
    }
    outFile << s->getID() << "|" << s->getName() << "|" << s->getIC() << "|"
            << s->getEmail() << "|" << s->getPassword() << "|" << s->getPhone() << "|"
            << s->getProgram() << "|" << s->getIntake() << endl;
    outFile.close();
}

void rewriteStudentsFile() {
    ofstream outFile("students.txt"); // default mode truncates/overwrites
    if (!outFile) {
        cout << "Error: could not open students.txt for writing." << endl;
        return;
    }
    for (int i = 0; i < studentCount; i++) {
        outFile << studentArray[i]->getID() << "|" << studentArray[i]->getName() << "|"
                << studentArray[i]->getIC() << "|" << studentArray[i]->getEmail() << "|"
                << studentArray[i]->getPassword() << "|" << studentArray[i]->getPhone() << "|"
                << studentArray[i]->getProgram() << "|" << studentArray[i]->getIntake() << endl;
    }
    outFile.close();
}

void rewriteGradesFile() {
    ofstream outFile("grades.txt");
    if (!outFile) {
        cout << "Error: could not open grades.txt for writing." << endl;
        return;
    }
    for (int i = 0; i < studentCount; i++) {
        GradeEntry tempArr[MAX_TEMP_GRADES];
        int n = 0;
        studentArray[i]->getGradeList()->toArray(tempArr, n);
        for (int j = 0; j < n; j++) {
            outFile << tempArr[j].getStudentID() << "|" << tempArr[j].getCourseCode() << "|"
                    << tempArr[j].getCourseName() << "|" << tempArr[j].getCredit() << "|"
                    << tempArr[j].getGrade() << "|" << tempArr[j].getTerm().semester << endl;
        }
    }
    outFile.close();
}

void appendCourseToFile(Course c) {
    // Same trailing-newline safety check used for students.txt - courses.txt
    // may not end with a newline either.
    bool needsLeadingNewline = false;
    ifstream checkFile("courses.txt");
    if (checkFile) {
        checkFile.seekg(0, ios::end);
        long fileSize = (long)checkFile.tellg();
        if (fileSize > 0) {
            checkFile.seekg(-1, ios::end);
            char lastChar = '\0';
            checkFile.get(lastChar);
            if (lastChar != '\n') {
                needsLeadingNewline = true;
            }
        }
    }
    checkFile.close();

    ofstream outFile("courses.txt", ios::app);
    if (!outFile) {
        cout << "Error: could not open courses.txt for writing." << endl;
        return;
    }
    if (needsLeadingNewline) {
        outFile << endl;
    }
    outFile << c.code << "|" << c.name << "|" << c.credit << "|" << c.year << "|" << c.department << endl;
    outFile.close();
}

void rewriteCoursesFile() {
    ofstream outFile("courses.txt");
    if (!outFile) {
        cout << "Error: could not open courses.txt for writing." << endl;
        return;
    }
    for (int i = 0; i < courseCount; i++) {
        outFile << courseArray[i].code << "|" << courseArray[i].name << "|"
                << courseArray[i].credit << "|" << courseArray[i].year << "|"
                << courseArray[i].department << endl;
    }
    outFile.close();
}

void generateStudentReport(Student* s) {
    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    s->getGradeList()->toArray(tempArr, n);

    double totalPoints = 0.0;
    int totalCredits = 0;

    ofstream outFile("student_report.txt"); // overwrites every time
    if (!outFile) {
        cout << "Error: could not open student_report.txt for writing." << endl;
        return;
    }

    outFile << "============================================================" << endl;
    outFile << "              STUDENT ACADEMIC SUMMARY REPORT" << endl;
    outFile << "============================================================" << endl;
    outFile << "Student ID : " << s->getID() << endl;
    outFile << "Name       : " << s->getName() << endl;
    outFile << "Program    : " << s->getProgram() << endl;
    outFile << "============================================================" << endl;

    outFile << left
            << setw(10) << "Code"
            << setw(25) << "Course Name"
            << setw(8)  << "Credit"
            << setw(10) << "Grade"
            << setw(15) << "Semester" << endl;

    outFile << "============================================================" << endl;

    for (int i = 0; i < n; i++) 
    {
        outFile << left
                << setw(10) << tempArr[i].getCourseCode()
                << setw(25) << tempArr[i].getCourseName()
                << setw(8)  << tempArr[i].getCredit()
                << setw(10) << tempArr[i].getGrade()
                << setw(15) << tempArr[i].getTerm().semester
                << endl;

        double gpaPoint = gradeToGPA(tempArr[i].getGrade());

        if (gpaPoint >= 0.0) 
        {
            totalPoints += gpaPoint * tempArr[i].getCredit();
            totalCredits += tempArr[i].getCredit();
        }
    }

    double gpa = (totalCredits > 0) ? (totalPoints / totalCredits) : 0.0;

    outFile << "============================================================" << endl;
    outFile << "Total Credits Completed : " << totalCredits << endl;
    outFile << "Cumulative GPA          : " << fixed << setprecision(2) << gpa << endl;
    outFile << "============================================================" << endl;
    outFile.close();
}

void cleanupStudents() {
    for (int i = 0; i < studentCount; i++) {
        delete studentArray[i];
        studentArray[i] = nullptr;
    }
}

void cleanupStaff() {
    for (int i = 0; i < staffCount; i++) {
        delete staffArray[i];
        staffArray[i] = nullptr;
    }
}

// =====================================================================
// FUNCTION PROTOTYPES - MAIN MENU LEVEL
// =====================================================================
void showMainMenu();
void studentRegistrationScreen();
void studentLoginScreen();
void staffLoginScreen();

// =====================================================================
// FUNCTION PROTOTYPES - STUDENT MODULE SCREENS
// =====================================================================
void studentMenuScreen(string studentID, string studentName);
void viewProfileScreen(string studentID);
void updateProfileScreen(string studentID);
void viewGradesScreen(string studentID);
void searchGradeScreen(string studentID);
void sortGradesScreen(string studentID);
void addEnrollmentScreen(string studentID);
void editPendingEnrollmentScreen(string studentID);
void dropCourseScreen(string studentID);
void saveStudentReportScreen(string studentID);
void viewStudentReportScreen(string studentID);

// =====================================================================
// FUNCTION PROTOTYPES - STAFF MODULE SCREENS (unified menu, placeholders)
// =====================================================================
void staffMenuScreen(string staffID, string staffName);
void manageStudentsScreen();
void manageCoursesScreen();
void manageGradesScreen();
void sortStudentsScreen();
void searchStudentScreen();
void generateAdminReportScreen();
void viewAdminReportScreen();
void viewAllStudentsScreen();
void addStudentScreen();
void editStudentScreen();
void deleteStudentScreen();
void viewAllCoursesScreen();
void addCourseScreen();
void editCourseScreen();
void deleteCourseScreen();
void viewGradesForStudentScreen();
void assignGradeScreen();
void approveEnrollmentScreen();

// =====================================================================
// MAIN
// =====================================================================
int main() {
    loadCoursesFromFile();
    loadStudentsFromFile();
    loadGradesFromFile();
    loadStaffFromFile();

    showMainMenu();

    cleanupStudents();
    cleanupStaff();
    return 0;
}

// =====================================================================
// MAIN MENU (SCREEN 0)
// =====================================================================
void showMainMenu() {
    int choice;
    bool running = true;

    while (running) {
        clearScreen();
        cout << "====================================" << endl;
        cout << "   STUDENT ACADEMIC RECORDS SYSTEM" << endl;
        cout << "====================================" << endl;
        cout << "1. Student Login" << endl;
        cout << "2. Student Registration" << endl;
        cout << "3. Staff/Admin Login" << endl;
        cout << "4. Exit" << endl;
        cout << "====================================" << endl;
        cout << "Enter your choice: ";
        choice = readIntInput();

        switch (choice) {
            case 1: studentLoginScreen();        break;
            case 2: studentRegistrationScreen(); break;
            case 3: staffLoginScreen();          break;
            case 4:
                clearScreen();
                cout << "Thank you for using the system. Goodbye!" << endl;
                running = false;
                break;
            default:
                cout << "Invalid choice." << endl;
                pauseScreen();
        }
    }
}

// Helper 1: Manually validate email format (No STL)
bool isValidEmailFormat(string email) {
    int atIndex = -1;
    int dotIndex = -1;
    int atCount = 0;

    for (int i = 0; i < email.length(); i++) {
        if (email[i] == '@') {
            atIndex = i;
            atCount++;
        } else if (email[i] == '.' && atIndex != -1) {
            // Record the position of the last '.' that appears after the '@'
            dotIndex = i; 
        }
    }

    // Format rules:
    // 1. Must contain exactly one '@'
    if (atCount != 1) return false;
    // 2. '@' cannot be the first character
    if (atIndex == 0) return false;
    // 3. Must have a '.' somewhere after the '@'
    if (dotIndex == -1) return false;
    // 4. '.' cannot immediately follow the '@' (e.g., user@.com is invalid)
    if (dotIndex == atIndex + 1) return false;
    // 5. '.' cannot be the last character
    if (dotIndex == email.length() - 1) return false;

    return true;
}

// Helper 2: Check if the email is already registered in the system
bool isEmailDuplicate(string email) {
    // Iterate through the currently loaded studentArray in memory
    for (int i = 0; i < studentCount; i++) {
        // Note: Ensure getEmail() matches the actual getter method in your Person/Student class
        if (studentArray[i]->getEmail() == email) {
            return true; // Duplicate found
        }
    }
    return false;
}

// =====================================================================
// SCREEN 1 - STUDENT REGISTRATION (real logic, with IC/phone validation)
// =====================================================================
void studentRegistrationScreen() {
    clearScreen();
    cout << "========= STUDENT REGISTRATION =========" << endl;

    try {
        string name, ic, email, password, phone, program;

        cout << "Enter Name (0 to cancel): ";
        cin.ignore();
        getline(cin, name);

        if (name == "0") {
            return;
        }   

        int icAttempts = 0;
        do {
            cout << "Enter IC Number (must be exactly 12 digits): ";
            cin >> ic;
            icAttempts++;
            if (!isValidIC(ic)) {
                cout << "Invalid IC number. It must contain exactly 12 digits." << endl;
            } else if (!isICUnique(ic, "")) {
                cout << "This IC number is already registered to another student." << endl;
            }
            if ((!isValidIC(ic) || !isICUnique(ic, "")) && icAttempts >= 5) {
                throw runtime_error("Too many invalid IC attempts. Registration cancelled.");
            }
        } while (!isValidIC(ic) || !isICUnique(ic, ""));

        // ==========================================
        // [START of new Email validation logic]
        // ==========================================
        int emailAttempts = 0;
        bool validEmailFound = false;
        do {
            cout << "Enter Email: ";
            cin >> email;
            emailAttempts++;

            if (!isValidEmailFormat(email)) {
                cout << "Invalid email format. Must contain '@' and a domain (e.g., user@email.com)." << endl;
                if (emailAttempts >= 5) {
                    throw runtime_error("Too many invalid email format attempts. Registration cancelled.");
                }
            } else if (isEmailDuplicate(email)) {
                cout << "Email is already registered! Please use a different email." << endl;
                if (emailAttempts >= 5) {
                    throw runtime_error("Too many duplicate email attempts. Registration cancelled.");
                }
            } else {
                validEmailFound = true; // Format is valid and no duplicates found
            }
        } while (!validEmailFound);
        // ==========================================
        // [END of new Email validation logic]
        // ==========================================

        cout << "Enter Password: ";
        cin >> password;

        int phoneAttempts = 0;
        do {
            cout << "Enter Phone Number (numeric, below 12 digits): ";
            cin >> phone;
            phoneAttempts++;
            if (!isValidPhone(phone)) {
                cout << "Invalid phone number. Must be numeric and below 12 digits." << endl;
                if (phoneAttempts >= 5) {
                    throw runtime_error("Too many invalid phone attempts. Registration cancelled.");
                }
            }
        } while (!isValidPhone(phone));

        cout << "Enter Program (CS/IT/SE): ";
        cin >> program;

        string newID = generateNextStudentID();
        string intake = "September2026"; // TODO: prompt for this in a later phase if needed

        studentArray[studentCount] = new Student(newID, name, password, ic, phone, email, program, intake);
        studentCount++;
        appendStudentToFile(studentArray[studentCount - 1]);

        cout << "=========================================" << endl;
        cout << "Registration successful! Your Student ID is: " << newID << endl;
        cout << "=========================================" << endl;
    }
    catch (runtime_error &e) {
        cout << "=========================================" << endl;
        cout << "Registration failed: " << e.what() << endl;
        cout << "=========================================" << endl;
    }

    pauseScreen();
}            
// =====================================================================
// SCREEN 2 - STUDENT LOGIN (real logic)
// =====================================================================
void studentLoginScreen() {
    clearScreen();
    cout << "============ STUDENT LOGIN =============" << endl;

    string id, password;
    cout << "Enter Student ID (0 to cancel): ";
    cin >> id;
    if (id == "0") {
        return;
    }       
    cout << "Enter Password: ";
    cin >> password;

    try {
        int idx = findStudentIndexByID(id);
        if (idx == -1) {
            throw runtime_error("Student ID not found.");
        }
        if (!studentArray[idx]->login(id, password)) {
            throw runtime_error("Incorrect password.");
        }

        cout << "Login successful!" << endl;
        pauseScreen();
        studentMenuScreen(studentArray[idx]->getID(), studentArray[idx]->getName());
    }
    catch (runtime_error &e) {
        cout << "Login failed: " << e.what() << endl;
        pauseScreen();
    }
}

// =====================================================================
// STUDENT MAIN MENU
// =====================================================================
void studentMenuScreen(string studentID, string studentName) {
    int choice;
    bool inStudentMenu = true;

    while (inStudentMenu) {
        clearScreen();
        cout << "============ STUDENT MENU =============" << endl;
        cout << "Welcome, " << studentName << " (ID: " << studentID << ")" << endl;
        cout << "=========================================" << endl;
        cout << "1. View My Profile" << endl;
        cout << "2. Update My Profile" << endl;
        cout << "3. View My Academic Records (Grades)" << endl;
        cout << "4. Search My Grades by Course" << endl;
        cout << "5. Sort My Grades (by Grade / by Semester)" << endl;
        cout << "6. Add Course Enrollment Request" << endl;
        cout << "7. Edit Pending Enrollment" << endl;
        cout << "8. Drop a Course" << endl;
        cout << "9. Save My Academic Summary Report" << endl;
        cout << "10. View My Saved Report" << endl;
        cout << "11. Logout" << endl;
        cout << "=========================================" << endl;
        cout << "Enter your choice: ";
        choice = readIntInput();

        switch (choice) {
            case 1:  viewProfileScreen(studentID);          break;
            case 2:  updateProfileScreen(studentID);        break;
            case 3:  viewGradesScreen(studentID);           break;
            case 4:  searchGradeScreen(studentID);          break;
            case 5:  sortGradesScreen(studentID);           break;
            case 6:  addEnrollmentScreen(studentID);        break;
            case 7:  editPendingEnrollmentScreen(studentID);break;
            case 8:  dropCourseScreen(studentID);           break;
            case 9:  saveStudentReportScreen(studentID);    break;
            case 10: viewStudentReportScreen(studentID);    break;
            case 11: inStudentMenu = false;                 break;
            default:
                cout << "Invalid choice." << endl;
                pauseScreen();
        }
    }
}

// =====================================================================
// STUDENT SUBMENU SCREENS (full logic)
// =====================================================================

void viewProfileScreen(string studentID) {
    clearScreen();
    cout << "============ MY PROFILE ============" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
    } else {
        studentArray[idx]->displayProfile();
    }
    
    pauseScreen();        
}

void updateProfileScreen(string studentID) {
    clearScreen();
    cout << "========== UPDATE MY PROFILE ==========" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
        pauseScreen();
        return;
    }

    cout << "1. Update Phone Number" << endl;
    cout << "2. Update Email" << endl;
    cout << "3. Update Password" << endl;
    cout << "0. Back" << endl;
    cout << "Enter your choice: ";
    int choice;
    choice = readIntInput();

    try {
        if (choice == 1) {
            string newPhone;
            int attempts = 0;
            bool cancelled = false;
            do {cout << "Note: Phone number must be numeric and below 12 digits." << endl;
                cout << "Enter new Phone Number (0 to cancel): ";
                cin >> newPhone;
                if (newPhone == "0") { cancelled = true; break; }
                attempts++;
                if (!isValidPhone(newPhone)) {
                    cout << "Invalid phone number." << endl;
                    if (attempts >= 5) {
                        throw runtime_error("Too many invalid attempts. Update cancelled.");
                    }
                }
            } while (!isValidPhone(newPhone));

            if (cancelled) {
                
                return;
            }
            studentArray[idx]->setPhone(newPhone);
            cout << "Phone number updated." << endl;
        }
        else if (choice == 2) {
            string newEmail;
            bool cancelled = false;
            int emailAttempts = 0;
            do {
                cout << "Enter new Email (or 0 to cancel): ";
                cin >> newEmail;
                if (newEmail == "0") { cancelled = true; break; }

                emailAttempts++;
                if (!isValidEmail(newEmail)) {
                    cout << "Invalid email format." << endl;
                } else if (!isEmailUnique(newEmail, studentID)) {
                    cout << "This email is already used by another student." << endl;
                }
                if ((!isValidEmail(newEmail) || !isEmailUnique(newEmail, studentID)) && emailAttempts >= 5) {
                    throw runtime_error("Too many invalid email attempts. Update cancelled.");
                }
            } while (!isValidEmail(newEmail) || !isEmailUnique(newEmail, studentID));

            if (cancelled) {
                cout << "Update cancelled." << endl;
                pauseScreen();
                return;
            }
            studentArray[idx]->setEmail(newEmail);
            cout << "Email updated." << endl;
        }
        else if (choice == 3) {
            // Require the new password to be entered twice so the student
            // doesn't accidentally lock themselves out with a typo.
            string newPassword, confirmPassword;
            int mismatchAttempts = 0;
            bool cancelled = false;

            while (true) {
                cout << "Enter new Password (or 0 to cancel): ";
                cin >> newPassword;
                if (newPassword == "0") { cancelled = true; break; }

                cout << "Confirm new Password (or 0 to cancel): ";
                cin >> confirmPassword;
                if (confirmPassword == "0") { cancelled = true; break; }

                if (newPassword == confirmPassword) {
                    break; // confirmed match, proceed to save
                }

                mismatchAttempts++;
                cout << "Passwords do not match. Please try again." << endl;
                if (mismatchAttempts >= 5) {
                    throw runtime_error("Too many mismatched attempts. Update cancelled.");
                }
            }

            if (cancelled) {
        
                return;
            }
            studentArray[idx]->setPassword(newPassword);
            cout << "Password updated." << endl;
        }
        else if (choice == 0) {
            return;
        }
        else {
            cout << "Invalid choice." << endl;
            pauseScreen();
            return;
        }

        rewriteStudentsFile();
    }
    catch (runtime_error &e) {
        cout << "Update failed: " << e.what() << endl;
    }

    pauseScreen();
}

void viewGradesScreen(string studentID) {
    clearScreen();
    cout << "================================ MY ACADEMIC RECORDS ================================" << endl;
    cout << left
         << setw(12) << "Student ID"   
         << setw(12) << "Code"
         << setw(35) << "Course Name"
         << setw(8)  << "Credit"
         << setw(10) << "Grade"
         << setw(15) << "Semester" << endl;
     cout << "-------------------------------------------------------------------------------------" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
    } else {
        studentArray[idx]->getGradeList()->displayAll();
    }

    pauseScreen();
    
}

void searchGradeScreen(string studentID) {
    clearScreen();
    cout << "========= SEARCH MY GRADES =========" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
        pauseScreen();
        return;
    }

    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    studentArray[idx]->getGradeList()->toArray(tempArr, n);

    if (n == 0) {
        cout << "No grade records to search." << endl;
        pauseScreen();
        return;
    }

    quickSort(tempArr, 0, n - 1, "CODE"); // binary search requires sorted data

    cout << "Enter Course Code to search (0 to cancel): ";
    string code;
    cin >> code;

    if (code == "0") {
        return;
    }

    int result = binarySearch(tempArr, n, code);

    if (result != -1) {
        cout << "\nRecord Found:" << endl;
            cout<< left
            << setw(10) << "Code"
            << setw(30) << "Course Name"
            << setw(17)  << "Credit"
            << setw(8) << "Grade"
            << setw(15) << "Semester" << endl;
            cout<< "--------------------------------------------------------------------------" << endl;
        tempArr[result].displayRecord();
    } else {
        cout << "\nCourse not found in your records." << endl;
    }

    pauseScreen();
}

void sortGradesScreen(string studentID) {
    clearScreen();
    cout << "========== SORT MY GRADES ==========" << endl;
    cout << "1. Sort by Grade (Highest to Lowest)" << endl;
    cout << "2. Sort by Semester (Oldest to Newest)" << endl;
    cout << "3. Back" << endl;
    cout << "Enter your choice: ";
    int choice;
    choice = readIntInput();

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
        pauseScreen();
        return;
    }

    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    studentArray[idx]->getGradeList()->toArray(tempArr, n);

    if (n == 0) {
        cout << "No grade records to sort." << endl;
        pauseScreen();
        return;
    }

    if (choice == 1) {
        quickSort(tempArr, 0, n - 1, "GRADE");
    } else if (choice == 2) {
        quickSort(tempArr, 0, n - 1, "SEMESTER");
    } else if (choice == 3) {
        return;
    } else {
        cout << "Invalid choice." << endl;
        pauseScreen();
        return;
    }

    cout << "\n----------------------------- Sorted Results -----------------------------" << endl;
    cout<< left
        << setw(10) << "Code"
        << setw(30) << "Course Name"
        << setw(17)  << "Credit"
        << setw(8) << "Grade"
        << setw(4) << "Semester" << endl;
        cout << "--------------------------------------------------------------------------" << endl;
    for (int i = 0; i < n; i++) {
        tempArr[i].displayRecord();
    }

    pauseScreen();
}

void addEnrollmentScreen(string studentID) {
    clearScreen();
    cout << "===== ADD COURSE ENROLLMENT REQUEST =====" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
        pauseScreen();
        return;
    }

    if (courseCount == 0) {
        cout << "No courses available in the catalog." << endl;
        pauseScreen();
        return;
    }

    cout << "Available Courses:" << endl;
    cout<< left
        << setw(10) << "Code"
        << setw(30) << "Course Name"
       << "Credit" << endl;
    cout<< "--------------------------------------------------------------------------" << endl;
    for (int i = 0; i < courseCount; i++) {
        cout <<left<< setw(10) << courseArray[i].code << setw(30) << courseArray[i].name
         << courseArray[i].credit << " credit(s)" << endl;
    }

    cout << "\nEnter Course Code to enroll (0 to cancel): ";
    string code;
    cin >> code;

    if (code == "0") {
        return;
    }

    // Search the course catalog using binary search (2nd search feature)
    Course sortedCourses[MAX_COURSES];
    for (int i = 0; i < courseCount; i++) sortedCourses[i] = courseArray[i];
    quickSort(sortedCourses, 0, courseCount - 1);

    int courseIdx = binarySearch(sortedCourses, courseCount, code);
    if (courseIdx == -1) {
        cout << "Course not found in catalog." << endl;
        pauseScreen();
        return;
    }

    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    studentArray[idx]->getGradeList()->toArray(tempArr, n);
    for (int i = 0; i < n; i++) {
        if (tempArr[i].getCourseCode() == code) {
            cout << "You are already enrolled in (or pending for) this course." << endl;
            pauseScreen();
            return;
        }
    }

    string semester = promptSemesterInput("Enter the Semester you are enrolling for");
    if (semester == "0") {
        cout << "Enrollment cancelled." << endl;
        pauseScreen();
        return;
    }

    GradeEntry newEntry(studentID, sortedCourses[courseIdx].code, sortedCourses[courseIdx].name,
                         sortedCourses[courseIdx].credit, "Pending", semester);
    studentArray[idx]->getGradeList()->insert(newEntry);
    rewriteGradesFile();

    cout << "Enrollment request submitted (status: Pending Staff Approval)." << endl;
    pauseScreen();
}

void editPendingEnrollmentScreen(string studentID) {
    clearScreen();
    cout << "===== EDIT PENDING ENROLLMENT =====" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
        pauseScreen();
        return;
    }

    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    studentArray[idx]->getGradeList()->toArray(tempArr, n);

    bool hasPending = false;
    cout << "Your Pending Enrollments:" << endl;
    cout<< left
        << setw(10) << "Code"
        << setw(30) << "Course Name" << endl;
    cout << "--------------------------" << endl;
    for (int i = 0; i < n; i++) {
        if (tempArr[i].getGrade() == "Pending") {
            cout <<left<< setw(10) << tempArr[i].getCourseCode() << setw(30) << tempArr[i].getCourseName() << endl;
            hasPending = true;
        }
    }

    if (!hasPending) {
        cout << "You have no pending enrollments to edit." << endl;
        pauseScreen();
        return;
    }

    cout << "\nEnter the Course Code of the pending enrollment to change (0 to cancel): ";
    string oldCode;
    cin >> oldCode;

    if (oldCode == "0") {
        return;
    }

    cout << "Enter the new Course Code you want instead (0 to cancel): ";
    string newCode;
    cin >> newCode;

    if (newCode == "0") {
        return;
    }

    int courseIdx = findCourseIndexByCode(newCode);
    if (courseIdx == -1) {
        cout << "New course code not found in catalog." << endl;
        pauseScreen();
        return;
    }

    // Ask for the semester BEFORE touching the linked list, so if the
    // student cancels here the original pending enrollment is left
    // completely untouched (no remove/restore juggling needed).
    string semester = promptSemesterInput("Enter the Semester for this enrollment");
    if (semester == "0") {
        cout << "Enrollment edit cancelled." << endl;
        pauseScreen();
        return;
    }

    bool removed = studentArray[idx]->getGradeList()->removeByCourse(oldCode);
    if (!removed) {
        cout << "Original pending enrollment not found (was it already approved?)." << endl;
        pauseScreen();
        return;
    }

    GradeEntry replacement(studentID, courseArray[courseIdx].code, courseArray[courseIdx].name,courseArray[courseIdx].credit, "Pending", semester);
    studentArray[idx]->getGradeList()->insert(replacement);
    rewriteGradesFile();

    cout << "Pending enrollment updated successfully." << endl;
    pauseScreen();
}

void dropCourseScreen(string studentID) {
    clearScreen();
    cout << "============ DROP A COURSE ============" << endl;
    cout<< "Note: You can only drop courses that are currently in your records." << endl;
    cout<<left<< setw(12) << "Student ID"<< setw(12) << "Code"<< setw(35) << "Course Name"<< setw(8)  << "Credit"<< setw(10) << "Grade"<< setw(15) << "Semester" << endl;
    cout << "-------------------------------------------------------------------------------------" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
        pauseScreen();
        return;
    }

    studentArray[idx]->getGradeList()->displayAll();

    cout << "\nEnter Course Code to drop (0 to cancel): ";
    string code;
    cin >> code;

    if (code == "0") {
        return;
    }

    bool removed = studentArray[idx]->getGradeList()->removeByCourse(code);
    if (removed) {
        rewriteGradesFile();
        cout << "Course dropped successfully." << endl;
    } else {
        cout << "Course code not found in your records." << endl;
    }
    pauseScreen();
}

void saveStudentReportScreen(string studentID) {
    clearScreen();
    cout << "===== SAVE MY ACADEMIC SUMMARY REPORT =====" << endl;

    int idx = findStudentIndexByID(studentID);
    if (idx == -1) {
        cout << "Error: student record not found." << endl;
        pauseScreen();
        return;
    }

    generateStudentReport(studentArray[idx]);
    cout << "Report saved to student_report.txt" << endl;

    pauseScreen();    
    
}

void viewStudentReportScreen(string studentID) {
    clearScreen();
    cout << "======= VIEW MY SAVED REPORT =======" << endl;

    ifstream inFile("student_report.txt");
    if (!inFile) {
        cout << "No saved report found. Please save a report first." << endl;
        pauseScreen();
        return;
    }

    string line;
    while (getline(inFile, line)) {
        cout << line << endl;
    }
    inFile.close();

    pauseScreen();     
}

// =====================================================================
// SCREEN 3 - STAFF LOGIN (placeholder - next build phase)
// =====================================================================
void staffLoginScreen() {
    clearScreen();
    cout << "============ STAFF/ADMIN LOGIN ============" << endl;

    string id, password;
    cout << "Enter Staff ID (or 0 to cancel): ";
    cin >> id;

    if (id == "0") {
        return;
    }

    cout << "Enter Password: ";
    cin >> password;

    try {
        int idx = findStaffIndexByID(id);
        if (idx == -1) {
            throw runtime_error("Staff ID not found.");
        }
        if (!staffArray[idx]->login(id, password)) {
            throw runtime_error("Incorrect password.");
        }

        cout << "Login successful!" << endl;
        pauseScreen();
        staffMenuScreen(staffArray[idx]->getID(), staffArray[idx]->getName());
    }
    catch (runtime_error &e) {
        cout << "Login failed: " << e.what() << endl;
        pauseScreen();
    }
}

// =====================================================================
// STAFF MAIN MENU (unified - placeholders, next build phase)
// =====================================================================
void staffMenuScreen(string staffID, string staffName) {
    int choice;
    bool inStaffMenu = true;

    while (inStaffMenu) {
        clearScreen();
        cout << "============ STAFF/ADMIN MENU ============" << endl;
        cout << "Welcome, " << staffName << " (ID: " << staffID << ")" << endl;
        cout << "===========================================" << endl;
        cout << "1. Manage Student Records" << endl;
        cout << "2. Manage Course Records" << endl;
        cout << "3. Manage Grade Records" << endl;
        cout << "4. Sort Students (by GPA / by ID)" << endl;
        cout << "5. Search Student Record" << endl;
        cout << "6. Generate Department Report" << endl;
        cout << "7. View Saved Report" << endl;
        cout << "0. Logout" << endl;
        cout << "===========================================" << endl;
        cout << "Enter your choice: ";
        choice = readIntInput();

        switch (choice) {
            case 1: manageStudentsScreen();      break;
            case 2: manageCoursesScreen();       break;
            case 3: manageGradesScreen();        break;
            case 4: sortStudentsScreen();        break;
            case 5: searchStudentScreen();       break;
            case 6: generateAdminReportScreen(); break;
            case 7: viewAdminReportScreen();     break;
            case 0: inStaffMenu = false;         break;
            default:
                cout << "Invalid choice." << endl;
                pauseScreen();
        }
    }
}

void manageStudentsScreen() {
    int choice;
    bool inSubMenu = true;
    while (inSubMenu) {
        clearScreen();
        cout << "======= MANAGE STUDENT RECORDS =======" << endl;
        cout << "1. View All Students" << endl;
        cout << "2. Add New Student" << endl;
        cout << "3. Edit Student Details" << endl;
        cout << "4. Delete Student" << endl;
        cout << "0. Back" << endl;
        cout << "=======================================" << endl;
        cout << "Enter your choice: ";
        choice = readIntInput();
        switch (choice) {
            case 1: viewAllStudentsScreen(); break;
            case 2: addStudentScreen();      break;
            case 3: editStudentScreen();     break;
            case 4: deleteStudentScreen();   break;
            case 0: inSubMenu = false;       break;
            default:
                cout << "Invalid choice." << endl;
                pauseScreen();
        }
    }
}

void viewAllStudentsScreen() {
    clearScreen();
    cout << "============ ALL STUDENT RECORDS ============" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        pauseScreen();
        return;
    }

    cout << left
         << setw(8)  << "ID"
         << setw(20) << "Name"
         << setw(10) << "Program"
         << setw(15) << "Phone"
         << setw(25) << "Email"
         << endl;
    cout << "----------------------------------------------------------------------" << endl;

    for (int i = 0; i < studentCount; i++) {
        cout << left
             << setw(8)  << studentArray[i]->getID()
             << setw(20) << studentArray[i]->getName()
             << setw(10) << studentArray[i]->getProgram()
             << setw(15) << studentArray[i]->getPhone()
             << setw(25) << studentArray[i]->getEmail()
             << endl;
    }

    pauseScreen();
}

void addStudentScreen() {
    clearScreen();
    cout << "========= ADD NEW STUDENT =========" << endl;

    if (studentCount >= MAX_STUDENTS) {
        cout << "Student record list is full." << endl;
        pauseScreen();
        return;
    }

    try {
        string name, ic, email, password, phone, program;

        cout << "Enter Name (or 0 to cancel): ";
        cin.ignore();
        getline(cin, name);

        if (name == "0") {
            
            return;
        }

        int icAttempts = 0;
        do {
            cout << "Enter IC Number (must be exactly 12 digits, or 0 to cancel): ";
            cin >> ic;
            if (ic == "0") { 
                return;
            }
            icAttempts++;
            if (!isValidIC(ic)) {
                cout << "Invalid IC number. It must contain exactly 12 digits." << endl;
            } else if (!isICUnique(ic, "")) {
                cout << "This IC number is already registered to another student." << endl;
            }
            if ((!isValidIC(ic) || !isICUnique(ic, "")) && icAttempts >= 5) {
                throw runtime_error("Too many invalid IC attempts. Operation cancelled.");
            }
        } while (!isValidIC(ic) || !isICUnique(ic, ""));

        int emailAttempts = 0;
        do {
            cout << "Enter Email (or 0 to cancel): ";
            cin >> email;
            if (email == "0") {
                return;
            }
            emailAttempts++;
            if (!isValidEmail(email)) {
                cout << "Invalid email format." << endl;
            } else if (!isEmailUnique(email, "")) {
                cout << "This email is already registered to another student." << endl;
            }
            if ((!isValidEmail(email) || !isEmailUnique(email, "")) && emailAttempts >= 5) {
                throw runtime_error("Too many invalid email attempts. Operation cancelled.");
            }
        } while (!isValidEmail(email) || !isEmailUnique(email, ""));

        cout << "Enter Password: ";
        cin >> password;

        int phoneAttempts = 0;
        do {
            cout << "Enter Phone Number (numeric, below 12 digits, or 0 to cancel): ";
            cin >> phone;
            if (phone == "0") {   
                return; 
            }
            phoneAttempts++;
            if (!isValidPhone(phone)) {
                cout << "Invalid phone number. Must be numeric and below 12 digits." << endl;
                if (phoneAttempts >= 5) {
                    throw runtime_error("Too many invalid phone attempts. Operation cancelled.");
                }
            }
        } while (!isValidPhone(phone));

        cout << "Enter Program (CS/IT/SE): ";
        cin >> program;

        string newID = generateNextStudentID();
        string intake = "September2026";

        studentArray[studentCount] = new Student(newID, name, password, ic, phone, email, program, intake);
        studentCount++;
        appendStudentToFile(studentArray[studentCount - 1]);

        cout << "=========================================" << endl;
        cout << "Student added successfully! New Student ID: " << newID << endl;
        cout << "=========================================" << endl;
    }
    catch (runtime_error &e) {
        cout << "Operation failed: " << e.what() << endl;
    }

    pauseScreen();
}

void editStudentScreen() {
    clearScreen();
    cout << "======== EDIT STUDENT DETAILS ========" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Student ID to edit (or 0 to cancel): ";
    string id;
    cin >> id;

    if (id == "0") {
        
        return;
    }

    int idx = findStudentIndexByID(id);
    if (idx == -1) {
        cout << "Student ID not found." << endl;
        pauseScreen();
        return;
    }
    
    while (true)
    {
	    studentArray[idx]->displayProfile();
	
	    cout << "\n1. Edit Name" << endl;
	    cout << "2. Edit Program" << endl;
	    cout << "3. Edit Phone" << endl;
	    cout << "4. Edit Email" << endl;
	    cout << "0. Back (cancel edit)" << endl;
	    cout << "Enter your choice: ";
	    int choice = readIntInput();
	
	    switch (choice) {
	        case 1: {
	            string newName;
	            cout << "Enter new Name: ";
	            cin.ignore();
	            getline(cin, newName);
	            studentArray[idx]->setName(newName);
	            cout << "Name updated." << endl;
	            break;
	        }
	        case 2: {
	            string newProgram;
	            cout << "Enter new Program: ";
	            cin >> newProgram;
	            studentArray[idx]->setProgram(newProgram);
	            cout << "Program updated." << endl;
	            break;
	        }
	        case 3: {
	            string newPhone;
	            int attempts = 0;
	            bool cancelled = false;
	            do {
	                cout << "Enter new Phone (numeric, below 12 digits, or 0 to cancel): ";
	                cin >> newPhone;
	                if (newPhone == "0") { cancelled = true; break; }
	                attempts++;
	                if (!isValidPhone(newPhone)) {
	                    cout << "Invalid phone number." << endl;
	                    if (attempts >= 5) {
	                        cout << "Too many invalid attempts. Edit cancelled." << endl;
	                        pauseScreen();
	                        break;
	                    }
	                }
	            } while (!isValidPhone(newPhone));
	            if (cancelled) { 
	             break; 
	            }
	            studentArray[idx]->setPhone(newPhone);
	            cout << "Phone updated." << endl;
	            break;
	        }
	        case 4: {
	            string newEmail;
	            bool cancelled = false;
	            do{
	            	cout << "Enter new Email (or 0 to cancel): ";
	            	cin >> newEmail;
	            	if (newEmail == "0") { cancelled = true; break; }

	            	if (!isValidEmail(newEmail)) {
	            	    cout << "Invalid email format." << endl;
	            	} else if (!isEmailUnique(newEmail, studentArray[idx]->getID())) {
	            	    cout << "This email is already used by another student." << endl;
	            	}
				}while(!isValidEmail(newEmail) || !isEmailUnique(newEmail, studentArray[idx]->getID()));

				if (cancelled) {
				    cout << "Email edit cancelled." << endl;
				    break;
				}

	            studentArray[idx]->setEmail(newEmail);
	            cout << "Email updated." << endl;
	            break;
	        }
	        case 0:
	            return;
	        default:
	            cout << "Invalid choice." << endl;
	            
	            continue;
	    }
	
	    rewriteStudentsFile();
	    
	    cout<<"\nUpdated Student Information:\n";
	    
	    pauseScreen();
    }
}

void deleteStudentScreen() {
    clearScreen();
    cout << "============ DELETE STUDENT ============" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        
        return;
    }

    cout << "Enter Student ID to delete (or 0 to cancel): ";
    string id;
    cin >> id;

    if (id == "0") {
        
        return;
    }

    int idx = findStudentIndexByID(id);
    if (idx == -1) {
        cout << "Student ID not found." << endl;
        pauseScreen();
        return;
    }

    cout << "\nAre you sure you want to delete this student record?" << endl;
    studentArray[idx]->displayProfile();
    cout << "1. Yes, delete" << endl;
    cout << "0. No, cancel" << endl;
    cout << "Enter your choice: ";
    int confirm = readIntInput();

    if (confirm != 1) {   
        return;
    }

    delete studentArray[idx]; // dynamic memory operation: delete

    for (int i = idx; i < studentCount - 1; i++) {
        studentArray[i] = studentArray[i + 1];
    }
    studentArray[studentCount - 1] = nullptr;
    studentCount--;

    rewriteStudentsFile();
    rewriteGradesFile(); // removes the deleted student's grade rows too

    cout << "Student deleted successfully." << endl;
    pauseScreen();
}

void manageCoursesScreen() {
    int choice;
    bool inSubMenu = true;
    while (inSubMenu) {
        clearScreen();
        cout << "======= MANAGE COURSE RECORDS =======" << endl;
        cout << "1. View All Courses" << endl;
        cout << "2. Add New Course" << endl;
        cout << "3. Edit Course Details" << endl;
        cout << "4. Delete Course" << endl;
        cout << "0. Back" << endl;
        cout << "======================================" << endl;
        cout << "Enter your choice: ";
        choice = readIntInput();
        switch (choice) {
            case 1: viewAllCoursesScreen(); break;
            case 2: addCourseScreen();      break;
            case 3: editCourseScreen();     break;
            case 4: deleteCourseScreen();   break;
            case 0: inSubMenu = false;      break;
            default:
                cout << "Invalid choice." << endl;
                pauseScreen();
        }
    }
}

void viewAllCoursesScreen() {
    clearScreen();
    cout << "============ ALL COURSE RECORDS ============" << endl;

    if (courseCount == 0) {
        cout << "No course records found." << endl;
        pauseScreen();
        return;
    }

    cout << left
         << setw(10) << "Code"
         << setw(28) << "Course Name"
         << setw(8)  << "Credit"
         << setw(8)  << "Year"
         << setw(10) << "Dept"
         << endl;
    cout << "----------------------------------------------------------------------" << endl;

    for (int i = 0; i < courseCount; i++) {
        cout << left
             << setw(10) << courseArray[i].code
             << setw(28) << courseArray[i].name
             << setw(8)  << courseArray[i].credit
             << setw(8)  << courseArray[i].year
             << setw(10) << courseArray[i].department
             << endl;
    }

    pauseScreen();
}

void addCourseScreen() {
    clearScreen();
    cout << "========= ADD NEW COURSE =========" << endl;

    if (courseCount >= MAX_COURSES) {
        cout << "Course catalog is full." << endl;
        pauseScreen();
        return;
    }

    Course c;
    cout << "Enter Course Code (or 0 to cancel): ";
    cin >> c.code;

    if (c.code == "0") {
        
        return;
    }

    if (findCourseIndexByCode(c.code) != -1) {
        cout << "A course with this code already exists." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Course Name: ";
    cin.ignore();
    getline(cin, c.name);

    cout << "Enter Credit Hours (must be between 1 and 8): ";
    c.credit = readIntInput();
    while (c.credit < 1 || c.credit > 8) {
        cout << "Invalid credit value. Please enter a number between 1 and 8: ";
        c.credit = readIntInput();
    }

    cout << "Enter Year (e.g. Year1): ";
    cin >> c.year;

    cout << "Enter Department: ";
    cin >> c.department;

    courseArray[courseCount] = c;
    courseCount++;
    appendCourseToFile(c);

    cout << "Course added successfully." << endl;
    pauseScreen();
}

void editCourseScreen() {
    clearScreen();
    cout << "======== EDIT COURSE DETAILS ========" << endl;

    if (courseCount == 0) {
        cout << "No course records found." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Course Code to edit (or 0 to cancel): ";
    string code;
    cin >> code;

    if (code == "0") {
        
        return;
    }

    int idx = findCourseIndexByCode(code);
    if (idx == -1) {
        cout << "Course code not found." << endl;
        pauseScreen();
        return;
    }

    cout << "\nCurrent details: " << courseArray[idx].code << " - " << courseArray[idx].name
         << " (" << courseArray[idx].credit << " credit(s))" << endl;

    cout << "1. Edit Course Name" << endl;
    cout << "2. Edit Credit Hours" << endl;
    cout << "3. Edit Department" << endl;
    cout << "0. Back (cancel edit)" << endl;
    cout << "Enter your choice: ";
    int choice = readIntInput();

    switch (choice) {
        case 1:
            cout << "Enter new Course Name: ";
            cin.ignore();
            getline(cin, courseArray[idx].name);
            break;
        case 2:
            cout << "Enter new Credit Hours (must be between 1 and 8): ";
            courseArray[idx].credit = readIntInput();
            while (courseArray[idx].credit < 1 || courseArray[idx].credit > 8) {
                cout << "Invalid credit value. Please enter a number between 1 and 8: ";
                courseArray[idx].credit = readIntInput();
            }
            break;
        case 3:
            cout << "Enter new Department: ";
            cin >> courseArray[idx].department;
            break;
        case 0:
            
            return;
        default:
            cout << "Invalid choice." << endl;
            pauseScreen();
            return;
    }

    rewriteCoursesFile();
    cout << "Course updated successfully." << endl;
    pauseScreen();
}

void deleteCourseScreen() {
    clearScreen();
    cout << "============ DELETE COURSE ============" << endl;

    if (courseCount == 0) {
        cout << "No course records found." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Course Code to delete (or 0 to cancel): ";
    string code;
    cin >> code;

    if (code == "0") {
        
        return;
    }

    int idx = findCourseIndexByCode(code);
    if (idx == -1) {
        cout << "Course code not found." << endl;
        pauseScreen();
        return;
    }

    cout << "\nAre you sure you want to delete this course?" << endl;
    cout << courseArray[idx].code << " - " << courseArray[idx].name << endl;
    cout << "1. Yes, delete" << endl;
    cout << "0. No, cancel" << endl;
    cout << "Enter your choice: ";
    int confirm = readIntInput();

    if (confirm != 1) {
        return;
    }

    for (int i = idx; i < courseCount - 1; i++) {
        courseArray[i] = courseArray[i + 1];
    }
    courseCount--;

    rewriteCoursesFile();
    cout << "Course deleted successfully." << endl;
    pauseScreen();
}

void manageGradesScreen() {
    int choice;
    bool inSubMenu = true;
    while (inSubMenu) {
        clearScreen();
        cout << "======= MANAGE GRADE RECORDS =======" << endl;
        cout << "1. View All Grades for a Student" << endl;
        cout << "2. Assign/Update Grade for Student" << endl;
        cout << "3. Approve Pending Enrollment" << endl;
        cout << "0. Back" << endl;
        cout << "=====================================" << endl;
        cout << "Enter your choice: ";
        choice = readIntInput();
        switch (choice) {
            case 1: viewGradesForStudentScreen(); break;
            case 2: assignGradeScreen();          break;
            case 3: approveEnrollmentScreen();    break;
            case 0: inSubMenu = false;            break;
            default:
                cout << "Invalid choice." << endl;
                pauseScreen();
        }
    }
}

void viewGradesForStudentScreen() {
    clearScreen();
    cout << "===== VIEW GRADES FOR A STUDENT =====" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Student ID (or 0 to cancel): ";
    string id;
    cin >> id;

    if (id == "0") {
    
        return;
    }

    int idx = findStudentIndexByID(id);
    if (idx == -1) {
        cout << "Student ID not found." << endl;
        pauseScreen();
        return;
    }

    cout << "\nGrades for " << studentArray[idx]->getName() << " (" << id << "):" << endl;
    studentArray[idx]->getGradeList()->displayAll();

    pauseScreen();
}

void assignGradeScreen() {
    clearScreen();
    cout << "===== ASSIGN/UPDATE GRADE =====" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Student ID (or 0 to cancel): ";
    string id;
    cin >> id;

    if (id == "0") {
        
        return;
    }

    int idx = findStudentIndexByID(id);
    if (idx == -1) {
        cout << "Student ID not found." << endl;
        pauseScreen();
        return;
    }

    studentArray[idx]->getGradeList()->displayAll();

    cout << "\nEnter Course Code to assign/update a grade for (or 0 to cancel): ";
    string code;
    cin >> code;

    if (code == "0") {
        
        return;
    }

    // Grade whitelist validation loop - anything outside the fixed
    // letter-grade set (checked by isValidGradeFormat) is rejected and
    // re-prompted, up to 5 attempts before the operation is cancelled.
    string grade;
    int gradeAttempts = 0;
    while (true) {
        cout << "Enter Grade (A+, A, A-, B+, B, B-, C+, C, C-, D+, D, D-, F, Pending, or 0 to cancel): ";
        cin >> grade;
        if (grade == "0") {
            return;
        }
        if (isValidGradeFormat(grade)) break;

        cout << "Invalid grade code. Please use one of the accepted grade values." << endl;
        gradeAttempts++;
        if (gradeAttempts >= 5) {
            cout << "Too many invalid attempts. Operation cancelled." << endl;
            pauseScreen();
            return;
        }
    }

    // Check whether this student already has a record for this course so
    // we can show the admin its current semester before they overwrite it.
    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    studentArray[idx]->getGradeList()->toArray(tempArr, n);

    bool existingRecord = false;
    string currentSemester = "";
    for (int i = 0; i < n; i++) {
        if (tempArr[i].getCourseCode() == code) {
            existingRecord = true;
            currentSemester = tempArr[i].getTerm().semester;
            break;
        }
    }

    if (existingRecord) {
        cout << "This record currently shows Semester: " << currentSemester
             << ". You may now enter the correct/updated Semester below." << endl;
    }

    string semester = promptSemesterInput("Enter the Semester for this grade record");
    if (semester == "0") {
        cout << "Operation cancelled." << endl;
        pauseScreen();
        return;
    }

    bool updated = studentArray[idx]->getGradeList()->updateGradeAndSemesterByCourse(code, grade, semester);

    if (!updated) {
        // Course isn't on this student's record yet - look it up in the
        // catalog and add it directly with the given grade and semester.
        int courseIdx = findCourseIndexByCode(code);
        if (courseIdx == -1) {
            cout << "Course code not found in catalog and student has no record for it." << endl;
            pauseScreen();
            return;
        }
        GradeEntry newEntry(id, courseArray[courseIdx].code, courseArray[courseIdx].name,
                             courseArray[courseIdx].credit, grade, semester);
        studentArray[idx]->getGradeList()->insert(newEntry);
    }

    rewriteGradesFile();
    cout << "Grade assigned/updated successfully." << endl;
    pauseScreen();
}

void approveEnrollmentScreen() {
    clearScreen();
    cout << "===== APPROVE PENDING ENROLLMENT =====" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Student ID (or 0 to cancel): ";
    string id;
    cin >> id;

    if (id == "0") {
        
        return;
    }

    int idx = findStudentIndexByID(id);
    if (idx == -1) {
        cout << "Student ID not found." << endl;
        pauseScreen();
        return;
    }

    GradeEntry tempArr[MAX_TEMP_GRADES];
    int n = 0;
    studentArray[idx]->getGradeList()->toArray(tempArr, n);

    bool hasPending = false;
    cout << "\nPending Enrollments for " << studentArray[idx]->getName() << ":" << endl;
    for (int i = 0; i < n; i++) {
        if (tempArr[i].getGrade() == "Pending") {
            cout << tempArr[i].getCourseCode() << " - " << tempArr[i].getCourseName() << endl;
            hasPending = true;
        }
    }

    if (!hasPending) {
        cout << "No pending enrollments for this student." << endl;
        pauseScreen();
        return;
    }

    cout << "\nEnter Course Code to approve (or 0 to cancel): ";
    string code;
    cin >> code;

    if (code == "0") {
        
        return;
    }

    // Grade whitelist validation loop, same rule set as assignGradeScreen().
    string grade;
    int gradeAttempts = 0;
    while (true) {
        cout << "Enter the Grade to assign (A+, A, A-, B+, B, B-, C+, C, C-, D+, D, D-, F, Pending, or 0 to cancel): ";
        cin >> grade;
        if (grade == "0") {
            return;
        }
        if (isValidGradeFormat(grade)) break;

        cout << "Invalid grade code. Please use one of the accepted grade values." << endl;
        gradeAttempts++;
        if (gradeAttempts >= 5) {
            cout << "Too many invalid attempts. Operation cancelled." << endl;
            pauseScreen();
            return;
        }
    }

    // Find the current semester on the pending record so the admin can
    // confirm it or correct it as part of the approval step.
    string currentSemester = "";
    for (int i = 0; i < n; i++) {
        if (tempArr[i].getCourseCode() == code) {
            currentSemester = tempArr[i].getTerm().semester;
            break;
        }
    }
    cout << "This pending enrollment currently shows Semester: " << currentSemester
         << ". Confirm or correct it below." << endl;

    string semester = promptSemesterInput("Enter the Semester to finalize for this approval");
    if (semester == "0") {
        cout << "Operation cancelled." << endl;
        pauseScreen();
        return;
    }

    bool updated = studentArray[idx]->getGradeList()->updateGradeAndSemesterByCourse(code, grade, semester);
    if (!updated) {
        cout << "Course code not found in this student's pending list." << endl;
        pauseScreen();
        return;
    }

    rewriteGradesFile();
    cout << "Enrollment approved and grade assigned." << endl;
    pauseScreen();
}

void sortStudentsScreen() {
    clearScreen();
    cout << "========== SORT STUDENTS ==========" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        pauseScreen();
        return;
    }

    cout << "1. Sort by GPA (Highest to Lowest)" << endl;
    cout << "2. Sort by Student ID" << endl;
    cout << "0. Back" << endl;
    cout << "Enter your choice: ";
    int choice = readIntInput();

    if (choice == 0) {

        return;
    }

    Student* sortedArr[MAX_STUDENTS];
    for (int i = 0; i < studentCount; i++) sortedArr[i] = studentArray[i];

    if (choice == 1) {
        quickSort(sortedArr, 0, studentCount - 1, "GPA");
    } else if (choice == 2) {
        quickSort(sortedArr, 0, studentCount - 1, "ID");
    } else {
        cout << "Invalid choice." << endl;
        pauseScreen();
        return;
    }

    cout << "\n----- Sorted Results -----" << endl;
    cout << left
         << setw(8)  << "ID"
         << setw(20) << "Name"
         << setw(10) << "Program"
         << setw(10) << "GPA"
         << endl;
    cout << "------------------------------------------------" << endl;

    cout << fixed << setprecision(2);
    for (int i = 0; i < studentCount; i++) {
        cout << left
             << setw(8)  << sortedArr[i]->getID()
             << setw(20) << sortedArr[i]->getName()
             << setw(10) << sortedArr[i]->getProgram()
             << setw(10) << calculateStudentGPA(sortedArr[i])
             << endl;
    }

    pauseScreen();
}

void searchStudentScreen() {
    clearScreen();
    cout << "========= SEARCH STUDENT RECORD =========" << endl;

    if (studentCount == 0) {
        cout << "No student records found." << endl;
        pauseScreen();
        return;
    }

    cout << "Enter Student ID to search (or 0 to cancel): ";
    string id;
    cin >> id;

    if (id == "0") {
        
        return;
    }

    Student* sortedArr[MAX_STUDENTS];
    for (int i = 0; i < studentCount; i++) sortedArr[i] = studentArray[i];
    quickSort(sortedArr, 0, studentCount - 1, "ID");

    int result = binarySearch(sortedArr, studentCount, id);

    if (result != -1) {
        cout << "\nStudent Found:" << endl;
        sortedArr[result]->displayProfile();
        cout << "Cumulative GPA: " << fixed << setprecision(2)
             << calculateStudentGPA(sortedArr[result]) << endl;
    } else {
        cout << "\nStudent ID not found." << endl;
    }

    pauseScreen();  
}

void generateAdminReportScreen() {
    clearScreen();
    cout << "===== GENERATE DEPARTMENT REPORT =====" << endl;

    if (studentCount == 0) {
        cout << "No student records found. Cannot generate report." << endl;
        pauseScreen();
        return;
    }

    ofstream outFile("admin_report.txt");
    if (!outFile) {
        cout << "Error: could not open admin_report.txt for writing." << endl;
        pauseScreen();
        return;
    }

    outFile << "============================================================" << endl;
    outFile << "           DEPARTMENT ACADEMIC SUMMARY REPORT" << endl;
    outFile << "============================================================" << endl;

    double totalGPA = 0.0;
    for (int i = 0; i < studentCount; i++) {
        totalGPA += calculateStudentGPA(studentArray[i]);
    }
    double overallAverage = totalGPA / studentCount;

    outFile << fixed << setprecision(2);
    outFile << "Total Students        : " << studentCount << endl;
    outFile << "Total Courses Offered : " << courseCount << endl;
    outFile << "Overall Average GPA   : " << overallAverage << endl;
    outFile << "============================================================" << endl;

    // Average GPA per program (manual grouping - no STL map/set)
    string programs[MAX_STUDENTS];
    int programCount = 0;
    for (int i = 0; i < studentCount; i++) {
        string prog = studentArray[i]->getProgram();
        bool found = false;
        for (int j = 0; j < programCount; j++) {
            if (programs[j] == prog) { found = true; break; }
        }
        if (!found) {
            programs[programCount] = prog;
            programCount++;
        }
    }

    outFile << "\nAverage GPA by Program:" << endl;
    outFile << "------------------------------------------------------------" << endl;
    for (int p = 0; p < programCount; p++) {
        double sum = 0.0;
        int count = 0;
        for (int i = 0; i < studentCount; i++) {
            if (studentArray[i]->getProgram() == programs[p]) {
                sum += calculateStudentGPA(studentArray[i]);
                count++;
            }
        }
        double avg = (count > 0) ? (sum / count) : 0.0;
        outFile << left << setw(10) << programs[p]
                << "Students: " << setw(5) << count
                << "Avg GPA: " << avg << endl;
    }

    // Top students by GPA (capped at 5, or fewer if the cohort is smaller)
    Student* sortedArr[MAX_STUDENTS];
    for (int i = 0; i < studentCount; i++) sortedArr[i] = studentArray[i];
    quickSort(sortedArr, 0, studentCount - 1, "GPA");

    outFile << "\nTop Students by GPA:" << endl;
    outFile << "------------------------------------------------------------" << endl;
    outFile << left << setw(8) << "ID" << setw(20) << "Name" << setw(10) << "Program" << setw(8) << "GPA" << endl;

    int topN = (studentCount < 5) ? studentCount : 5;
    for (int i = 0; i < topN; i++) {
        outFile << left
                << setw(8)  << sortedArr[i]->getID()
                << setw(20) << sortedArr[i]->getName()
                << setw(10) << sortedArr[i]->getProgram()
                << setw(8)  << calculateStudentGPA(sortedArr[i])
                << endl;
    }

    outFile << "============================================================" << endl;
    outFile.close();

    cout << "Report saved to admin_report.txt" << endl;
    pauseScreen();
}

void viewAdminReportScreen() {
    clearScreen();
    cout << "========= VIEW SAVED REPORT =========" << endl;

    ifstream inFile("admin_report.txt");
    if (!inFile) {
        cout << "No saved report found. Please generate a report first." << endl;
        pauseScreen();
        return;
    }

    string line;
    while (getline(inFile, line)) {
        cout << line << endl;
    }
    inFile.close();

    pauseScreen();
}
