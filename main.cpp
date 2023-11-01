// a3.cpp

/////////////////////////////////////////////////////////////////////////
//
// Student Info
// ------------
//
// Name : <Gilbert Wong>
// St.# : <301308094>
// Email: <gsw6@sfu.ca>
//
//
// Statement of Originality
// ------------------------
//
// All the code and comments below are my own original work. For any non-
// original work, I have provided citations in the comments with enough
// detail so that someone can see the exact source and extent of the
// borrowed work.
//
// In addition, I have not shared this work with anyone else, and I have
// not seen solutions from other students, tutors, websites, books,
// etc.
//
/////////////////////////////////////////////////////////////////////////

//
// Do not #include any other files!
//
#include "Announcement.h"
#include "JingleNet_announcer.h"
#include "Queue_base.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;
class AnnouncementNode {
public:
    Announcement* data;
    AnnouncementNode* next;

    AnnouncementNode(Announcement* announcement) : data(announcement), next(nullptr) {}
};

class Queue : public Queue_base<Announcement> {
private:
    AnnouncementNode* head;
    AnnouncementNode* tail;
    int queueSize = 0;
public:
    Queue() : head(nullptr), tail(nullptr) {}

    //Getter
    AnnouncementNode* getHead() const {
        return head;
    }

    //Returns size in o(1), keeps running total of all nodes in the queue across all priority levels
    int size() const override {
        return queueSize;
    }

    //Iterates through the priority level and gets the size for that priority level, only used for announce
    //Completely separate from int size()
    int prioritySize() const {
        int count = 0;
        AnnouncementNode* curr = head;
        while (curr) {
            count++;
            curr = curr->next;
        }
        return count;
    }

    void enqueue(const Announcement& announcement) override {
        AnnouncementNode* newNode = new AnnouncementNode(new Announcement(announcement));
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
      queueSize++;
    }

    const Announcement& front() const override {
        if (!head) {
            throw runtime_error("front: queue is empty");
        }
        return *head->data;
    }

    void dequeue() override {
        if (!head) {
            throw runtime_error("dequeue: queue is empty");
        }

        AnnouncementNode* temp = head;
        head = head->next;
        delete temp->data;
        delete temp;
        queueSize--;
        if (!head) {
            tail = nullptr;
        }
    }
    //Removes every node that has the matches the sendername
    void removeAllOfUser(const string& user) {
        AnnouncementNode* curr = head;
        AnnouncementNode* prev = nullptr;

        while (curr) {
            if (curr->data->get_sender_name() == user) {
                AnnouncementNode* toDelete = curr;
                if (prev) {
                    prev->next = curr->next;
                } else {
                    head = curr->next;
                }
                curr = curr->next;

                delete toDelete->data;
                delete toDelete;
                queueSize--; //Decrement total size for every node removed
            } else {
                prev = curr;
                curr = curr->next;
            }
        }
    }
    //Returns the head of the specific queue
    const Announcement& peek() const {
        if (head) {
            return *head->data;
        } else {
            throw runtime_error("Attempted to peek an empty queue.");
        }
    }

    //Helper Method for dequeueing the specific node since a copy that has been promoted is enqueued
    void dequeue_specific(AnnouncementNode* node) {
        if (!node) return; 

        if (node == head) {
            head = head->next;
            if (!head) tail = nullptr;   
            delete node->data;
            delete node;
            queueSize--;
            return;
        }

        AnnouncementNode* curr = head;
        AnnouncementNode* prev = nullptr;
        while (curr && curr != node) {
            prev = curr;
            curr = curr->next;
        }

        if (curr == node) { // found the node
            prev->next = curr->next;
            if (tail == node) tail = prev;  
            delete node->data;
            delete node;
            queueSize--;
        }
    }

    ~Queue() {
        while (head) {
            AnnouncementNode* temp = head;
            head = head->next;
            delete temp->data;
            delete temp;
        }
    }
};

class JingleNet {
private:
    Queue queues[5]; // 5 Priority levels - one queue for each priority level

public:
    JingleNet() {}

    // Sends an announcement to the correct queue based on its rank.
    void send(const Announcement& announcement) {
      int priorityIndex = static_cast<int>(announcement.get_rank()) - 1;
      queues[priorityIndex].enqueue(announcement);
    }

