#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

class threadPool {
public:
    /**
     * @brief Construct a new thread Pool object
     * with given number of threads to take jobs. 
     *
     * @param num_threads 
     */
    threadPool(size_t num_threads){
        pool_.resize(num_threads);
        cnt_busy_ = (int)num_threads;
        for (uint32_t i = 0; i < num_threads; i++) {
            // each thread will run threadLoop function
            pool_.at(i) = std::thread(&threadPool::threadLoop,this);
        }
    }

    /**
     * @brief Destroy the thread Pool object.
     * Called when object falls out of scope.
     */
    ~threadPool(){
        // Acquire lock to change terminate variable
        {
            std::unique_lock<std::mutex> l(m_);
            terminate_ = true;
        }
        // Notify all threads of the change
        cv_.notify_all();
        // Join all threads
        for (std::thread& active_thread : pool_)
            active_thread.join();
        pool_.clear();
    }

    /**
     * @brief When the variable flips to true, all
     * threads are terminated. Can be used to race
     * multiple threads against each other and stop
     * once the first one completes.
     * 
     * @param stop 
     */
    void terminate_on_flag(std::atomic<bool>  &stop){
      while (!stop.load()){}
      terminate_ = true;
      std::unique_lock<std::mutex> l{m_};
      cv_.notify_all();
    }

    void add_job(const std::function<void()>& job);
    bool is_queue_empty();
    bool pool_busy();
    int jobs_in_queue(){
        std::unique_lock<std::mutex> l{m_};
        return queue_.size();
    }
    int threads_busy(){
        return cnt_busy_.load();
    }

private:
    void threadLoop();

    bool terminate_ = false;                  // Tells threads to stop looking for jobs
    std::mutex m_;                            // Prevents data races to the job queue
    std::condition_variable cv_;              // Allows threads to wait on new jobs or termination 
    std::vector<std::thread> pool_;           // A pool to hold all threads
    std::queue<std::function<void()>> queue_; // A queue to hold incoming jobs
    std::atomic<int> cnt_busy_;               // A counter to keep track of sleeping/active threads
};

#endif
