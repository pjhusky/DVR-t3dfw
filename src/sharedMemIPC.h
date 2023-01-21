#ifndef _SHARED_MEM_IPC_H_B0FC681C_DB0A_4B07_BB89_276144A875DF
#define _SHARED_MEM_IPC_H_B0FC681C_DB0A_4B07_BB89_276144A875DF

#include <string>
#include <vector>

#include <stdint.h>

class simdb;

struct SharedMemIPC {
    SharedMemIPC( const std::string& name, const uint32_t smBlockSizeInBytes = 1024u, const uint32_t smNumBlocks = 4096u );
    ~SharedMemIPC();
    
    static const std::vector<std::string> listSharedMemFiles();

    int64_t put( const std::string& key, const std::string& value);
    const std::string get( const std::string& key);

private:
    auto *const ptrSharedMem() { return reinterpret_cast< simdb *const >( mpHandle ); }

    void *mpHandle;
};

#endif // _SHARED_MEM_IPC_H_B0FC681C_DB0A_4B07_BB89_276144A875DF