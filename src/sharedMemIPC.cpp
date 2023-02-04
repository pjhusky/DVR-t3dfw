#include "sharedMemIPC.h"

#include "external/simdb/simdb.hpp" // only include once!!!

SharedMemIPC::SharedMemIPC( const std::string& name, const uint32_t smBlockSizeInBytes, const uint32_t smNumBlocks ) {
    mpHandle = new simdb( name.c_str(), smBlockSizeInBytes, smNumBlocks );

    auto *const pSharedMem = ptrSharedMem();

    auto sharedMemFiles = simdb_listDBs();

    pSharedMem->put( "lock free", "is the way to be" );

    const auto queriedSmVal = pSharedMem->get( "lock free" );
}

SharedMemIPC::~SharedMemIPC() {
    auto *const pSharedMem = ptrSharedMem();
    pSharedMem->close();
    delete mpHandle;
    mpHandle = nullptr;
}

const std::vector<std::string> SharedMemIPC::listSharedMemFiles() {
    const auto sharedMemFiles = simdb_listDBs();
    return sharedMemFiles;
}

int64_t SharedMemIPC::put( const std::string& key, const std::string& value ) {
    return ptrSharedMem()->put( key, value );
}

int64_t SharedMemIPC::put( const std::string& key, const uint8_t *const pValues, const uint32_t valueByteCount ) {
    return ptrSharedMem()->put(key.data(), (uint32_t)key.length(), pValues, (uint32_t)(valueByteCount) );
}

const std::string SharedMemIPC::get( const std::string& key ) {
    return ptrSharedMem()->get( key );
}

const bool SharedMemIPC::get( const std::string& key, void* const pOutValues, uint32_t valueByteCount, uint32_t* const bytesRead ) {
    return ptrSharedMem()->get( key.c_str(), static_cast<simdb::u32>( key.length() ), pOutValues, valueByteCount, bytesRead );
}


