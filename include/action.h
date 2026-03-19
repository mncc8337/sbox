#ifndef ACTION_H
#define ACTION_H

#include <string>

class Screen;
class U8G2;

class Action {
public:
    std::string name;

    Action(std::string name);
    virtual void execute() = 0;

    void set_name(std::string new_name);
};

class DummyAction: public Action {
public:
    DummyAction(std::string name);
    void execute() override;
};

class FunctionAction: public Action {
private:
    void (*action_ptr)(void);
public:
    FunctionAction(std::string name, void (*action_ptr)(void));
    void execute() override;
};

class OpenScreenAction: public Action {
private:
    Screen *screen;
public:
    OpenScreenAction(std::string name, Screen *screen);
    void execute() override;
};

#endif
