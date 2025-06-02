#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;
#include "imgui/imgui.h"
#include <OpenGL/gl.h>

#define ENSURE_MAIN_THREAD() \
    do { \
        static const auto mainThreadId = std::this_thread::get_id(); \
        assert(std::this_thread::get_id() == mainThreadId && "ImGui used from non-main thread!"); \
    } while(0)


class Log {
    private:
        int logLevel;
        ofstream logFile;
        vector<string> logMessages;
        std::mutex logMutex;



    public:
        enum Level {
            DEBUG = 0,
            INFO = 1,
            WARNING = 2,
            ERROR = 3
        };
        Log(){
            logLevel = INFO;
        }

        ~Log(){
            if (logFile.is_open()) {
                logFile.close();
            }
        }

        void initialize(int level, const string& filename = "log.txt")
        {
            logLevel = level;
            logFile.open(filename, ios::out | ios::app);
            if (!logFile) {
                cerr << "Error opening log file." << endl;
                return;
            }else{
                log("INFO", "Log initialized");
            }

        }

        void log(const string& level, string message){
            std::lock_guard<std::mutex> lock(logMutex);
            string fullMessage = "[ " + level + " ] " + message;
            logMessages.push_back(fullMessage);
            if (logFile.is_open()) {
                logFile << fullMessage << endl;
            }
            cout << fullMessage << endl;


        }
        void setLevel(int level, const string& message){
            // std::lock_guard<std::mutex> lock(logMutex);
            if (level >= logLevel){
                switch(level){
                    case DEBUG:
                        log("DEBUG", message);
                        break;
                    case INFO:
                        log("INFO", message);
                        break;
                    case WARNING:
                        log("WARNING", message);
                        break;
                    case ERROR:
                        log("ERROR", message);
                        break;

                }

            }

        }
        void displayLog(){
            ENSURE_MAIN_THREAD();

            ImGui::Begin("Log");

            if (ImGui::Button("Clear")) {
                ImGui::Separator();
            }

            ImGui::Separator();

            for (const auto& message : logMessages) {
                ImGui::TextUnformatted(message.c_str());
            }
            ImGui::End();
        }


        void checkOpenGLError(const std::string& tag) {
            GLenum err;
            while ((err = glGetError()) != GL_NO_ERROR) {
                std::stringstream ss;
                ss << "[OpenGL ERROR] " << err << " in " << tag;
                        log("ERROR", ss.str());  // Store string safely
            }
        }

};



#endif // LOG_HPP
