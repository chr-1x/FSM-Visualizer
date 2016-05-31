#include <stdlib.h>
#include <stdio.h>
#include "graphgen.h"
#include "windows.h"
#include "windowsx.h"

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width; int Height;
    int Pitch;
};

struct win32_dynamic_code
{
    bool IsValid;
    HMODULE DLLHandle;
    FILETIME DLLLastWriteTime;

    update_and_render* UpdateAndRender;
};

global_variable bool GlobalRunning;
global_variable bool GlobalResized;
global_variable ivec2 GlobalMouseP;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable s64 GlobalPerfCountFrequency;

internal void
Win32GetEXEFileName(size_t EXEFileNameCapacity, char* EXEFileName, char** OnePastLastSlash)
{
    GetModuleFileNameA(0, EXEFileName, (DWORD)EXEFileNameCapacity);
    *OnePastLastSlash = EXEFileName;
    for(char *Scan = EXEFileName;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            *OnePastLastSlash = Scan + 1;
        }
    }
}

internal void
CatStrings(size_t SourceACount, char* SourceA, size_t SourceBCount, char* SourceB, size_t DestCount, char* Dest)
{
    
    for (u32 Index = 0;
        Index < Min(SourceACount, DestCount);
        ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for (u32 Index = 0;
        Index < Min(SourceBCount, DestCount - SourceACount);
        ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

internal void
Win32BuildEXEPathFileName(char* EXEFileName, char* OnePastLastSlash, 
                          char *FileName, int DestCount, char *Dest)
{
    CatStrings(OnePastLastSlash - EXEFileName, EXEFileName,
               strlen(FileName), FileName,
               DestCount, Dest);
}

internal FILETIME
Win32GetFileModifiedTime(char* Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return LastWriteTime ;
}

internal win32_dynamic_code
Win32LoadDynamicCode(char* SourceDLLName, char* TempDLLName, char* LockFileName) 
{
    win32_dynamic_code Result = {};

    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if(!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetFileModifiedTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, FALSE);
        Result.DLLHandle = LoadLibraryA(TempDLLName);
        if (Result.DLLHandle) {
            Result.UpdateAndRender = (update_and_render*)GetProcAddress(Result.DLLHandle, "UpdateAndRender");
            
            Result.IsValid = Result.UpdateAndRender && true;
        }
    }
    
    if (!Result.IsValid) {
        Result.UpdateAndRender = NULL;
    }

    return Result;
}

internal void
Win32UnloadDynamicCode(win32_dynamic_code* Library)
{
    if (Library->DLLHandle) 
    {
        FreeLibrary(Library->DLLHandle);
        Library->DLLHandle = NULL;
    }

    Library->IsValid = false;
    Library->UpdateAndRender = NULL;
}

internal ivec2
Win32GetWindowDimension(HWND Window)
{
    ivec2 Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    if ((WindowWidth >= Buffer->Width*2) && (WindowHeight >= Buffer->Height*2))
    {
        StretchDIBits(DeviceContext,
            0, 0, Buffer->Width*2, Buffer->Height*2,
            0, 0, Buffer->Width, Buffer->Height,
            Buffer->Memory,
            &Buffer->Info,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }
    else
        {
        int OffsetX = (WindowWidth - Buffer->Width) / 2;
        int OffsetY = (WindowHeight - Buffer->Height) / 2;

        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, OffsetX, WindowHeight, BLACKNESS);

        StretchDIBits(DeviceContext,
            OffsetX, OffsetY, Buffer->Width, Buffer->Height,
            0, Buffer->Height, Buffer->Width, -Buffer->Height,
            Buffer->Memory,
            &Buffer->Info,
            DIB_RGB_COLORS, // Using RGB, not palletized,
            SRCCOPY
        );
    }
}

internal void
Win32DirectoryWildcardLimit5(char* Search, int* NumberOfListedFiles, char* ListedFiles[5])
{
    *NumberOfListedFiles = 0;

    WIN32_FIND_DATA FindData = {};
    HANDLE FindHandle = FindFirstFile(Search, &FindData);

    int Found = true;
    do
    {   
        u32 Attribs = FindData.dwFileAttributes;
        if (Attribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Don't recurse
        }
        else if (FindData.cFileName[0] != '\0' && FindData.cFileName[0] != '.')
        {
            char FullFilename[2 * MAX_PATH + 1];
            CatStrings(5, "data/", strlen(FindData.cFileName), FindData.cFileName, MAX_PATH * 2, FullFilename);

            ListedFiles[(*NumberOfListedFiles)++] = _strdup(FullFilename);
        }
        Found = FindNextFile(FindHandle, &FindData);
    } while(Found != 0 && *NumberOfListedFiles < 5);

    u32 LastError = GetLastError();
    if (LastError != ERROR_NO_MORE_FILES)
    {
        OutputDebugString("Couldn't get all files");
    }

    FindClose(FindHandle);
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart)/ (f32)GlobalPerfCountFrequency);
    return Result;
}

