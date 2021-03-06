#ifndef DIRT_INPUTLINE_H
#define DIRT_INPUTLINE_H

#include "Window.h"
#include "defs.h"
#include <vector>


#define MAX_PROMPT_BUF 80
extern time_t current_time;

// History ids
typedef enum {hi_none=0, hi_generic, hi_main_input, hi_open_mud, hi_search_scrollback} history_id;
class InputHistorySelection;

// Input line. This one handles displaying/editing
class InputLine : public Window {
public:
    InputLine(Window *_parent, int _w, int _h, Style _style, int _x, int _y, history_id _id);
    virtual ~InputLine() {};
    
    bool getline(char *buf, bool fForce); // Return true if there is a line available
    inline int  getlastkey() { return lastkey; } // return the last key pressed.
    void set_prompt (const char *prompt);
    char* get_prompt(void) { return prompt_buf; }
    void set_default_prompt();
    virtual void redraw();
    virtual void set (const char *s); // Set the input line to this
    virtual char* get () { return input_buf; }
    NAME(InputLine);   
    virtual bool keypress(int key); // FIXME remove after key transition complete.

    static bool keypress_ctrl_a(string&, void*);
    static bool keypress_ctrl_c(string&, void*);
    static bool keypress_ctrl_e(string&, void*);
    static bool keypress_ctrl_k(string&, void*);
    static bool keypress_ctrl_u(string&, void*);
    static bool keypress_ctrl_w(string&, void*);
    static bool keypress_escape(string&, void*);
    static bool keypress_backspace(string&, void*);
    static bool keypress_delete(string&, void*);
    static bool keypress_enter(string&, void*);
    static bool keypress_arrow_left(string&, void*);
    static bool keypress_arrow_right(string&, void*);
    static bool keypress_arrow_up(string&, void*);
    static bool keypress_arrow_down(string&, void*);
protected:
    
    char input_buf[MAX_INPUT_BUF];
    char prompt_buf[MAX_PROMPT_BUF];
    unsigned int cursor_pos;     // Where will the next character be inserted?
    unsigned int max_pos;	// How many characters are there in the buffer?
    unsigned int left_pos;	// What is the left position?
    
    bool ready;		// The input line holds a finished string (hmm)
    
    history_id id;	// ID for history saving
    int history_pos;    // For cycling through history
    int lastkey;        // the last key pressed, so KeypressHookStub can get at it.
    
    
    virtual void adjust();	// Adjust left_pos
    virtual bool isExpandable() {return false;}
    
    static void selection_callback(void *obj, const char *s, int no);
    
    friend class InputHistorySelection;
};

// This is dirt's main input line
class MainInputLine : public InputLine {
public:
    MainInputLine();
    
    virtual void set (const char *s); // Set the input line to this
    virtual bool isExpandable() { return true; }
    NAME(MainInputLine);
};

// A history for one input line
// This class is used internally by InputLine
class History {
public:
    History(int _id);
    ~History();
    
    void add (const string& s,time_t);			// Add this string
    const char * get (int no, time_t *timestamp);	// Get this string.

    int id;						// Id number
    
private:
    
    string *strings;					// Array of strings
    time_t *timestamps;
    int max_history;					// Max number of strings
    int current;					// Current place we will insert a new
};

// This class has a set of history arrays
// This is so they can save between invokactions of the input line in
// question without requiring globals
class HistorySet {
private:
    typedef vector<History*> hist_list_t;
    hist_list_t hist_list;
public:
    HistorySet();
    ~HistorySet();
    void saveHistory();
    void loadHistory();
    
    const char *get (history_id id, int count, time_t* timestamp) {
        return hist_list[id]->get(count, timestamp);
    }
    
    void add (history_id id, const string& s, time_t t = 0) {
        hist_list[id]->add(s,t ? t : current_time);
    }
    
};

extern HistorySet *history;
extern MainInputLine *inputLine;

#endif
