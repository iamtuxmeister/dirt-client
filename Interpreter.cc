// a class to keep a list of commands and execute them one at a time

#include <iostream>
#include "Interpreter.h"
#include "EmbeddedInterpreter.h"
#include "Option.h"
#include "Config.h"
#include "MessageWindow.h"
#include "OutputWindow.h"
#include "Shell.h"
#include "Chat.h"
#include "Session.h"
#include "Screen.h"
#include "MUD.h"
#include "Hook.h"
#include "StatusLine.h"
#include "StaticBuffer.h"
#include "InputLine.h"

extern MUD *lastMud;
extern bool dirtFinished;


// Pick off one argument, optionally smashing case and place it in buf
// Eat whitespace, respect "" and ''
// FIXME This fails for escaped quotes.  From Notes:
// Add a function to take the command, parse it, and return argv, argc style arg
//    list?  Should I do option processing too?  (would add to consistency)
//    Maybe return a hash of option->value pairs.
const char *one_argument (const char *argument, char *buf, bool smash_case) {
    char end;
    
    while(isspace(*argument))
        argument++;

    if (*argument == '\'' || *argument == '\"')
        end = *argument++;
    else
        end = NUL;

    while(*argument && (end ? *argument != end : !isspace(*argument)))  {
        *buf++ = smash_case ? tolower(*argument) : *argument;
        argument++;
    }

    *buf++ = NUL;

    if (*argument && end)
        argument++;

    while(isspace(*argument))
        argument++;

    return argument;
}

bool Interpreter::command_quit(string&, void*, savedmatch* sm) {
    dirtFinished = true;
    if(sm) sm->retval = true;
    return true;
}

bool Interpreter::command_echo(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "nW:");
    if(sm) sm->retval = true;
    string s = opt.restStr();
    if(!opt.flag('n')) s.append("\n");

    if(!opt.valid()) return true;
    if(opt.gotOpt('W')) {
        MessageWindow* w = MessageWindow::find(opt['W'].c_str());
        if (!w) {
	    int CmdChar = config->getOption(opt_commandcharacter);
            status->setf("%cprint: %s: no such window", CmdChar, opt['W'].c_str());
            return true;
        }
        w->addInput(s.c_str());
    } else cout << s; //outputWindow->printf("%s", s.c_str());
    return true;
}

bool Interpreter::command_status(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "W:");
    if(sm) sm->retval = true;
    if(!opt.valid()) return true;
    if(!opt.gotOpt('W')) status->setf("%s", opt.restStr().c_str());
    else {
        MessageWindow* mw = MessageWindow::find(opt['W']);
        if(!mw) { 
           int CmdChar = config->getOption(opt_commandcharacter);
	   report_err("%cstatus: unable to find window '%s'\n", CmdChar, opt['W'].c_str()); 
            return true;
        }
        mw->set_bottom_message(opt.restStr().c_str());
    }
    return true;
}

bool Interpreter::command_bell(string&, void*, savedmatch* sm) {
    screen->flash();
    if(sm) sm->retval = true;
    return true;
}

bool Interpreter::command_exec(string& str, void*, savedmatch* sm) {
    int w = 80, h=10, x=0, y=3, t=10;
    if(sm) sm->retval = true; // FIXME return the return value from the command instead?
    
    OptionParser opt(str, "w:h:x:y:t:");
    if(!opt.valid()) return true;
    
    if(opt.gotOpt('w')) w = atoi(opt['w'].c_str());
    if(opt.gotOpt('h')) h = atoi(opt['h'].c_str());
    if(opt.gotOpt('x')) x = atoi(opt['x'].c_str());
    if(opt.gotOpt('y')) y = atoi(opt['y'].c_str());
    if(opt.gotOpt('t')) t = atoi(opt['t'].c_str());
            
    // Do some sanity checking
    int CmdChar = config->getOption(opt_commandcharacter);
    if (h < 3)
        status->setf("%cexec: Value h=%d too low.", CmdChar, h);
    else if (w < 3)
        status->setf("%cexec: Value w=%d too low.", CmdChar, w);
    else if (x < 0)
        status->setf("%cexec: Value x=%d too low.", CmdChar, x);
    else if (y < 0)
        status->setf("%cexec: Value y=%d too low.", CmdChar, y);
    else if (w+x > screen->width)
        status->setf("%cexec: Value w=%d or x=%d too large.  Window would extend beyond screen.", CmdChar, w, x);
    else if (y+h > screen->height)
        status->setf("%cexec: Value y=%d or h=%d too large.  Window would extend beyond screen.", CmdChar, y, h);
    else
        new Shell(screen, opt.restStr().c_str(), w,h,x,y,t);
    return true; 
}

