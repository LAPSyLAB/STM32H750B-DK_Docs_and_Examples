#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

void Screen2Presenter::activate()
{
    view.setDifficulty(model->getDifficulty());
    view.setTick(model->getTick());
}

void Screen2Presenter::deactivate()
{

}


