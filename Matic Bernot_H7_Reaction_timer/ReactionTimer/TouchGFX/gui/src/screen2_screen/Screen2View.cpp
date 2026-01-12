#include <gui/screen2_screen/Screen2View.hpp>
#include <images/BitmapDatabase.hpp>


Screen2View::Screen2View()
{

}

void Screen2View::setDifficulty(bool val)
{
    difficulty = val;

    if (difficulty)
        {
            target.setBitmaps(Bitmap(BITMAP_TARGET20_MIN_ID), Bitmap(BITMAP_TARGET20_MIN_ID));
        }
        else
        {
            target.setBitmaps(Bitmap(BITMAP_TARGET50_MIN_ID), Bitmap(BITMAP_TARGET50_MIN_ID));
        }

        target.invalidate();

}

void Screen2View::setTick(int val)
{
    infinitick = val;

}




static uint16_t randomish(uint32_t seed)
{
    seed = seed * 1103515245 + 12345;
    return (seed / 65536) % 32768;
}




void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();

    
    tickCounter=0;
    timePassed = 0;
    randomTime = 180 + (randomish(infinitick) % 120);
    

}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::handleTickEvent()
{
    tickCounter++;
    infinitick++;

    if(tickCounter == randomTime){

        infinitick++;
        randomX = 50 + (randomish(infinitick) % 380);
        infinitick++;
        randomY = 50 + (randomish(infinitick) % 170);
        target.setXY(randomX, randomY);
    }

    if(tickCounter >= randomTime && allowSpawn){

        target.setVisible(true);
        //target.invalidate();
        timePassed++;
    }
    target.invalidate();
}

void Screen2View::targetClicked()

{
    tickCounter = 0;
    double convertedTime = timePassed * 16.6667;
    Unicode::snprintf(textArea1Buffer, TEXTAREA1_SIZE, "%d", static_cast<int>(convertedTime));
    textArea1.invalidate();
    textArea1.setVisible(true);

    allowSpawn = false;

    timePassed = 0;

    target.setVisible(false);
    restart.setVisible(true);
    home.setVisible(true);

    restart.invalidate();
    home.invalidate();

    infinitick++;
    randomX = 50 + (randomish(infinitick) % 380);
    infinitick++;
    randomY = 50 + (randomish(infinitick) % 170);
    target.setXY(randomX, randomY);

}

void Screen2View::reset()

{
    tickCounter = 0;
    timePassed = 0;
    textArea1.setVisible(false);

    allowSpawn = true;
    restart.setVisible(false);
    home.setVisible(false);
    

    restart.invalidate();
    textArea1.invalidate();
    home.invalidate();


}