bool Interpreter::command_clear(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "W:");
    if(sm) sm->retval = true;
    if(!opt.valid()) return true;
    string name;
    if(opt.gotOpt('W')) name = opt['W'];
    else name = opt.arg(1);
    
    if (!name.length()) {  // no window specified.
        outputWindow->clear();
        outputWindow->move(0,0);
    }
    else {
        MessageWindow *w = MessageWindow::find(name);
        if (!w) {
	    int CmdChar = config->getOption(opt_commandcharacter);
            status->setf("%cclear: %s: no such window", CmdChar, name.c_str());
	}
        else {
            w->clear();
            w->gotoxy(0,0);
        }
    }
    return true;
}

bool Interpreter::command_prompt(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "");
    if(sm) sm->retval = true;
    if(!opt.valid()) return true;
    inputLine->set_prompt(opt.restStr().c_str());
    return true;
}

bool Interpreter::command_close(string&, void*, savedmatch* sm) {
    if(sm) sm->retval = true;
    if (!currentSession || currentSession->state == disconnected)
        status->setf ("You are not connected - nothing to close");
    else
    {
        status->setf ("Closing link to %s@%s:%d", currentSession->mud.getName(), 
                currentSession->mud.getHostname(), currentSession->mud.getPort());
        inputLine->set_default_prompt();
        delete currentSession;
        currentSession = NULL;
    }
    return true;
}

bool Interpreter::command_open(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "");
    if(sm) sm->retval = true;
    if(!opt.valid()) return true;
    string name = opt.arg(1);
    int CmdChar = config->getOption(opt_commandcharacter);

    if(opt.argc() < 1) status->setf ("%copen: open a connection to where?", CmdChar);

    if (currentSession && currentSession->state != disconnected)
        status->setf ("You are connected to %s, use %cclose first", 
                currentSession->mud.getName(), CmdChar);
    else
    {
        MUD *mud;
        if (!(mud = config->findMud (name.c_str())))
            status->setf ("MUD '%s' was not found", name.c_str());
        else
        {
            delete currentSession;
            currentSession = new Session(*mud, outputWindow);
            currentSession->open();
            lastMud = mud;
        }
    }
    return true;
}

bool Interpreter::command_reopen(string&, void* mt, savedmatch* sm) {
    Interpreter* mythis = (Interpreter*)mt;
    if(sm) sm->retval = true;
    if (!lastMud)
        status->setf ("There is no previous MUD to which I can reconnect");
    else
    {
        string s = "/open ";
        s += lastMud->getName();
        mythis->commands->push_back(pair<string,savedmatch*>(s,NULL));
    }
    return true;
}

bool Interpreter::command_send(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "nu");
    if(sm) sm->retval = true;
    if(!opt.valid()) return true;
    string towrite = opt.restStr();

    if (!currentSession) status->setf("No session active");
    else {
        if(opt.gotOpt('n')) {
            if(opt.gotOpt('u')) 
                currentSession->writeUnbuffered(towrite.c_str(), towrite.length());
            else writeMUDn(towrite);
        }
        else { 
            string gonnawrite = towrite.append("\n");
            if(opt.gotOpt('u')) 
                currentSession->writeUnbuffered(gonnawrite.c_str(), gonnawrite.length());
            else writeMUD(towrite);  // writeMUD adds a CR
        }
    }
    return true;
}

bool Interpreter::command_setinput(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "");
    if(sm) sm->retval = true;
    inputLine->set(opt.restStr().c_str());
    return true;
}

bool Interpreter::command_save(string& str, void*, savedmatch* sm) {
    OptionParser opt(str, "a");
    if(sm) sm->retval = true;
    if(!opt.valid()) return true;
    bool color = opt.gotOpt('a');
    
    if(opt.argc() < 1) status->setf("Specify file to save scrollback to.");
    else outputWindow->saveToFile(opt.arg(1).c_str(), color);
    return true;
}

