#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

Screen3Presenter::Screen3Presenter(Screen3View& v)
    : view(v)
{

}

void Screen3Presenter::activate()
{
    view.setDifficulty(model->getDifficulty());
    view.setTick(model->getTick());
}

void Screen3Presenter::deactivate()
{

}