internal char* 
ReadFileIntoCString(char* Filename)
{
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle == INVALID_HANDLE_VALUE) { return NULL; }

    LARGE_INTEGER FileSize;
    if (!GetFileSizeEx(FileHandle, &FileSize)) { return NULL; }

    u32 FileSize32 = (u32)(FileSize.QuadPart);
    char* Contents = (char*)VirtualAlloc(0, FileSize32 + 1, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!Contents) { return NULL; }

    DWORD BytesRead;
    if (ReadFile(FileHandle, Contents, FileSize32, &BytesRead, 0)) 
    {
        Contents[BytesRead] = '\0';
        return Contents;
    }
    else 
    {
        VirtualFree(Contents, 0, MEM_RELEASE);
        return NULL;
    }
}

inline void
Win32UpdateButtonState(button_state* Button, bool IsPressed)
{
    Button->WasPressed = Button->IsPressed;
    Button->IsPressed = IsPressed;
}

inline button_state
Win32PropagateButton(button_state Button)
{
    Button.WasPressed = Button.IsPressed;
    return Button;
}

internal void
Win32ProcessPendingMessages(app_input* Input)
{
    MSG Message;
    // Have to actually pull messages off the queue in order to receive them
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message) 
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;

            case WM_MOUSEWHEEL:
            {
                Input->Mouse.ScrollDelta = GET_WHEEL_DELTA_WPARAM(Message.wParam) / (f32)WHEEL_DELTA;
            } break;

            case WM_LBUTTONDOWN:
            {
                Win32UpdateButtonState(&Input->Mouse.Buttons[0], true);
            } break;

            case WM_LBUTTONUP:
            {
                Win32UpdateButtonState(&Input->Mouse.Buttons[0], false);
            } break;

            case WM_MBUTTONDOWN:
            {
                Win32UpdateButtonState(&Input->Mouse.Buttons[1], true);
            } break;

            case WM_MBUTTONUP:
            {
                Win32UpdateButtonState(&Input->Mouse.Buttons[1], false);
            } break;

            case WM_RBUTTONDOWN:
            {
                Win32UpdateButtonState(&Input->Mouse.Buttons[2], true);
            } break;

            case WM_RBUTTONUP:
            {
                Win32UpdateButtonState(&Input->Mouse.Buttons[2], false);
            } break;

            case WM_MOUSEMOVE:
            {
                if (Message.wParam & MK_LBUTTON) {
                    Win32UpdateButtonState(&Input->Mouse.Buttons[0], true);
                }

                if (Message.wParam & MK_MBUTTON) {
                    Win32UpdateButtonState(&Input->Mouse.Buttons[1], true);
                }

                if (Message.wParam & MK_RBUTTON) {
                    Win32UpdateButtonState(&Input->Mouse.Buttons[2], true);
                }

                ivec2 Dimension = Win32GetWindowDimension(Message.hwnd);

                Input->Mouse.P.X = GET_X_LPARAM(Message.lParam);
                // Flip y so that the game doesn't have to think about anything
                // in y+down coordinates
                Input->Mouse.P.Y = Dimension.Height - GET_Y_LPARAM(Message.lParam);

                Input->Mouse.P = Input->Mouse.P - Dimension/2;
            } break;

            case WM_KEYDOWN:
            {
                if (Message.wParam == VK_SPACE ||
                    Message.wParam == VK_RIGHT ||
                    Message.wParam == VK_LEFT) {
                    Win32UpdateButtonState(&Input->ResetButton, true);
                }
            } break;

            case WM_KEYUP:
            {
                if (Message.wParam == VK_SPACE ||
                    Message.wParam == VK_RIGHT ||
                    Message.wParam == VK_LEFT) {
                    Win32UpdateButtonState(&Input->ResetButton, false);
                }
            }

            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

