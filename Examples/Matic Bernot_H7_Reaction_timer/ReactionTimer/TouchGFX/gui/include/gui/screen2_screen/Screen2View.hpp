#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}


    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent();
    void targetClicked();

    void setDifficulty(bool val);
    void setTick(int val);
    void reset();

    



protected:

int tickCounter;
int timePassed;

int randomTime;
int randomX;
int randomY;
int infinitick = 1;

bool difficulty = false;
bool allowSpawn = true;

};

#endif // SCREEN2VIEW_HPP
