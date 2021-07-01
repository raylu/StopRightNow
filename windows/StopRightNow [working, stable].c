//// In the name of Allah, the Most Gracious, the Most Merciful ////

/******* INCLUDES *********/
#include <Windows.h>        // Hooks, Messageloop, etc. large components of the put_keystroke_to_input_buffer() function,
#include <stdio.h>         // tis a crime to not include this in any C program
#include <string.h>         // strstr(), strrchr(), strlen()
#include <stdlib.h>      // system(), malloc(), realloc()


/******* DEFINES(settings) & MACROS *********/
// defines whether the window is visible or not
#define invisible // (visible / invisible)

// defines whether to show characters in the window as you type them, or to display the entire buffer when 'ESC' is pressed
// note: nothing happens if invisible was defined.
//#define live_update

// if defined, the input buffer will not reset on pressing enter. this means its likely to store more chars, which may slow down performance
// by default, this is off
//#define dont_reset_buffer_on_enter

// debug switch: enabling this prevents system shutdown so you dont lose your code progress when testing the dumb program...
//#define DEBUG_SRN

// defines the size of the input buffer array. The larger it is, the less it has to get reset (and so less probability of getting cut-off words pass through the filter)
// but the larger it is, the more harder strstr will have to work to find your phrase (increasing CPU load)
#define BUFFER_SIZE 60

// defines the largest possible size of the trigger phrase. If the size of a trigger phrase found in the blocklist exceeds this number...
// To prevent an array overrun the program will terminate and ask user to fix the blocklist.
#define TRIGGER_MAX_SIZE 30

// input buffer should definitely be larger than the largest trigger phrase. I wanted it to be larger than 2x, but at that point memory wastage becomes an issue.
#if BUFFER_SIZE < (2 * TRIGGER_MAX_SIZE)
#error input_buffer[BUFFER_SIZE] must be large enough to fit the largest possible trigger phrase (char str[TRIGGER_MAX_SIZE])
#endif


/******* GLOBAL VARIABLES *********/
// The input buffer; this is where characters read from the keyboard are stored to be compared against the blocklist
char input_buffer[BUFFER_SIZE] = {0};
size_t input_buf_index = 0;

// Global pointer required as it is assigned in main, and then used in HookCallback
char **list_of_blocked_strings;

/* variable to store the HANDLE to the hook. Don't declare it anywhere else then globally
 or you will get problems since every function uses this variable. */
HHOOK _hook;

/* This struct contains the data received by the hook callback. As you see in the callback function
 it contains the thing you will need: vkCode = virtual key code. */
KBDLLHOOKSTRUCT kbdStruct;


/******* PROTOTYPES *********/
int put_keystroke_to_input_buffer(int key_stroke, const HKL layout);    // Intercepts keystrokes, converts them to char type and stores them inside the input buffer
char* fgets_0n(char *arr, int maxcount, FILE * fileptr);                // interface function for fgets(), should behave just like it, except that it removes the newline character at the end if there is any
char** ReadStrFromFile(FILE *fp);                                       // takes a file pointer and methodically extracts all newline-seperated strings and returns them as an array of character pointers. Includes comment detection
int CompareAndSignal(void);                                             // compares the global variable array of char pointers (trigger phrases) and tests them against the input buffer, returns 1 if found, 0 otherwise

/***********************************************************************/


