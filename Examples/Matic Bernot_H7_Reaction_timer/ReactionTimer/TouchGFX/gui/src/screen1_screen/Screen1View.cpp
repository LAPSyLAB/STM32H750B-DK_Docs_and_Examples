#include <gui/screen1_screen/Screen1View.hpp>

Screen1View::Screen1View()
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    difficultyButton.forceState(difficulty);
   
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::toggleDifficulty()
{
    difficulty = difficultyButton.getState();
    presenter->storeDifficulty(difficulty);
}

void Screen1View::handleTickEvent()
{
    randomTick++;
}

void Screen1View::getTick()
{
    randomTick++;
    randomTick = randomTick * 1103515245 + 12345;
    randomTick = (randomTick / 65536) % 32768;
    presenter->storeTick(randomTick);
}


