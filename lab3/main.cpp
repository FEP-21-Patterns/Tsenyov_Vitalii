#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <map>

class Teacher {
protected:
    std::string name;
public:
    explicit Teacher(const std::string& n) : name(n) {}
    virtual ~Teacher() = default;
    
    std::string getName() const { return name; }
    
    virtual bool canTeachLecture() const = 0;
    virtual bool canTeachPractical() const = 0;
    virtual bool canSuperviseCourseWork() const = 0;
    
    virtual std::string getType() const = 0;
};

class Lecturer : public Teacher {
public:
    explicit Lecturer(const std::string& n) : Teacher(n) {}
    
    bool canTeachLecture() const override { return true; }
    bool canTeachPractical() const override { return true; }
    bool canSuperviseCourseWork() const override { return true; }
    std::string getType() const override { return "Lecturer"; }
};

class Assistant : public Teacher {
public:
    explicit Assistant(const std::string& n) : Teacher(n) {}
    
    bool canTeachLecture() const override { return false; }
    bool canTeachPractical() const override { return true; }
    bool canSuperviseCourseWork() const override { return true; }
    std::string getType() const override { return "Assistant"; }
};

class ExternalMentor : public Teacher {
public:
    explicit ExternalMentor(const std::string& n) : Teacher(n) {}
    
    bool canTeachLecture() const override { return false; }
    bool canTeachPractical() const override { return false; }
    bool canSuperviseCourseWork() const override { return true; }
    std::string getType() const override { return "External Mentor"; }
};

class Session {
protected:
    std::string time;
    std::string room;
    std::shared_ptr<Teacher> teacher;
    
public:
    // ЦЕЙ РЯДОК ВИПРАВЛЕНО
    Session(const std::string& t, const std::string& r, std::shared_ptr<Teacher> teach)
        : time(t), room(r), teacher(std::move(teach)) {}
    
    virtual ~Session() = default;
    
    std::string getTime() const { return time; }
    std::string getRoom() const { return room; }
    std::shared_ptr<Teacher> getTeacher() const { return teacher; }
    
    virtual std::string getType() const = 0;
    
    virtual std::string getInfo() const {
        std::stringstream ss;
        ss << getType() << " | " << time << " | Room: " << room 
           << " | Teacher: " << teacher->getName() << " (" << teacher->getType() << ")";
        return ss.str();
    }
};

class LectureSession : public Session {
public:
    LectureSession(const std::string& t, const std::string& r, std::shared_ptr<Teacher> teach)
        : Session(t, r, std::move(teach)) {
        if (!teacher->canTeachLecture()) {
            throw std::invalid_argument(teacher->getName() + " cannot teach lectures!");
        }
    }
    
    std::string getType() const override { return "Lecture"; }
};

class PracticalSession : public Session {
public:
    PracticalSession(const std::string& t, const std::string& r, std::shared_ptr<Teacher> teach)
        : Session(t, r, std::move(teach)) {
        if (!teacher->canTeachPractical()) {
            throw std::invalid_argument(teacher->getName() + " cannot teach practicals!");
        }
    }
    
    std::string getType() const override { return "Practical"; }
};

class SessionFactory {
public:
    virtual ~SessionFactory() = default;
    virtual std::shared_ptr<Session> createSession(const std::string& time, const std::string& room, 
                                                   std::shared_ptr<Teacher> teacher) = 0;
};

class LectureFactory : public SessionFactory {
public:
    std::shared_ptr<Session> createSession(const std::string& time, const std::string& room, 
                                           std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<LectureSession>(time, room, std::move(teacher));
    }
};

class PracticalFactory : public SessionFactory {
public:
    std::shared_ptr<Session> createSession(const std::string& time, const std::string& room, 
                                           std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<PracticalSession>(time, room, std::move(teacher));
    }
};

class CourseWork {
protected:
    std::string title;
    std::shared_ptr<Teacher> supervisor;
    bool submitted;
    
public:
    CourseWork(const std::string& t, std::shared_ptr<Teacher> sup) 
        : title(t), supervisor(std::move(sup)), submitted(false) {
        if (!supervisor->canSuperviseCourseWork()) {
            throw std::invalid_argument(supervisor->getName() + " cannot supervise coursework!");
        }
    }
    
    virtual ~CourseWork() = default;
    
    virtual void submit() = 0;
    virtual std::string getSubmissionType() const = 0;
    
    std::string getInfo() const {
        std::stringstream ss;
        ss << title << " | Supervisor: " << supervisor->getName() 
           << " | Type: " << getSubmissionType() 
           << " | Status: " << (submitted ? "Submitted" : "Not Submitted");
        return ss.str();
    }
    
    bool isSubmitted() const { return submitted; }
    std::shared_ptr<Teacher> getSupervisor() const { return supervisor; }
};

class OnlineSubmission : public CourseWork {
    std::string fileURL;
public:
    explicit OnlineSubmission(const std::string& t, std::shared_ptr<Teacher> sup) 
        : CourseWork(t, std::move(sup)) {}
    