// This is the callback function. Consider it the event that is raised when, in this case,
// a key is pressed.
LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        // the action is valid: HC_ACTION.
        if (wParam == WM_KEYDOWN)
        {
            const HKL layout = LoadKeyboardLayoutA("00000409", KLF_SUBSTITUTE_OK);

            // lParam is the pointer to the struct containing the data needed, so cast and assign it to kdbStruct.
            kbdStruct = *((KBDLLHOOKSTRUCT*) lParam);

            put_keystroke_to_input_buffer(kbdStruct.vkCode, layout);
            if (CompareAndSignal())
            {
                // TRIGGER WORD FOUND! begin shutdown process here.

                // basically reset the array, because CompareAndSignal() is gonna return true every single time you type a letter from now on... we only need the trigger to occur once.
                // tho lets be honest, the program is gonna stop execution as soon as "shutdown /s" is called, so it doesnt matter too much anyway..
                input_buf_index = 0;
                input_buffer[0] = 0;

                // Punish the user here.
                {
                    LPCSTR a = "Think hard about what you were going to do...";
                    LPCSTR b = "WHAT ARE YOU DOING!?";

                    MessageBox(NULL, a, b, MB_ICONERROR);
                }
                #ifndef DEBUG_SRN
                system("shutdown /s /t 5");
                #endif
            }
		}
    }

    // call the next hook in the hook chain. This is necessary or your hook chain will break and the hook stops
    return CallNextHookEx(_hook, nCode, wParam, lParam);
}


void SetHook()
{
    // Set the hook and set it to use the callback function above
    // WH_KEYBOARD_LL means it will set a low level keyboard hook. More information about it at MSDN.
    // The last 2 parameters are NULL, 0 because the callback function is in the same thread and window as the
    // function that sets and releases the hook.
    if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
    {
        LPCSTR a = "Failed to install hook!";
        LPCSTR b = "Critical Error";

        MessageBox(NULL, a, b, MB_ICONERROR);
    }
}


void ReleaseHook()
{
    UnhookWindowsHookEx(_hook);
}

int put_keystroke_to_input_buffer(int key_stroke, const HKL layout)
{
    int cur_char = 0;
    // Anfilt: if this a hot loop probably better to pass the layout as a variable to this function
    // AbdullahTrees: gud idea b0ss!
    //HKL layout = LoadKeyboardLayoutA("00000409", KLF_SUBSTITUTE_OK);

    switch(key_stroke)
    {
        case VK_RETURN:
            #ifndef dont_reset_buffer_on_enter
            input_buf_index = 0;
            #endif
            return 0;
        case VK_BACK:
            if(input_buf_index > 0)
            {
                input_buf_index--;
            }
            return 0;
        case VK_OEM_PERIOD: case VK_DECIMAL:
            cur_char = '.';
            break;
        case VK_OEM_MINUS: case VK_SUBTRACT:
            cur_char = '-';
            break;
        case VK_SPACE:
            cur_char = ' ';
            break;
        // Use fall through for these
        case VK_ESCAPE:
            #ifdef visible
                #ifndef live_update
                printf("\n%s", input_buffer);  // DEBUG
                return 0;
                #endif
            #endif
        case 1: case 2: // ignore mouse clicks
        case VK_CAPITAL: case VK_NEXT: case VK_PRIOR: case VK_DOWN: case VK_RIGHT:                  // ignore non-graphical keys
        case VK_LEFT: case VK_HOME: case VK_END: case VK_LWIN: case VK_RWIN: case VK_UP:
        case VK_MENU: case VK_RCONTROL: case VK_LCONTROL: case VK_CONTROL: case VK_RSHIFT:
        case VK_LSHIFT: case VK_SHIFT: case VK_TAB:
            return 0; // ignore
        default:
            cur_char = MapVirtualKeyExA(key_stroke, MAPVK_VK_TO_CHAR, layout);
            cur_char = tolower(cur_char);
            break;
    }

    if (input_buf_index >= BUFFER_SIZE - 2)
    {
        //fprintf(stderr, "MY BUFFER IS FULL!!!!");
        //return 0;

        // a partial triggerword may be present at the end of the (now filled) input_buffer array... what to do....
        // Solution: copy the end of the array (size of largest trigger phrase possible) to the beginning. then change input_buf_index to new position and start again
        #ifdef visible
        printf("\n[Buffer exceeded]");
        #endif
        input_buf_index = strlen(input_buffer) - TRIGGER_MAX_SIZE - 1;

        strcpy(input_buffer, input_buffer + input_buf_index);

        input_buf_index = strlen(input_buffer);
    }
    input_buffer[input_buf_index]     = (char) cur_char;
    input_buffer[input_buf_index + 1] = '\0';
    input_buf_index++;

    #ifdef visible
        #ifdef live_update
            putchar(input_buffer[input_buf_index - 1]);     // DEBUG
        #endif
    #endif

    return 0;
}


