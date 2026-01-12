#include <gui/screen3_screen/Screen3View.hpp>
#include <images/BitmapDatabase.hpp>

Screen3View::Screen3View()
{

}

void Screen3View::setDifficulty(bool val)
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

void Screen3View::setTick(int val)
{
   infinitick = val;
    
}


static uint16_t randomish(uint32_t seed)
{
    seed = seed * 1103515245 + 12345;
    return (seed / 65536) % 32768;
}


void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();


    allTime = 0;
    numTargets = 0;
    tickCounter=0;
    infinitick++;
    randomTime = 180 + (randomish(infinitick) % 120);
    infinitick++;
    randomX = 50 + (randomish(infinitick) % 380);
    infinitick++;
    randomY = 50 + (randomish(infinitick) % 170);
    target.setXY(randomX, randomY);

}

void Screen3View::tearDownScreen()
{
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::handleTickEvent()
{
    tickCounter++;

    if(tickCounter >= randomTime && allowSpawn){

        target.setVisible(true);
        target.invalidate();
        timePassed++;

        if(tickCounter % 60 == 0){
            time--;
            Unicode::snprintf(timerBuffer, TIMER_SIZE, "%d", time);
            timer.invalidate();
        }


    }
    target.invalidate();



    if(time <= 0){
        time = 0;
        endGame();
    }

}

void Screen3View::targetClicked()

{
    //tickCounter = 0;
    double convertedTime = timePassed * 16.6667;
    Unicode::snprintf(fadeBuffer, FADE_SIZE, "%d", static_cast<int>(convertedTime));
    //fade.invalidate();
    fade.setPosition(198, 220, 84, 22);
    fade.setVisible(true);
    
    numTargets++;
    allTime += static_cast<int>(convertedTime);
    

    //allowSpawn = false;

    timePassed = 0;

    target.setVisible(false);
    //restart.setVisible(true);
    //home.setVisible(true);

    restart.invalidate();
    home.invalidate();
    fade.invalidate();
    infinitick++;
    randomX = 50 + (randomish(infinitick) % 380);
    infinitick++;
    randomY = 50 + (randomish(infinitick) % 170);
    target.setXY(randomX, randomY);

}

void Screen3View::reset()

{
    tickCounter = 0;
    timePassed = 0;
    allTime = 0;
    numTargets = 0;
    time=30;
    Unicode::snprintf(timerBuffer, TIMER_SIZE, "%d", time);
    fade.setVisible(false);
    textArea1.setVisible(false);

    allowSpawn = true;
    restart.setVisible(false);
    home.setVisible(false);
    timer.setVisible(true);
    
    timer.invalidate();
    restart.invalidate();
    fade.invalidate();
    home.invalidate();
    textArea1.invalidate();


}

void Screen3View::endGame()

{
    tickCounter = 0;
    timePassed = 0;
    restart.setVisible(true);
    home.setVisible(true);
    target.setVisible(false);
    allowSpawn = false;
    Unicode::snprintf(textArea1Buffer1, TEXTAREA1BUFFER1_SIZE, "%d", numTargets);
    Unicode::snprintf(textArea1Buffer2, TEXTAREA1BUFFER2_SIZE, "%d", static_cast<int>(allTime/numTargets));
    textArea1.setVisible(true);
    timer.setVisible(false);

    target.invalidate();
    restart.invalidate();
    textArea1.invalidate();
    home.invalidate();
    timer.invalidate();




}