// Construct Interpreter, add dirt commands as command hook callbacks.
Interpreter::Interpreter() {    
    hook.addDirtCommand("quit", &command_quit, (void*)this);        /* doc */
    hook.addDirtCommand("echo", &command_echo, (void*)this);        /* doc */
    hook.addDirtCommand("status", &command_status, (void*)this);    /* doc */
    hook.addDirtCommand("bell", &command_bell, (void*)this);        /* doc */
    hook.addDirtCommand("exec", &command_exec, (void*)this);        /* doc */
    hook.addDirtCommand("clear", &command_clear, (void*)this);      /* doc */
    hook.addDirtCommand("prompt", &command_prompt, (void*)this);    /* doc */
    hook.addDirtCommand("close", &command_close, (void*)this);      /* doc */
    hook.addDirtCommand("open", &command_open, (void*)this);        /* doc */
    hook.addDirtCommand("reopen", &command_reopen, (void*)this);    /* doc */ // FIXME make an alias instead?
    hook.addDirtCommand("send", &command_send, (void*)this);        /* doc */
    hook.addDirtCommand("setinput", &command_setinput, (void*)this);/* doc */
    hook.addDirtCommand("save", &command_save, (void*)this);        /* doc */
    hook.addDirtCommand("window", &MessageWindow::command_window, (void*)NULL); /* doc */
    hook.addDirtCommand("hook", &Hook::command_hook, (void*)&hook); /* doc */     // FIXME unfinished...
    hook.addDirtCommand("enable", &Hook::command_enable, (void*)&hook); /* doc */
    hook.addDirtCommand("disable", &Hook::command_disable, (void*)&hook); /* doc */
    hook.addDirtCommand("group", &Hook::command_group, (void*)&hook); /* doc */
    hook.addDirtCommand("run", &EmbeddedInterpreter::command_run, (void*)embed_interp);  /* doc */
    hook.addDirtCommand("eval", &EmbeddedInterpreter::command_eval, (void*)embed_interp);/* doc */
    hook.addDirtCommand("load", &EmbeddedInterpreter::command_load, (void*)embed_interp);/* doc */
    hook.add(SEND, new CppHookStub(INT_MAX, 1.0, -1, false, true, true,  // FIXME command to toggle this?
        "__DIRT_BUILTIN_expandSemicolon", vector<string>(1,"Dirt builtins"), &Interpreter::expandSemicolon));
    hook.add(SEND, new CppHookStub(INT_MAX-1, 1.0, -1, false, true, true,// FIXME command to toggle this?
        "__DIRT_BUILTIN_expandSpeedwalk", vector<string>(1,"Dirt builtins"), &Interpreter::expandSpeedwalk));
    commands = new deque<pair<string,savedmatch*> >;
    command_stack.push_back(commands);
}

void Interpreter::execute() {
    //report("Interpreter::execute() stop mark is: %s\n", stopmark->first.c_str());
    // FIXME should we process a fixed # of commands and return?  We can always
    // process the rest on the next call, and we need to give the screen a chance
    // to update if we're sending lots of commands->
    while(!commands->empty())    {
        pair<string,savedmatch*> line = commands->front();
        commands->pop_front();  // destroy the command on the top of the stack.
        if (line.first.length() > 0 && line.first[0] == commandCharacter) {
            hook.run(COMMAND, line.first, line.second);
            dirtCommand (line.first.c_str() + 1);  // FIXME remove this once all commands are class members
        } else if(currentSession) {
            hook.run(SEND, line.first);
        } else
            status->setf ("You are not connected. Use Alt-O to connect to a MUD.");
        if(line.second) delete line.second;
    }
    if(command_stack.size() > 1) {
        delete commands;
        commands = command_stack.back();
        command_stack.pop_back();
    }
}

void Interpreter::execute(string& data) {
    // FIXME should we process a fixed # of commands and return?  We can always
    // process the rest on the next call, and we need to give the screen a chance
    // to update if we're sending lots of commands->
    while(!commands->empty())    
    {
        string nore("");
        savedmatch sm(data, nore);
        pair<string,savedmatch*> line = commands->front();
        commands->pop_front();  // destroy the command on the top of the stack.
        if (line.first.length() > 0 && line.first[0] == commandCharacter) {
            hook.run(COMMAND, line.first, &sm); // If line.first, by chance, deletes the parent hook
            if(sm.data != data) data = sm.data; // we're screwed.
            dirtCommand (line.first.c_str() + 1);  // FIXME remove this once all commands are class members
        } else if(currentSession) {
            hook.run(SEND, line.first);
        } else
            status->setf ("You are not connected. Use Alt-O to connect to a MUD.");
        if(line.second) delete line.second;
    }
    if(command_stack.size() > 1) {
        delete commands;
        commands = command_stack.back();
        command_stack.pop_back();
    }
}

// Legal speedwalk strings
const char legal_standard_speedwalk[] = "nsewud0123456789";
const char legal_extended_speedwalk[] = "nsewudhjkl0123456789";