    // Announces (dequeues) the next announcement from the highest priority queue.
    void announce() {
        for (int i = 4; i >= 0; i--) { // Start from highest priority queue
            if (queues[i].prioritySize() > 0) {
                Announcement ann = queues[i].front();

                //JingleNet passes announcenment (ann) to announce method from JingleNet_announcer.h
                jnet.announce(ann);
                queues[i].dequeue();
                return; // Exit after announcing one announcement.
            }
        }
    }

    void remove_all(const string& user) {
        for (int i = 4; i >= 0; i--) { 
            queues[i].removeAllOfUser(user);
        }
    }

    //Helper method, changes the rank of the user to the next level
    //Consulted with chatGPT on using switch cases in a helper method compared to direct logic in method
    Announcement set_higher_rank(const Announcement& ann) {
        // Fetch current rank of the announcement
        Rank currentRank = ann.get_rank();

        // Determine the next rank
        Rank higherRank;
        switch (currentRank) {
            case Rank::SNOWMAN: 
              higherRank = Rank::ELF1; break;
            case Rank::ELF1: 
              higherRank = Rank::ELF2; break;
            case Rank::ELF2: 
              higherRank = Rank::REINDEER; break;
            case Rank::REINDEER: 
              higherRank = Rank::SANTA; break;
            case Rank::SANTA: break; //Nothing since SANTA is highest rank
        }

        // Create a new announcement with the higher rank
        Announcement newAnn(ann.get_sender_name(), higherRank, ann.get_text());
        return newAnn;
        }

    void promote_announcements(const string& user) {
        for (int i = 3; i >= 0; i--) {
            AnnouncementNode* current = queues[i].getHead(); 
            while (current) {
                if (current->data->get_sender_name() == user) {

                    // Create a new announcement with a higher rank
                    Announcement higherRankedAnn = set_higher_rank(*(current->data));

                    // Enqueue the new announcement to the higher rank queue
                    queues[i+1].enqueue(higherRankedAnn); 

                    // Move to the next node
                    AnnouncementNode* toDelete = current;
                    current = current->next;

                    // Dequeue the original announcement (since it is promoted) from its current queue
                    queues[i].dequeue_specific(toDelete); 
                } else {
                    current = current->next;
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    cout << "Welcome to Assignment 3!" << endl;
    JingleNet jingleNet;

    if (argc < 2) {
      cerr << "Usage: " << argv[0] << " <filename>" << endl;
      return 1;
    }
    string filename = argv[1];
    
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Failed to open the file." << endl;
        return 1;
    }

    //This entire block was written with the help of StackOverflow explanations after googling:
    //"Syntax for reading through a file line by line"
    //After googling and reviewing StackOverflow, used chatGPT for syntax and debugging
    string line;
    while (getline(inputFile, line)) {
        if (line.find("SEND") != string::npos) {
            size_t pos = line.find(' ');
            string sender_name = line.substr(pos + 1, line.find(' ', pos + 1) - (pos + 1));

            pos = line.find(' ', pos + 1); // Move to the next space
            string rank_str = line.substr(pos + 1, line.find(' ', pos + 1) - (pos + 1));

            pos = line.find(' ', pos + 1); // Move to the next space
            string text = line.substr(pos + 1);

            Rank rank = to_rank(rank_str);

            Announcement announcement(sender_name, rank, text);
            jingleNet.send(announcement);
        } else if (line.find("ANNOUNCE ") != string::npos) {
            size_t pos = line.find(' ');
            
            //Googled how to read a string as an integer
            //https://www.udacity.com/blog/2021/05/the-cpp-stoi-function-explained.html
            int num = stoi(line.substr(pos + 1)); // Convert to integer
            for (int i = 0; i < num; ++i) {
                jingleNet.announce();
            }
        } else if (line.find("REMOVE_ALL") != string::npos) {
            size_t pos = line.find(' ');
            string username = line.substr(pos + 1);

            jingleNet.remove_all(username);
        }
          else if (line.find("PROMOTE_ANNOUNCEMENTS") != string::npos) {
            size_t pos = line.find(' ');
            string username = line.substr(pos + 1);
            jingleNet.promote_announcements(username);
        }     
    }

    inputFile.close();
    return 0;
}