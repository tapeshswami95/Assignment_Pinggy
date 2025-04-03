#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <sstream>
#include <mutex>
#include <map>

using namespace std;

struct Job {
    int jobType;
    int jobDuration;
};

class ThreadPool {
public:
    ThreadPool() : stop(false) {}

    ~ThreadPool() {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
        lock.unlock();
        condition.notify_all();
        // Join all worker threads
        for (auto& entry : workers) {
            for (auto& t : entry.second) {
                if (t.joinable()) {
                    t.join();
                }
            }
        }
    }

    void createWorkers(const vector<pair<int, int>>& workerConfig) {
        for (const auto& config : workerConfig) {
            int workerType = config.first;
            int count = config.second;
            for (int i = 0; i < count; ++i) {
                workers[workerType].push_back(thread(&ThreadPool::workerFunction, this, workerType));
            }
        }
    }

    void distributeJobs() {
        string line;
        while (getline(cin, line)) {
            stringstream ss(line);
            int jobType, jobDuration;
            ss >> jobType >> jobDuration;
            unique_lock<mutex> lock(queueMutex);
            jobs.push({jobType, jobDuration});
            lock.unlock();
            condition.notify_one();
        }
    }

private:
    map<int, vector<thread>> workers;
    queue<Job> jobs;
    mutex queueMutex;
    condition_variable condition;
    bool stop;


    void workerFunction(int workerType) {
        while (!stop) {
            Job job;
            unique_lock<mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !jobs.empty() || stop; });
            if (stop && jobs.empty()) {
                return;
            }
            job = jobs.front();
            jobs.pop();
            lock.unlock();

            if (job.jobType == workerType) {
                cout << "Worker " << workerType << " processing job of type " << job.jobType << " with duration " << job.jobDuration << " seconds." << endl;
                this_thread::sleep_for(chrono::seconds(job.jobDuration));
                cout << "Worker " << workerType << " finished processing job of type " << job.jobType << "." << endl;
            } else {
                unique_lock<mutex> lock(queueMutex);
                jobs.push(job);
                lock.unlock();
                condition.notify_one();
            }
        }
    }
};

int main() {
    ThreadPool pool;
    cout<<"created a Pool"<<std::endl;
    vector<pair<int, int>> workerConfig;
    string line;
    for (int i = 0; i < 5; ++i) {
        getline(cin, line);
        stringstream ss(line);
        int workerType, workerCount;
        ss >> workerType >> workerCount;
        workerConfig.push_back({workerType, workerCount});
    }


    pool.createWorkers(workerConfig);
    
    cout<<"worker configration is done"<<endl;
  
    pool.distributeJobs();

    return 0;
}