    void submit() override {
        fileURL = "http://courses.example.com/submit/" + title;
        submitted = true;
        std::cout << "✓ Submitted online: " << fileURL << std::endl;
    }
    
    std::string getSubmissionType() const override { return "Online Upload"; }
};

class GitHubSubmission : public CourseWork {
    std::string repoURL;
public:
    explicit GitHubSubmission(const std::string& t, std::shared_ptr<Teacher> sup) 
        : CourseWork(t, std::move(sup)) {}
    
    void submit() override {
        repoURL = "https://github.com/student/" + title;
        submitted = true;
        std::cout << "✓ Submitted to GitHub: " << repoURL << std::endl;
    }
    
    std::string getSubmissionType() const override { return "GitHub Repository"; }
};

class OralDefense : public CourseWork {
    std::string defenseDate;
public:
    explicit OralDefense(const std::string& t, std::shared_ptr<Teacher> sup) 
        : CourseWork(t, std::move(sup)) {}
    
    void submit() override {
        defenseDate = "2024-12-15";
        submitted = true;
        std::cout << "✓ Oral defense scheduled: " << defenseDate << std::endl;
    }
    
    std::string getSubmissionType() const override { return "Oral Defense"; }
};

class CourseFactory {
public:
    virtual ~CourseFactory() = default;
    
    virtual std::shared_ptr<Session> createLecture(const std::string& time, const std::string& room,
                                                   std::shared_ptr<Teacher> teacher) = 0;
    virtual std::shared_ptr<Session> createPractical(const std::string& time, const std::string& room,
                                                     std::shared_ptr<Teacher> teacher) = 0;
    virtual std::shared_ptr<CourseWork> createCourseWork(const std::string& title,
                                                         std::shared_ptr<Teacher> supervisor) = 0;
};

class ProgrammingCourseFactory : public CourseFactory {
public:
    std::shared_ptr<Session> createLecture(const std::string& time, const std::string& room,
                                           std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<LectureSession>(time, room, std::move(teacher));
    }
    
    std::shared_ptr<Session> createPractical(const std::string& time, const std::string& room,
                                             std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<PracticalSession>(time, room, std::move(teacher));
    }
    
    std::shared_ptr<CourseWork> createCourseWork(const std::string& title,
                                                 std::shared_ptr<Teacher> supervisor) override {
        return std::make_shared<GitHubSubmission>("Programming Project: " + title, std::move(supervisor));
    }
};

class DatabasesCourseFactory : public CourseFactory {
public:
    std::shared_ptr<Session> createLecture(const std::string& time, const std::string& room,
                                           std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<LectureSession>(time, room, std::move(teacher));
    }
    
    std::shared_ptr<Session> createPractical(const std::string& time, const std::string& room,
                                             std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<PracticalSession>(time, room, std::move(teacher));
    }
    
    std::shared_ptr<CourseWork> createCourseWork(const std::string& title,
                                                 std::shared_ptr<Teacher> supervisor) override {
        return std::make_shared<OnlineSubmission>("Database Project: " + title, std::move(supervisor));
    }
};

class MathCourseFactory : public CourseFactory {
public:
    std::shared_ptr<Session> createLecture(const std::string& time, const std::string& room,
                                           std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<LectureSession>(time, room, std::move(teacher));
    }
    
    std::shared_ptr<Session> createPractical(const std::string& time, const std::string& room,
                                             std::shared_ptr<Teacher> teacher) override {
        return std::make_shared<PracticalSession>(time, room, std::move(teacher));
    }
    
    std::shared_ptr<CourseWork> createCourseWork(const std::string& title,
                                                 std::shared_ptr<Teacher> supervisor) override {
        return std::make_shared<OralDefense>("Math Exam: " + title, std::move(supervisor));
    }
};

class StudentGroup {
private:
    std::string name;
    std::vector<std::shared_ptr<Session>> sessions;
    std::vector<std::string> students;
    
public:
    explicit StudentGroup(const std::string& n) : name(n) {}
    
    void addStudent(const std::string& studentName) {
        students.emplace_back(studentName);
    }
    
    void addSession(std::shared_ptr<Session> session) {
        sessions.emplace_back(std::move(session));
    }
    
    std::vector<std::shared_ptr<Session>> checkConflicts() const {
        std::map<std::string, std::vector<std::shared_ptr<Session>>> timeMap;
        for (const auto& s : sessions) {
            timeMap[s->getTime()].push_back(s);
        }
        
        std::vector<std::shared_ptr<Session>> conflicts;
        for (const auto& pair : timeMap) {
            if (pair.second.size() > 1) {
                conflicts.insert(conflicts.end(), pair.second.begin(), pair.second.end());
            }
        }
        return conflicts;
    }

