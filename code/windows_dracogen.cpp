#include "dracogen.h"
#include "windows.h"
#include "windows_dracogen.h"

global_variable bool GlobalRunning;
global_variable ivec2 GlobalMouseP;
global_variable win32_offscreen_buffer GlobalBackbuffer;

internal void
Win32GetEXEFileName(size_t EXEFileNameCapacity, char* EXEFileName, char** OnePastLastSlash)
{
    DWORD SizeOfFilename = GetModuleFileNameA(0, EXEFileName, (DWORD)EXEFileNameCapacity);
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
Win32BuildEXEPathFileName(char* EXEFileName, char* OnePastLastSlash, 
                          char *FileName, int DestCount, char *Dest)
{
    CatStrings(OnePastLastSlash - EXEFileName, EXEFileName,
               StringLength(FileName), FileName,
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

internal win32_game_code
Win32LoadGameCode(char* SourceDLLName, char* TempDLLName, char* LockFileName) 
{
    win32_game_code Result = {};

    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if(!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetFileModifiedTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, FALSE);
        Result.DLLHandle = LoadLibraryA(TempDLLName);
        if (Result.DLLHandle) {
            Result.UpdateAndRender = (game_update_and_render*)GetProcAddress(Result.DLLHandle, "GameUpdateAndRender");
            
            Result.IsValid = Result.UpdateAndRender && true;
        }
    }
    
    if (!Result.IsValid) {
        Result.UpdateAndRender = NULL;
    }

    return Result;
}

internal void
Win32UnloadGameCode(win32_game_code* Library)
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
    // TODO(casey): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;

    // NOTE(casey): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // NOTE(casey): Thank you to Chris Hecker of Spy Party fame
    // for clarifying the deal with StretchDIBits and BitBlt!
    // No more DC for us.
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;

    // TODO(casey): Probably clear this to black
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
{
#if 0
    rect Window = {};
    Window.Left = Window.Top = 0;
    Window.Right = WindowWidth;
    Window.Bottom = WindowHeight;

    point WindowHalfDim = { WindowWidth/2, WindowHeight/2 };

    // Move it centered to do the scaling
    rect Target = { -WindowHalfDim + (GlobalOffsetP/GlobalScale), WindowHalfDim + (GlobalOffsetP/GlobalScale) };

    Target.TopLeft = Target.TopLeft * GlobalScale;
    Target.BottomRight = Target.BottomRight * GlobalScale;

    // Now move back
    Target.TopLeft = Target.TopLeft + WindowHalfDim + GlobalOffsetP;
    Target.BottomRight = Target.BottomRight + WindowHalfDim + GlobalOffsetP;

    StretchDIBits(DeviceContext,
        Target.Left, Target.Top, Target.Right - Target.Left, Target.Bottom - Target.Top,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, // Using RGB, not palletized,
        SRCCOPY
    );
#else
    //TODO(handmade): Centering / black bars?
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
#endif
}

internal void
Win32ProcessPendingMessages(game_input* Input)
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
                int zDelta = GET_WHEEL_DELTA_WPARAM(Message.wParam);
            } break;

            case WM_MOUSEMOVE:
            {
                if (Message.wParam & MK_LBUTTON) {
                }
            } break;

            case WM_KEYDOWN:
            {
                if (Message.wParam == VK_SPACE ||
                    Message.wParam == VK_RIGHT ||
                    Message.wParam == VK_LEFT) {
                    Input->Reloaded = true;
                }
            } break;

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
    char EXEFileName[MAX_PATH];
    char* OnePastLastSlash;
    Win32GetEXEFileName(MAX_PATH, (char*)EXEFileName, &OnePastLastSlash);

    char SourceGameCodeDLL[MAX_PATH];
    Win32BuildEXEPathFileName(EXEFileName, OnePastLastSlash, 
                              "dracogen.dll", sizeof(SourceGameCodeDLL), SourceGameCodeDLL);

    char TempGameCodeDLL[MAX_PATH];
    Win32BuildEXEPathFileName(EXEFileName, OnePastLastSlash, 
                              "dracogen_temp.dll", sizeof(TempGameCodeDLL), TempGameCodeDLL);

    char LockFileName[MAX_PATH];
    Win32BuildEXEPathFileName(EXEFileName, OnePastLastSlash, 
                              "lock.tmp", sizeof(LockFileName), LockFileName);

    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
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
            "Dragon generator",
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

            game_memory GameMemory = {};
            GameMemory.PermanentSize = Megabytes(10);
            GameMemory.TemporarySize = Megabytes(100);
            GameMemory.PermanentBlock = VirtualAlloc((LPVOID)Terabytes(2), GameMemory.PermanentSize + GameMemory.TemporarySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TemporaryBlock = (u8*)GameMemory.PermanentBlock + GameMemory.PermanentSize;

            if (GameMemory.PermanentBlock && GameMemory.TemporaryBlock) 
            {
                GameMemory.IsInitialized = true;

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLL, TempGameCodeDLL, LockFileName);

                game_input Input = {};

                GlobalRunning = true;
                while(GlobalRunning)
                {
                    FILETIME NewDLLWriteTime = Win32GetFileModifiedTime(SourceGameCodeDLL);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLL,
                                                 TempGameCodeDLL,
                                                 LockFileName);

                        Input.Reloaded = true;
                    }
                    else { Input.Reloaded = false; }
    
                    Win32ProcessPendingMessages(&Input);

                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Width = GlobalBackbuffer.Width; 
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Pitch = GlobalBackbuffer.Pitch; 
                    Game.UpdateAndRender(&GameMemory, &Buffer, &Input);

                    ivec2 Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
                                               Dimension.Width, Dimension.Height);

                    Input.MonotonicCounter += 1;
                }
            }
        }
    }

    DeleteFile("dracogen_temp.dll");
}