void Stealth()
{
#ifdef visible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), SW_SHOWNORMAL); // visible window
#else
#ifdef invisible
    ShowWindow(FindWindowA("ConsoleWindowClass", NULL), SW_HIDE); // invisible window
#endif
#endif
}


int main()
{
	FILE *file_blocklist;

	// Splash
	#ifdef visible
        printf("StopRightNow!!!: An application that intercepts keystrokes globally and takes action if a trigger word/phrase is found\n");
        printf("By AbdullahTrees\n");

        #ifdef live_update
            printf("Live-Update Enabled: Characters will display in the window as they are typed.\n");
        #endif

        #ifdef DEBUG_SRN
            printf("DEBUG Mode: Shutdown will not occur.\n");
            printf("Compile-time constants: BUFFER_SIZE = %d \t|\tTRIGGER_MAX_SIZE = %d\n", BUFFER_SIZE, TRIGGER_MAX_SIZE);
        #endif

        #ifdef dont_reset_buffer_on_enter
            printf("Buffer reset on Enter/Return disabled: The input buffer will not reset upon pressing the enter key.\n");
        #endif
	#endif

	file_blocklist = fopen("blocklist.dat", "r+");
	if (file_blocklist == NULL)
	{
		LPCSTR message = "Failed to open blocklist!";
        LPCSTR title = "Error";

		MessageBox(NULL, message, title, MB_ICONERROR);
		return 255;
	}
	else
    {
        // Begin importing trigger words/phrases into memory.
        /* Specifications

            BEGIN FILE
            trigger phrase 1
            # comments, anything or any line with a # sign should be ignored
            trigger phrase 2
            trigger phrase 3
            END OF FILE

            Each trigger phrase will be seperated by newlines. Each trigger phrase cannot exceed TRIGGER_MAX_SIZE no. of characters (global variable limit)
        */
        // begin seeking to find no. of trigger phrases and hence memory which should be allocated by malloc

        list_of_blocked_strings = ReadStrFromFile(file_blocklist);
        // DEBUG
        #ifdef visible
        printf("List of all trigger phrases loaded: \n");
        for(int k = 0; list_of_blocked_strings[k]; k++)
        {
            printf("%s\n", list_of_blocked_strings[k]);
        }

        putchar('\n'), putchar('\n'), putchar('\n');
        #endif
    }
    fclose(file_blocklist);

    // visibility of window
    Stealth();

    // set the hook
    SetHook();

    // loop to keep the console application running.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
    }
}


char* fgets_0n(char *arr, int maxcount, FILE *fileptr)
{
    char *c, *fgets_return;

    fgets_return = fgets(arr, maxcount, fileptr);
    if (fgets_return == NULL)
        return fgets_return;

    c = strrchr(arr, '\n');
    if (c)
        *c = '\0';

    return fgets_return;
}


