# Student Academic Records System 🎓

![C++](https://img.shields.io/badge/Language-C++-blue.svg)
![Environment](https://img.shields.io/badge/Environment-Dev_C++-lightgrey.svg)
![Status](https://img.shields.io/badge/Status-Completed-success.svg)

> **Course:** TDS4223 - Data Structures and Algorithms  
> **Group:** 11  
> **Team Members:** LIU JIUN LE, TAN LE YONG, NG ZHE JUN, ANG QI YANG, LIM MING XUAN

## 📌 Project Overview
The **Student Academic Records System** is a comprehensive, console-based C++ application designed to modernize university academic data management. It bridges the operational gap between Students and Administrative Staff, providing a seamless, real-time synchronized environment for course enrollments, grade tracking, and profile management.

### ⚠️ Critical Technical Constraint: "Zero STL"
This project was built under a strict academic constraint: **The complete prohibition of the C++ Standard Template Library (STL).** No `<vector>`, `<algorithm>`, `<list>`, or any pre-built containers were used. Every dynamic architecture, sorting mechanism, and search logic was manually engineered from scratch using raw arrays, pointers, and fundamental memory management techniques.

## 🚀 Key System Features

### Student Module
- **Registration & Authentication:** Secure login system with strict input validation (e.g., 12-digit IC, numeric phone numbers).
- **Profile Management:** View and update personal contact information and passwords.
- **Academic Dashboard:** View grades, search for specific courses, and sort academic records.
- **Course Enrollment:** Request to add or drop courses (creates "Pending" requests for Staff approval).
- **Report Generation:** Dynamically calculates cumulative GPA and generates a personalized `student_report.txt`.

### Staff/Admin Module
- **Student Management:** Add, edit, and delete student records.
- **Course Catalog Management:** Add, edit, and delete courses available in the university.
- **Enrollment Processing:** Review and approve "Pending" course enrollments and assign final grades.
- **System-Wide Sorting & Searching:** Sort students by GPA/ID and search for specific records.
- **Department Reporting:** Generates comprehensive administrative summaries (`admin_report.txt`).

## 🧠 Data Structures & Algorithms Applied

1. **Singly Linked List (Dynamic Non-Primitive Structure)**
   - Used to manage student grades (`GradeLinkedList`). 
   - Handles dynamic memory allocation (`new` / `delete`) for Node insertion and deletion, allowing flexible management of "Pending" and "Graded" course records without hardcoded array limits.

2. **Quick Sort (Recursive, In-Place)**
   - Manually implemented and **overloaded** to sort multiple data types.
   - Sorts `GradeEntry` objects (by Grade, Semester, or Course Code).
   - Sorts `Course` objects (by Course Code).
   - Sorts `Student` objects (by ID or calculated GPA).

3. **Binary Search ($O(\log n)$)**
   - Manually implemented and **overloaded** for rapid data retrieval.
   - Searches for specific courses in the catalog.
   - Searches for specific student IDs within the administrative dashboard.

## 📂 File I/O Architecture
Data persistence is handled via custom-parsed, pipe-delimited (`|`) text files. Real-time file synchronization ensures that actions taken in one module are immediately reflected in the other.
- `students.txt` - Stores student credentials and profile data.
- `staff.txt` - Stores pre-provisioned administrative accounts.
- `courses.txt` - The university course catalog.
- `grades.txt` - Stores all academic records and pending enrollment requests.

## 🛠️ How to Compile and Run

1. **Prerequisites:** - This project is strictly designed for the **Dev C++** compiler (MinGW/GCC).
2. **Setup:**
   - Clone this repository.
   - Ensure all `.txt` files (`students.txt`, `courses.txt`, `grades.txt`, `staff.txt`) are placed in the **same root directory** as the `.cpp` source file.
3. **Execution:**
   - Open `Project_GR11.cpp` in Dev C++.
   - Click `Compile & Run` (F11).
   - Navigate the console menus using integer inputs.

---
*Developed with ❤️ by Group 11 for TDS4223.*