    void enroll(const std::shared_ptr<CourseFactory>& factory, const std::string& courseName,
                std::shared_ptr<Teacher> lecturer, std::shared_ptr<Teacher> assistant,
                std::shared_ptr<Teacher> supervisor) {
        
        std::cout << "\n📚 Enrolling group " << name << " in " << courseName << "..." << std::endl;
        
        try {
            auto lecture = factory->createLecture("Mon 10:00", "Auditorium 1", std::move(lecturer));
            auto practical = factory->createPractical("Wed 14:00", "Lab 3", std::move(assistant));
            auto coursework = factory->createCourseWork(courseName, std::move(supervisor));
            
            // Використовуємо .back() тільки після того, як впевнились, що об'єкти додано
            addSession(std::move(lecture));
            std::cout << "  ✓ Added: " << sessions.back()->getInfo() << std::endl;

            addSession(std::move(practical));
            std::cout << "  ✓ Added: " << sessions.back()->getInfo() << std::endl;

            std::cout << "  ✓ Assigned: " << coursework->getInfo() << std::endl;
        } catch (const std::invalid_argument& e) {
            std::cerr << "  ✗ ERROR enrolling in " << courseName << ": " << e.what() << std::endl;
        }
    }
    
    std::string getSchedule() const {
        std::stringstream ss;
        ss << "\n📅 Schedule for group " << name << ":\n";
        ss << "────────────────────────────────────────────────────\n";
        
        for (const auto& session : sessions) {
            ss << "  • " << session->getInfo() << "\n";
        }
        
        auto conflicts = checkConflicts();
        if (!conflicts.empty()) {
            ss << "\n⚠️  WARNING: " << conflicts.size() << " session(s) involved in scheduling conflict(s)!\n";
            for (const auto& conflict : conflicts) {
                ss << "      -> CONFLICT: " << conflict->getInfo() << "\n";
            }
        }
        
        return ss.str();
    }
    
    std::string getName() const { return name; }
};

int main() {
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║   University Course Scheduling System Demo            ║\n";
    std::cout << "║   Design Patterns: Factory Method & Abstract Factory  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n\n";
    
    auto drSinkevych = std::make_shared<Lecturer>("Dr. Oleh Sinkevych");
    auto drPetrenko = std::make_shared<Assistant>("Dr. Mariia Petrenko");
    auto mentor = std::make_shared<ExternalMentor>("Industry Expert Ivan");
    auto drKovalenko = std::make_shared<Lecturer>("Dr. Anna Kovalenko");
    auto msShevchenko = std::make_shared<Assistant>("Ms. Oksana Shevchenko");
    
    auto group1 = std::make_shared<StudentGroup>("FeP-21");
    group1->addStudent("Ivan Ivanov");
    group1->addStudent("Maria Petrova");
    
    auto group2 = std::make_shared<StudentGroup>("FeP-22");
    group2->addStudent("Oleh Kovalchuk");
    group2->addStudent("Anna Sydorenko");
    
    auto progFactory = std::make_shared<ProgrammingCourseFactory>();
    auto dbFactory = std::make_shared<DatabasesCourseFactory>();
    auto mathFactory = std::make_shared<MathCourseFactory>();
    
    group1->enroll(progFactory, "OOP in C++", drSinkevych, drPetrenko, mentor);
    group1->enroll(dbFactory, "SQL & NoSQL", drKovalenko, msShevchenko, drKovalenko);
    
    group2->enroll(mathFactory, "Linear Algebra", drKovalenko, msShevchenko, drKovalenko);
    group2->enroll(progFactory, "Data Structures", drSinkevych, drPetrenko, mentor);
    
    std::cout << group1->getSchedule();
    std::cout << group2->getSchedule();
    
    std::cout << "\n\n🏭 Factory Method Pattern Demo:\n";
    std::cout << "────────────────────────────────────────────────────\n";
    
    auto lectureFactory = std::make_shared<LectureFactory>();
    auto practicalFactory = std::make_shared<PracticalFactory>();
    
    auto customLecture = lectureFactory->createSession("Fri 12:00", "Room 201", drSinkevych);
    std::cout << "Created: " << customLecture->getInfo() << std::endl;
    
    try {
        auto failedLecture = lectureFactory->createSession("Fri 14:00", "Room 202", mentor);
        std::cout << "Created: " << failedLecture->getInfo() << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Caught expected error: " << e.what() << std::endl;
    }

    std::cout << "\n\n📝 CourseWork Submission Demo (OCP):\n";
    std::cout << "────────────────────────────────────────────────────\n";
    
    auto githubCW = std::make_shared<GitHubSubmission>("Final Project", mentor);
    auto onlineCW = std::make_shared<OnlineSubmission>("Database Report", drKovalenko);
    auto oralCW = std::make_shared<OralDefense>("Calculus", drKovalenko);
    
    githubCW->submit();
    onlineCW->submit();
    oralCW->submit();
    
    try {
        std::cout << "\nTesting invalid supervisor assignment:\n";
        auto failedCW = std::make_shared<GitHubSubmission>("Invalid Project", mentor);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Caught expected error: " << e.what() << std::endl;
    }

    return 0;
}
