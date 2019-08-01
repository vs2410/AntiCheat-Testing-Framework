#pragma once
#include <windows.h>
#include <iostream>


//Function to get only file name from full path name
std::string getFileName(std::string file)
{
    size_t index;
    for (index = file.size(); index > 0; index--)
    {
        if (file[index - 1] == '\\' || file[index - 1] == '/')
            break;
    }
    return file.substr(index);
}

/*
Log print format:

[debug][Jul 15 2019,  09:18:34]: CheatHelper.cpp:22:: <MSG> 

*/
#define LOG_DEBUG_MSG(MSG)\
    std::cout << "[debug][" << __DATE__ << ",  " << __TIME__ << "]: "<< \
    getFileName(__FILE__) <<":" << __LINE__ << ":: " << MSG << std::endl;


/*
Log print format:

[error][Jul 15 2019,  09:18:34]: CheatHelper.cpp:22:: [-] <MSG>: A1 

*/

#define LOG_ERROR_MSG(MSG)\
        std::cout << "[error][" << __DATE__ << ",  " << __TIME__ << "]: "<< \
        getFileName(__FILE__) <<":" << __LINE__ << ":: " << "[-] " << MSG << ": " << std::dec << \
        GetLastError() << std::endl;