LRESULT CALLBACK 
MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam) 
{
    LRESULT Result = 0;
    switch (Message)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;

        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            ivec2 Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        case WM_SIZE:
        {
            ivec2 WindowDim = Win32GetWindowDimension(Window);
            if (WindowDim.X != 0 && WindowDim.Y != 0) {
                Win32ResizeDIBSection(&GlobalBackbuffer, WindowDim.X, WindowDim.Y);
                GlobalResized = true;
            }
        } break;

        default:
        {
            // Use default windows callback, to handle cases we don't care about
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}


int CALLBACK 
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    ShowCode, CommandLine, PrevInstance; // warning 4100
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    UINT DesiredSchedulerMS = 1;
    bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    char EXEFileName[MAX_PATH];
    char* OnePastLastSlash;
    Win32GetEXEFileName(MAX_PATH, (char*)EXEFileName, &OnePastLastSlash);

    char SourceDynamicCodeDLL[MAX_PATH];
    Win32BuildEXEPathFileName(EXEFileName, OnePastLastSlash, 
                              "graphgen.dll", sizeof(SourceDynamicCodeDLL), SourceDynamicCodeDLL);

    char TempDynamicCodeDLL[MAX_PATH];
    Win32BuildEXEPathFileName(EXEFileName, OnePastLastSlash, 
                              "graphgen_temp.dll", sizeof(TempDynamicCodeDLL), TempDynamicCodeDLL);

    char LockFileName[MAX_PATH];
    Win32BuildEXEPathFileName(EXEFileName, OnePastLastSlash, 
                              "lock.tmp", sizeof(LockFileName), LockFileName);

    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.lpszClassName = "TestbedWindowClass";
    WindowClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowEx(
            0, //WS_EX_TOPMOST|WS_EX_LAYERED
            WindowClass.lpszClassName,
            "Nodegraph generator",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0, // top level window
            0, Instance, NULL
        );
        if (Window)
        {
            ivec2 WindowDim = Win32GetWindowDimension(Window);
            Win32ResizeDIBSection(&GlobalBackbuffer, WindowDim.X, WindowDim.Y);

            HDC DeviceContext = GetDC(Window);

            app_memory AppMemory = {};
            AppMemory.PermanentSize = Megabytes(10);
            AppMemory.TemporarySize = Megabytes(100);
            AppMemory.PermanentBlock = VirtualAlloc((LPVOID)Terabytes(2), AppMemory.PermanentSize + AppMemory.TemporarySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            AppMemory.TemporaryBlock = (u8*)AppMemory.PermanentBlock + AppMemory.PermanentSize;

            if (AppMemory.PermanentBlock && AppMemory.TemporaryBlock) 
            {
                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();
                f32 ScreenUpdateHz = (60.0f / 1.0f);
                f32 TargetSecondsPerFrame = 1.0f / (f32)ScreenUpdateHz;
                
                AppMemory.IsInitialized = true;

                win32_dynamic_code App = Win32LoadDynamicCode(SourceDynamicCodeDLL, TempDynamicCodeDLL, LockFileName);

                app_input NewInput = {};
                app_input OldInput = {};

                char* Filenames[5];
                Win32DirectoryWildcardLimit5("data/*.nfa", &AppMemory.NFAFileCount, Filenames);
                for (int i = 0; i < AppMemory.NFAFileCount; ++i)
                {
                    AppMemory.NFAFiles[i] = ReadFileIntoCString(Filenames[i]);
                }

                AppMemory.TTFFile = (u8*)ReadFileIntoCString("data/font.ttf");

                GlobalRunning = true;
                while(GlobalRunning)
                {
                    NewInput = {};
                    NewInput.Mouse.P = OldInput.Mouse.P;
                    for (int MouseButton = 0; 
                        MouseButton < ArrayCount(NewInput.Mouse.Buttons); 
                        ++MouseButton)
                    {
                        NewInput.Mouse.Buttons[MouseButton] = Win32PropagateButton(OldInput.Mouse.Buttons[MouseButton]);
                    }
                    NewInput.ResetButton = Win32PropagateButton(OldInput.ResetButton);

                    FILETIME NewDLLWriteTime = Win32GetFileModifiedTime(SourceDynamicCodeDLL);
                    if(CompareFileTime(&NewDLLWriteTime, &App.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadDynamicCode(&App);
                        App = Win32LoadDynamicCode(SourceDynamicCodeDLL,
                                                 TempDynamicCodeDLL,
                                                 LockFileName);

                        NewInput.Reloaded = true;
                    }
                    else { NewInput.Reloaded = false; }
    
                    Win32ProcessPendingMessages(&NewInput);

                    if (GlobalResized) {
                        NewInput.Resized = true;
                        GlobalResized = false;
                    }

                    //TODO(chronister): Actual timing!
                    NewInput.dt = TargetSecondsPerFrame;

                    bitmap Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Width = GlobalBackbuffer.Width; 
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Stride = GlobalBackbuffer.Pitch; 
                    Buffer.BytesPerPixel = GlobalBackbuffer.Pitch / GlobalBackbuffer.Width; 
                    App.UpdateAndRender(&AppMemory, &Buffer, &NewInput);

                    LARGE_INTEGER WorkCounter = Win32GetWallClock();
                    f32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                    f32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if (SecondsElapsedForFrame < TargetSecondsPerFrame) 
                    {
                        if (SleepIsGranular) {
                            DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                            if (SleepMS > 0) 
                            {
                                Sleep(SleepMS);
                            }
                        }

                        f32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());

                        if (TestSecondsElapsedForFrame < TargetSecondsPerFrame) 
                        {
                            //TODO(handmade): LOG MISSED SLEEP HERE
                        }

                        while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        }
                    }
                    else 
                    {
                        //TODO(handmade): Missed frame rate!
                        //TODO(handmade): Logging
                    }

                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    LastCounter = EndCounter;

                    ivec2 Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                               Dimension.Width, Dimension.Height);
                    
                    OldInput = NewInput;
                }
            }
        }
    }

    DeleteFile("dracogen_temp.dll");
}

