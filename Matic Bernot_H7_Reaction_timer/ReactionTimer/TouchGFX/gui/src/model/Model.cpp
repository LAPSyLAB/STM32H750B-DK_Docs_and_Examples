#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

Model::Model() : modelListener(0)
{

}

void Model::tick()
{

}

void Model::storeDifficulty(bool val)
{
    difficulty = val;
}

bool Model::getDifficulty() const
{
    return difficulty;
}

void Model::storeTick(int val)
{
    randomTick = val;
}

int Model::getTick() const
{
    return randomTick;
}