// Expand possible speedwalk string
bool Interpreter::expandSpeedwalk(string& str)
{
    unsigned int start = 0;
    bool try_speedwalk = config->getOption(opt_speedwalk);
    
    if (str[0] == config->getOption(opt_speedwalk_character)
        && str.find_first_not_of(legal_extended_speedwalk, 1) == str.npos) {
        start = 1;
        try_speedwalk = true;
    } else if(str.length() >= 2 && str.find_first_not_of(legal_standard_speedwalk) == str.npos) {
        start = 0;
    } else {
        try_speedwalk = false;
    }
    
    if (try_speedwalk)
    {
        int repeat = 0,j = 0;
        deque<pair<string,savedmatch*> > replacement;
        for(unsigned int i=start;i<str.length();i++) {
            switch (str[i]) {
                case 'u':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("u",NULL)); 
                    repeat=0;
                    break;
                case 'd':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("d",NULL)); 
                    repeat=0;
                    break;
                case 'n':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("n",NULL)); 
                    repeat=0;
                    break;
                case 's':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("s",NULL)); 
                    repeat=0;
                    break;
                case 'e':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("e",NULL)); 
                    repeat=0;
                    break;
                case 'w':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("w",NULL)); 
                    repeat=0;
                    break;
// extra cases won't matter for standard speedwalk since we already checked that
// the string doesn't contain hjkl
                case 'h':   
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("nw",NULL)); 
                    repeat=0;
                    break;
                case 'j':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("ne",NULL)); 
                    repeat=0;
                    break;
                case 'k':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("sw",NULL)); 
                    repeat=0;
                    break;
                case 'l':
                    if(repeat == 0) repeat = 1;
                    for(j=0;j<repeat;j++) replacement.push_back(pair<string,savedmatch*>("se",NULL)); 
                    repeat=0;
                    break;
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    repeat = 10*repeat + str[i] - '0';
            }
        }
        interpreter.commands->insert(interpreter.commands->begin(), replacement.begin(), 
            replacement.end());
        return true;  // non-fallthrough will prevent other hooks from processing this command
    }
    return false; // not a speedwalk string, will fall-through to other hooks.
}

bool Interpreter::expandSemicolon(string& str) {
    bool retval = false;
    size_t pos=0, lastescape=0, firstescape=0;
    deque<pair<string,savedmatch*> > replacement;

    while((pos = str.find(';', pos)) != string::npos) { // returns index of first ; or -1 if not found
        if(str[pos-1] == '\\') {
            lastescape = str.rfind('\\', pos);
            lastescape = (lastescape > 0)?lastescape:pos;
            firstescape = str.find_last_not_of('\\', pos-1) + 1;
            if((pos - firstescape)%2 == 0) {   // number of backslashes is even.
                replacement.push_back(pair<string,savedmatch*>(str.substr(0, pos),NULL));
                str.replace(0, pos, "");
                retval = true;
                pos = 0;
            } else { // number of backslashes is odd, semicolon is escaped.  (DO NOTHING...)
                pos++; // keep going...(debackslashify in writeMUD will take care of backslashes)
            }
        } else {
            replacement.push_back(pair<string,savedmatch*>(str.substr(0, pos),NULL));
            str.replace(0, pos + 1, "");
            pos = 0;
            retval = true;
        }
    }
    if(retval) {
        replacement.push_back(pair<string,savedmatch*>(str,NULL));
    }
    interpreter.commands->insert(interpreter.commands->begin(), replacement.begin(), 
        replacement.end());
    return retval;
}

// Execute a dirt Command (s should *not* start with commandCharacter)
void Interpreter::dirtCommand (const char *s)
{
    char cmd[256];
    
    s = one_argument(s, cmd, true);
    
    // Chat subcommands
    if (!strncmp(cmd, "chat.", strlen("chat."))) {
        const char *subcmd = cmd+strlen("chat.");
        if (!chatServerSocket)
            status->setf("CHAT is not active. Set chat.name option in the config file");
        else
            chatServerSocket->handleUserCommand(subcmd,s);
    }

    // Chat aliases
    else if (!strcmp(cmd, "chat"))
        dirtCommand(Sprintf("%s %s", "chat.to", s));
    else if (!strcmp(cmd, "chatall"))
        dirtCommand(Sprintf("%s %s", "chat.all", s));
    else {
        if (embed_interp->run_quietly(NULL, Sprintf("dirtcmd_%s", cmd),s, NULL, true))
            return;
        
//        status->setf ("Unknown command: %s", cmd);
    }
}

void Interpreter::setCommandCharacter(int c) {
    commandCharacter = c;
}

char Interpreter::getCommandCharacter() {
    return commandCharacter;
}

void Interpreter::mark() {
    command_stack.push_back(commands);
    commands = new deque<pair<string,savedmatch*> >;
}

void Interpreter::dump_stack(void) {
    string s;
    size_t i;
    for(i=0;i<command_stack.size();i++) {
        for(deque<pair<string,savedmatch*> >::iterator it = command_stack[i]->begin();it != command_stack[i]->end();it++) {
            s.append(it->first);
            s.append(", ");
        }
        s.append(" | ");
    }
    report("Interpreter::commands: %s\n", s.c_str());
}

