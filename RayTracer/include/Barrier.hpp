
#ifndef _BARRIER_HPP_
#define _BARRIER_HPP_

#include <mutex>
#include <condition_variable>
#include <stdexcept>

class Barrier {

public:
    
    explicit Barrier(unsigned int c) :
        threshold(c), count(c), generation(0) {

            if (c == 0)
                throw std::invalid_argument(" barrier count cannot be zero."); 
    } 


    bool wait() {

        std::unique_lock<std::mutex> lock(mtx);

        unsigned int gen = generation;

        if (--count == 0) {

            ++generation;
            count = threshold;
            cond.notify_all();
            return true;
        }

        while (gen == generation) {
            cond.wait(lock);
        }

        return false;

        

    }



private:

    std::mutex mtx;
    std::condition_variable cond;
    unsigned int threshold;
    unsigned int count;
    unsigned int generation;

};





#endif