#include <vector>
#include <iostream>
#include <cstdint>
#include <malloc.h>
#include <assert.h>

class SomeTest {
public:
    static int i;
    SomeTest() { 
        std::cout << "STATIC CTOR " << i << std::endl;
    }
    // this should do SOMETHING, and it seems
    // to never bbe hit
    ~SomeTest() {
        printf("NO CLUE");
        ++i;
        std::cout << "STATIC DTOR " << --i << std::endl;
    }
};

int SomeTest::i = 0;

static SomeTest test;

static void ctor_(void) __attribute__((constructor));
static void ctor_(void) {
    test.i++;
    std::cout << "C CTOR FUNC" << std::endl;
}

static void dtor_(void) __attribute__((destructor));
static void dtor_(void) {
    test.i--;
    std::cout << "C DTOR FUNC " << std::endl;
}

static void exit_(void) { 
    std::cout << "ATEXIT CALLD " << std::endl;
}

//KOS_INIT_FLAGS(INIT_MALLOCSTATS);

void new_handler_cb() {
    std::cout << "new_handler callback invoked!" << std::endl;

    malloc_stats();

    // Unregister ourself as the new handler, so that next 
    // iteration will hit the exception handler.
    std::set_new_handler(NULL);
}

int main(int argc, char **argv) {
    std::vector<uint8_t> bytes;
    bool failed_once = false;
    atexit(exit_);

    // Sets the global, static C++ handler for when calls to new fail
    // this can be used without exceptions enabled!
    std::set_new_handler(new_handler_cb);

    std::cout << "Beginning out-of-memory demonstration." << std::endl;

    while(true) {
        try {
            // Just keep adding bytes until something bad happens!
            bytes.push_back(0xff);

        } catch(std::bad_alloc const&) {

            if(!failed_once) { 
                // std::bad_alloc is thrown when a call to new fails
                std::cout << "Caught std::bad_alloc! Current size: " 
                          << static_cast<double>(bytes.capacity()) 
                                / 1024.0 / 1024.0 << "MB" 
                          << std::endl;
                          
                // Remember, std::vector typically requests RAM in 
                // powers-of-two, so the actual requested allocation
                // was probably 2x the size of the vector.
                
                // Lets force the vector to shrink to free up some
                // space and ensure we can continue to allocate.
                
                malloc_stats();
                
                bytes.resize(0);
                bytes.shrink_to_fit();

                failed_once = true;
            } else {
                break;
            }
        }
    }

    std::cout << "All done. Thank you for the RAM!" << std::endl;

    return !failed_once;
}
