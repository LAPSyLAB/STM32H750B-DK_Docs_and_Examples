#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <touchgfx/EasingEquations.hpp>
#include <touchgfx/mixins/FadeAnimator.hpp>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}


    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent();
    void targetClicked();

    void setDifficulty(bool val);
    void setTick(int val);
    void reset();
    void endGame();

    



protected:
//touchgfx::FadeAnimator< touchgfx::TextAreaWithOneWildcard > fade;

int tickCounter;
int timePassed;

int randomTime;
int randomX;
int randomY;
int numTargets = 0;
int time = 30;
int allTime = 0;
int infinitick;

bool difficulty = false;
bool allowSpawn = true;


};

#endif // SCREEN3VIEW_HPP
