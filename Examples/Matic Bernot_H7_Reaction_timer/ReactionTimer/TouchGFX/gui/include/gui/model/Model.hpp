#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    void storeDifficulty(bool val);
    bool getDifficulty() const;
    void storeTick(int val);
    int getTick() const;
protected:
    ModelListener* modelListener;
    bool difficulty = false;
    int randomTick = 1;
};

#endif // MODEL_HPP
