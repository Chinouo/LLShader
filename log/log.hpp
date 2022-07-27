#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <string>

namespace LLShader{

    class LogUtil{
    public:

        static void LogD(const std::string& message){
            std::cout << prefix << "(D): " << message;
        }

        static void LogE(const std::string& message){
            std::cerr << prefix << "(E): " <<  message;
        }
        
        static void LogW(const std::string& message){
            std::cout << prefix << "(W): " << message;
        }

        static void LogI(const std::string& message){
            std::cout << prefix << "(I): " << message;
        }
    private:
        inline static const std::string prefix = "LLShader";

    };



}



#endif