char** ReadStrFromFile(FILE *fp)
{
    char **arr_of_strs_UNFIXED;
    int i;

    for (i = 0, arr_of_strs_UNFIXED = NULL; 1; i++)
    {
        int m;
        char *current_str;
        char temp_store[TRIGGER_MAX_SIZE + 2];

        if (fgets_0n(temp_store, TRIGGER_MAX_SIZE, fp) == NULL)
        {
            if (feof(fp))
                break;

            LPCSTR message = "Error reading file! An unknown error occurred [fgets() returned NULL]";
            LPCSTR title = "Error!";

            MessageBox(NULL, message, title, MB_ICONERROR);
            exit(127);
        }

        m = strlen(temp_store) + 1;

        // Comment implementation in file. Use #... to comment, which ignores the entire line and prevents it from being added to the blocklist
        if (*temp_store == '#')
        {
            if (m == TRIGGER_MAX_SIZE)
            {
                /* temp_store is full.... this probably means theres more characters in the same line... so use fgets to store into temp_store and then use strrchr to find \newline char
                and then end the current iteration of the loop so that the next iteration starts with the file already seeked to the next line.


                    PS: i know the past tense of the dictionary word 'seek' is 'sought', but 'seek' in C file I/O has a very different meaning compared to 'seek' in English
                */
                checkpoint:  // RIP gotophobes,
                if (fgets(temp_store, TRIGGER_MAX_SIZE, fp) == NULL)
                {
                    if (feof(fp))
                        break;   // why i used goto, to avoid creating a nested loop

                    LPCSTR message = "Error reading file! An unknown error occurred [fgets() returned NULL]";
                    LPCSTR title = "Error!";

                    MessageBox(NULL, message, title, MB_ICONERROR);
                    exit(127);
                }

                if (strrchr(temp_store, '\n') == NULL)  // newline char not found, the comment line is longer and fgets has not seeked to the beginning of the new line yet.
                    goto checkpoint;
            }

            i--;
            continue;
        }

        if (m == 1) // if m == 1, strlen(temp_store) was zero, meaning its a null string!! this doesn't need to be imported!
        {
            i--;
            continue;
        }

        if (m == TRIGGER_MAX_SIZE)
        {
            #define MAKE_STR_LITERAL(a) #a
            #define MACRO_INT_TO_STR(a) MAKE_STR_LITERAL(a)         // why on earth do you need 2 macros? because that's how C works... Anyways this is important to concatenate a constant compile-time string
            /*
                lethern's explanation
                ---------------------------------
                > let's say that #define TEST 20

                `puts( MAKE_STR_LITERAL(x) )` prints x
                `puts( MAKE_STR_LITERAL(1) )` prints 1

                > it changes whatever there is into a string literal
                > the issue is that MAKE_STR_LITERAL(TEST) will also turn it into a string literal, so it becomes "TEST"
                >  thats why we need one indirection
                > MACRO_INT_TO_STR(TEST) will evaluate to MAKE_STR_LITERAL(20), and that will become "20"

                > so
                `const char *str = "Error: " MAKE_STR_LITERAL(x);`
                > becomes
                `const char *str = "Error: " "20";`
                > and "x" "y" is the same as "xy" or "x"+"y"      // C string concatenation...
            */

            LPCSTR message = "The blocklist contains a line which has exceeded the maximum size of an allowed trigger phrase(" MACRO_INT_TO_STR(TRIGGER_MAX_SIZE) "). The program can continue running, but detection may be faulty. Please revise the blocklist file and restart the program.";
            LPCSTR title = "Warning! Max characters per line reached";

            MessageBox(NULL, message, title, MB_ICONWARNING);
            #undef MACRO_INT_TO_STR
            #undef MAKE_STR_LITERAL
        }

        current_str = malloc(m);
        strcpy(current_str, temp_store);

        arr_of_strs_UNFIXED = realloc(arr_of_strs_UNFIXED, sizeof(current_str)*(i+2));  // for i==0, "If ptr is NULL, the behavior is the same as calling malloc(new_size)."
        if (arr_of_strs_UNFIXED == NULL)
        {
            LPCSTR message = "Memory allocation failed! ";
            LPCSTR title = "Critical Error!";

            MessageBox(NULL, message, title, MB_ICONERROR);
            exit(255);
        }
        else
        {
            arr_of_strs_UNFIXED[i] = current_str;
            arr_of_strs_UNFIXED[i + 1] = NULL;    // This is the terminator for the array of pointers... So to check whether you've reached the end of array, check for NULL pointer
        }
    }

    return arr_of_strs_UNFIXED;
}

int CompareAndSignal(void)
{
    // serially compares against blocklist and returns true if a match has been found, false otherwise
    int i;

    for(i = 0; list_of_blocked_strings[i]; i++)
    {
        if (strstr(input_buffer, list_of_blocked_strings[i]) != NULL)   // if returns true if strstr returned a valid pointer (not null)
            return 1;
    }

    return 0;
}
