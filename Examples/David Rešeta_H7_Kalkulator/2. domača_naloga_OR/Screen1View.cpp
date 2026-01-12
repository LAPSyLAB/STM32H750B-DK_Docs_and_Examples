#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Unicode.hpp>
#include <cstring>
#include <cstdlib>
#include <cstdio>  // za snprintf
#include <cmath>

Screen1View::Screen1View() : currentInput("0"), previousInput(""), currentOperator('\0')
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    updateDisplay();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

// Funkcija za pretvorbo double v niz brez uporabe %f
void doubleToString(double value, char* buffer, size_t bufferSize)
{
    bool isNegative = false;
    if (value < 0)
    {
        isNegative = true;
        value = -value;
    }

    int intPart = static_cast<int>(value);
    double fraction = value - intPart;
    int fracPart = static_cast<int>(fraction * 100); // dve decimalki

    if (isNegative)
    {
        snprintf(buffer, bufferSize, "-%d.%02d", intPart, fracPart);
    }
    else
    {
        snprintf(buffer, bufferSize, "%d.%02d", intPart, fracPart);
    }
}

void Screen1View::updateDisplay()
{
    // Deklariramo lokalni buffer pravilnega tipa (UnicodeChar)
    static touchgfx::Unicode::UnicodeChar buffer[20];

    // Če je currentInput prazen, prikažemo "0"
    std::string toDisplay = currentInput.empty() ? "0" : currentInput;

    // Kopiramo niz v Unicode buffer
    touchgfx::Unicode::strncpy(buffer, toDisplay.c_str(), 20);

    // Posodobimo textArea z novim wildcard bufferjem in prisilimo ponovno risanje
    textArea1.setWildcard(buffer);
    textArea1.invalidate();
}
void Screen1View::operatorErase()
{
	currentInput = "0";
	previousInput = "0";
	currentOperator = '\0';
	updateDisplay();

}
void Screen1View::number7()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('7');
    updateDisplay();
}

void Screen1View::number8()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('8');
    updateDisplay();
}

void Screen1View::number9()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('9');
    updateDisplay();
}

void Screen1View::number4()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('4');
    updateDisplay();
}

void Screen1View::number5()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('5');
    updateDisplay();
}

void Screen1View::number6()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('6');
    updateDisplay();
}

void Screen1View::number1()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('1');
    updateDisplay();
}

void Screen1View::number2()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('2');
    updateDisplay();
}

void Screen1View::number3()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('3');
    updateDisplay();
}

void Screen1View::number0()
{
    if (currentInput == "0")
    {
        currentInput = "";
    }
    currentInput.push_back('0');
    updateDisplay();
}

void Screen1View::operatorMinus()
{
    // Preverimo, če v izrazu že obstaja operator, če ne, ga dodamo
    if (currentInput.find('-') == std::string::npos)
    {
        currentInput.push_back('-');
        currentOperator = '-';
    }
    updateDisplay();
}

void Screen1View::operatorMultiply()
{
    // Preverimo, če v izrazu že obstaja operator, če ne, ga dodamo
    if (currentInput.find('x') == std::string::npos)
    {
        currentInput.push_back('x');
        currentOperator = 'x';
    }
    updateDisplay();
}

void Screen1View::operatorDel()
{
    // Preverimo, če v izrazu že obstaja operator, če ne, ga dodamo
    if (currentInput.find('/') == std::string::npos)
    {
        currentInput.push_back('/');
        currentOperator = '/';
    }
    updateDisplay();
}

void Screen1View::operatorPlus()
{
    // Preverimo, če v izrazu že obstaja operator, če ne, ga dodamo
    if (currentInput.find('+') == std::string::npos)
    {
        currentInput.push_back('+');
        currentOperator = '+';
    }
    updateDisplay();
}

void Screen1View::operatorCalculate()
{
    // Preverimo, da je bil določen operator
    if (currentOperator == '\0')
    {
        return;
    }

    // Poiščemo pozicijo operatorja v trenutnem vnosu
    size_t pos = currentInput.find(currentOperator);
    if (pos == std::string::npos)
    {
        return;
    }

    // Razdelimo currentInput na dva operandna niza (npr. "7+9")
    std::string operand1 = currentInput.substr(0, pos);
    std::string operand2 = currentInput.substr(pos + 1);

    // Preverimo, da sta oba operanda prisotna
    if (operand1.empty() || operand2.empty())
    {
        return;
    }

    // Pretvorimo operandne nize v cela števila
    int num1 = std::stoi(operand1);
    int num2 = std::stoi(operand2);
    int result = 0;

    // Uporabimo switch stavek za izvedbo ustrezne operacije
    switch (currentOperator)
    {
        case '+':
            result = num1 + num2;
            break;
        case '-':
            result = num1 - num2;
            break;
        case 'x':
            result = num1 * num2;
            break;
        case '/':
            if (num2 == 0)
            {
                // Če pride do deljenja z 0, prikažemo napako
                currentInput = "Error";
                updateDisplay();
                return;
            }
            result = num1 / num2;
            break;
        default:
            return;
    }

    // Pretvorimo rezultat v niz (uporabimo format "%d")
    char resultBuffer[20];
    snprintf(resultBuffer, sizeof(resultBuffer), "%d", result);

    // Nastavimo currentInput na rezultat in počistimo prejšnje vrednosti
    currentInput = resultBuffer;
    previousInput.clear();
    currentOperator = '\0';

    updateDisplay();
}

