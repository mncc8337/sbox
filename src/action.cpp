#include <action.h>
#include <screen.h>

extern void open_screen(Screen*);

Action::Action(std::string name) : name(name) {};

void Action::set_name(std::string new_name) {
    name = new_name;
}

DummyAction::DummyAction(std::string name) : Action(name) {};

void DummyAction::execute() {};

FunctionAction::FunctionAction(std::string name, void (*action_ptr)(void)) : Action(name), action_ptr(action_ptr) {}

void FunctionAction::execute() {
    action_ptr();
}

OpenScreenAction::OpenScreenAction(std::string name, Screen *screen) : Action(name), screen(screen) {}

void OpenScreenAction::execute() {
    open_screen(screen);
}
