#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;
#include "imgui/imgui.h"
#include <OpenGL/gl.h>


class Log {
    private:
        int logLevel;
        ofstream logFile;
        vector<string> logMessages;

    
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
            string fullMessage = "[ " + level + " ] " + message;
            logMessages.push_back(fullMessage);
            if (logFile.is_open()) {
                logFile << fullMessage << endl;
            }
            cout << fullMessage << endl;

            
        }
        void setLevel(int level, const string& message){
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


        void checkOpenGLError(const std::string& context) {
            GLenum err;
            while ((err = glGetError()) != GL_NO_ERROR) {
                std::string errorMessage = "OpenGL error: " + std::to_string(err) + " in " + context;
                log("ERROR", errorMessage);
            }
        }

};



#endif // LOG_HPP