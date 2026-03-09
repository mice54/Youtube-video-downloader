#include <stdio.h>
#include <raylib.h>
//Since raylib does not work with windows.h we need to include a lot some other stuffs and define architecture
#define _AMD64_
#include <processthreadsapi.h>
#include <namedpipeapi.h>
#include <handleapi.h>
#include <ioringapi.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "tinyfiledialogs.h"

int main(){
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(800, 600, "Youtube video downloader");
    char url[1024] = "Enter your url";
    bool urlActive = false;
    char *file = "Select file to save";
    bool fileActive = false;
    bool userSelectFile = false;
    HANDLE readPipe = NULL;
    PROCESS_INFORMATION pi = {0};
    float percentage = 0;
    char speed[14] = {0};
    while (!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(WHITE);
        if (GuiTextBox(CLITERAL(Rectangle){
                0, 0, 400, 20
        }, url, 1024, urlActive)){
            urlActive ^= 1;
        }
        if (GuiTextBox(CLITERAL(Rectangle){
                0, 25, 375, 20
        }, file, 1024, fileActive)){
            fileActive ^= 1;
        }
        if(strcmp(file, "Select file to save") != 0) {
            userSelectFile = true;
        }
        if (GuiButton(CLITERAL(Rectangle){ 380, 25, 20, 20 },
                      GuiIconText(ICON_FILE_OPEN, ""))){
            const char *patterns[] = {
                    "*.mp4",
                    "*.avi"
            };
            file = tinyfd_saveFileDialog(
                    "Select video", /* NULL or "" */
                    file, /* NULL or "" , ends with / to set only a directory */
                    1, /* 0 (2 in the following example) */
                    patterns, /* NULL or char const * lFilterPatterns[2]={"*.png","*.jpg"}; */
                    "video files"); /* NULL or "image files" */
            userSelectFile = true;
        }
        GuiDrawText(TextFormat("Download speed %s", speed), CLITERAL(Rectangle){10, 75, 200, 20}, TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP, BLACK);
        GuiProgressBar(CLITERAL(Rectangle){10, 100, 200, 20}, "", "", &percentage, 0, 100);
        {
            char buffer[4096];
            DWORD read;
            if (readPipe && ReadFile(readPipe, buffer, sizeof(buffer) - 1, &read, NULL)){
                // '[download]   1.4% of  147.01MiB at    4.42MiB/s ETA 00:32'
                if(*(buffer+1) != 'd')continue;
                char * buf = buffer;
                buffer[read] = 0;
                buf += strlen("[download] ");
                //while(isspace(*buf))buf++;
                percentage = strtof(buf, NULL);
                while(*buf != 'a')buf++;
                buf += 2; // 'at ...' -> '...'
                while(isspace(*buf))buf++;
                memset(speed, 0, 14);
                for(int i = 0; i < 14 && !isspace(*buf); ++i, ++buf)
                    speed[i] = *buf;
            }
        }
        DWORD result = WaitForSingleObject(pi.hProcess, 0);
        if (result == WAIT_OBJECT_0) {
            CloseHandle(readPipe);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            readPipe = NULL;
            memset(&pi, 0, sizeof(pi));
            memset(speed, 0, 14);
        }
        if (userSelectFile){
            if (GuiButton(CLITERAL(Rectangle){ 405, 00, 140, 20 }, "Download video")){
                if (system("yt-dlp --version >nul 2>&1") != 0){
                    if (system("python -m pip --version >nul 2>&1") != 0){
                        fprintf(stderr, "please download python\n");
                    } else{
                        if (system("python -m pip install yt-dlp") != 0){
                            fprintf(stderr, "failed to install yt-dlp. Check your internet connection\n");
                        }
                    }
                }else{
                    HANDLE writePipe;
                    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

                    CreatePipe(&readPipe, &writePipe, &sa, 0);
                    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

                    STARTUPINFOA si = {0};

                    si.cb = sizeof(si);
                    si.dwFlags = STARTF_USESTDHANDLES;
                    si.hStdOutput = writePipe;
                    si.hStdError = writePipe;
                    si.hStdInput = NULL;

                    char *cmd = TextFormat("yt-dlp --newline --progress -f mp4 -o \"%s\" \"%s\"", file, url);

                    if(!CreateProcessA(
                            NULL,
                            cmd,
                            NULL,
                            NULL,
                            TRUE,
                            CREATE_NO_WINDOW,
                            NULL,
                            NULL,
                            &si,
                            &pi
                    )){
                        printf("CreateProcess failed\n");
                        return 1;
                    }

                    CloseHandle(writePipe);
                }
            }
       }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}
