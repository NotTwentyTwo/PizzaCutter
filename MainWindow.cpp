#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QColor>

#include "algorithm"
#include <cmath>


QRgb color(int r, int g, int b)
{
    return qRgb(r, g, b);
}

const auto pep = color(170,0,0);
const auto crust = color(170,170,170);
const auto cheese = color(170,170,85);

const auto pepH = QColor::fromHsv(7,83,73);
const auto crustH = QColor::fromHsv(33,58,96);
const auto cheeseH = QColor::fromHsv(44,30,92);
const auto sauseH = QColor::fromHsv(170,170,85);

const auto pepOut = color(255,0,0);
const auto crustOut = color(0,255,0);
const auto cheeseOut = color(0,0,255);

const auto black = color(0,0,0);
const auto white = color(255,255,255);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    load();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openButton_clicked()
{
    auto path =  QFileDialog::getOpenFileName(this, "Open Image", QString());
    if(path.isEmpty())
    {
        return;
    }
    inputImagePath = path;
    load();
}

double MainWindow::calcCutAngle(const int width, const int height, std::function<PizzaPart (int, int)> getComponent, const int cuts)
{
    int totalX(0), totalY(0);
    int pizzaCount = 0;

    //Find center of pizza
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (getComponent(x, y) != PizzaPart::notPizza)
            {
                totalX += x;
                totalY += y;
                pizzaCount++;
            }
        }
    }

    centerX = std::floor(totalX / pizzaCount);
    centerY = std::floor(totalY / pizzaCount);

    cutLength = 0;
    int checkingLength = 0;

    //Check distance to top left
    checkingLength = std::abs(std::floor(std::sqrt((0 - centerX) * (0 - centerX) + (0 - centerY) * (0 - centerY))));
    if (checkingLength > cutLength)
        cutLength = checkingLength;

    //Check distance to top right
    checkingLength = std::abs(std::floor(std::sqrt((width - centerX) * (width - centerX) + (0 - centerY) * (0 - centerY))));
    if (checkingLength > cutLength)
        cutLength = checkingLength;

    //Check distance to bottom right
    checkingLength = std::abs(std::floor(std::sqrt((width - centerX) * (width - centerX) + (height - centerY) * (height - centerY))));
    if (checkingLength > cutLength)
        cutLength = checkingLength;

    //Check distance to bottom left
    checkingLength = std::abs(std::floor(std::sqrt((0 - centerX) * (0 - centerX) + (height - centerY) * (height - centerY))));
    if (checkingLength > cutLength)
        cutLength = checkingLength;

    //We now have the cut length. Time to do some baaaddd math
    int maxNumberOfAngles = 360 / (cuts * 2);
    int bestAngle = 0;
    int hitsInBestAngle = INT_MAX;

    for (int checkingAngle = 0; checkingAngle < maxNumberOfAngles; checkingAngle++)
    {
        //We're checking a new angle. Our total hits so far is 0;
        int hitsThisAngle = 0;

        //We do all cuts in order, starting with primary
        for (int currentCutIndex = 0; currentCutIndex < cuts * 2; currentCutIndex++)
        {
            //Keep track of what the last pixel was. We're going to be getting a lot of repeats
            int     lastX(0), lastY(0);

            //What spot are we looking at
            int     currentX(0), currentY(0);

            //How many blanks have we hit in a row?
            int         blanksInRow = 0;
            const int   blankTolerance = 3;

            for (int spotAlongCut = 0; spotAlongCut < cutLength; spotAlongCut++)
            {
                if (spotAlongCut == 0)
                {
                    currentX = centerX;
                    currentY = centerY;
                }
                else {
                    currentX = std::floor(centerX + std::cos((checkingAngle + maxNumberOfAngles * currentCutIndex) * M_PI / 180) * spotAlongCut);
                    currentY = std::floor(centerY + std::sin((checkingAngle + maxNumberOfAngles * currentCutIndex) * M_PI / 180) * spotAlongCut);

                    //Due to rounding, we are on the same spot we checked last time. Continue loop
                    if (currentX == lastX && currentY == lastY)
                        continue;

                    //It was a fresh pixel. Make sure we don't do it next time again.
                    lastX = currentX;
                    lastY = currentY;

                    //We're out of the image. Don't try to check and bust us out of the cutting loop
                    if (currentX > width || currentY > height || currentX < 0 || currentY < 0)
                        break;

                    switch (getComponent(currentX, currentY))
                    {
                        case PizzaPart::notPizza:
                            blanksInRow++;
                            break;
                        case PizzaPart::peperoni:
                            blanksInRow = 0;
                            //We hit a new Pepperoni. This incident will be reported
                            hitsThisAngle++;
                            break;
                        default:
                            blanksInRow = 0;
                            break;
                    }

                    //We hit too many blanks in a row. We're outside the pizza and done cutting
                    if (blanksInRow >= blankTolerance)
                        break;

                }

            }
        }

        if (hitsThisAngle < hitsInBestAngle)
        {
            hitsInBestAngle = hitsThisAngle;
            bestAngle = checkingAngle;
        }
    }
    return bestAngle;
}

void MainWindow::on_processButton_clicked()
{
    if(inputMap.isNull())
        return;

    load();
    process();
    drawCut();

    inputMap = QPixmap::fromImage(inputImage);
    ui->image->setPixmap(inputMap);
}

double distance(QRgb a, QRgb b)
{
    int ar = qRed(a);
    int ab = qBlue(a);
    int ag = qGreen(a);

    int br = qRed(b);
    int bb = qBlue(b);
    int bg = qGreen(b);

    auto com = [](int a, int b)
    {
        return (a - b) * (a - b);
    };
    return sqrt(0.0 + com(ar, br) + com(ab, bb) + com(ag, bg));
}

double polarDistance(const int h1, const int s1, const int h2, const int s2)
{
    return std::sqrt(s1*s1+s2*s2 - 2*s1*s2*std::cos(s2*M_PI/180-s1*M_PI/180));
}

double distance(QColor a, QColor b)
{
    int d;
    int ah;
    int as;
    int bh;
    int bs;
    a.getHsv(&ah, &as, &d);
    b.getHsv(&bh, &bs, &d);
//    auto com = [](int a, int b)
//    {
//        return (a - b) * (a - b);
//    };
//    auto pol = [](int a, int b)
//    {
//        auto c = std::min(abs(a - b), abs(a - b - 360));
//        return c * c;
//    };
//    return std::sqrt(com(ah, bh) + com(as, bs));
    return polarDistance(ah, as, bh, bs);
}

QRgb reduce(QRgb in, int factor)
{
    auto adjust = [factor](int v)
    {
        return (v / factor) * factor;
    };
    QRgb pixel = in;
    int red = qRed(pixel);
    int blue = qBlue(pixel);
    int green = qGreen(pixel);

    red = adjust(red);
    blue = adjust(blue);
    green = adjust(green);
    return qRgb(red, green, blue);
}

PizzaPart getPart(QRgb pix)
{

    const auto distPep = distance(pix, pep);
    const auto distCrust = distance(pix, crust);
    const auto distCheese = distance(pix, cheese);

    auto low = std::min(distPep, std::min(distCrust, distCheese));
    if(120 < low || pix == black || pix == white)
    {
        return PizzaPart::notPizza;
    }
    if(low == distPep)
    {
        return PizzaPart::peperoni;
    }
    if(low == distCrust)
    {
        return PizzaPart::curst;
    }
    if(low == distCheese)
    {
        return PizzaPart::cheese;
    }
    return PizzaPart::other;
}

PizzaPart getPartOut(QRgb pix)
{

    if(pix == pepOut)
    {
        return PizzaPart::peperoni;
    }
    if(pix == crustOut)
    {
        return PizzaPart::curst;
    }
    if(pix == cheeseOut)
    {
        return PizzaPart::cheese;
    }
    if(pix == white)
    {
        return PizzaPart::notPizza;
    }
    return PizzaPart::other;
}

PizzaPart getPart(QColor pix)
{
    const auto distPep = distance(pix, pepH);
    const auto distCrust = distance(pix, crustH);
    const auto distCheese = distance(pix, cheeseH);

    auto low = std::min(distPep, std::min(distCrust, distCheese));
    if(240 < low || pix == black || pix == white)
    {
        return PizzaPart::notPizza;
    }
    if(low == distPep)
    {
        return PizzaPart::peperoni;
    }
    if(low == distCrust)
    {
        return PizzaPart::curst;
    }
    if(low == distCheese)
    {
        return PizzaPart::cheese;
    }
    return PizzaPart::other;

}

void MainWindow::process()
{
    int devisor = 255 / ui->colorCount->value();

    for(int y = 0; y < inputImage.height(); ++y)
    {
        for(int x = 0; x < inputImage.width(); ++x)
        {
            const auto simplePix = reduce(inputImage.pixel(x, y), devisor);
            auto part = getPart(simplePix);

            //test
            QRgb out;
//            if(10 > abs(y - inputImage.height()/ 2))
//            {
//                out = pep;
//            }else{
//                out = crust;
//            }

            //fixme
            switch(part)
            {
            case PizzaPart::cheese:
                out = cheeseOut;
                break;
            case PizzaPart::curst:
                out = crustOut;
                break;
            case PizzaPart::peperoni:
                out = pepOut;
                break;
            case PizzaPart::other:
                out = black;
                break;
            default:
                out = white;
            }

            inputImage.setPixelColor(x, y, out);
        }
    }

    //fixme
    angle = calcCutAngle(inputImage.width(), inputImage.height(), [this](int x, int y)
    {
        const auto pix = inputImage.pixel(x, y);
        return getPartOut(pix);
    }, ui->cutsCount->value());

    //test
//    angle = calcCutAngle(inputImage.width(), inputImage.height(), [this](int x, int y)
//    {
//        if(10 > abs(y - inputImage.height()/ 2))
//        {
//            return PizzaPart::peperoni;
//        }
//        return PizzaPart::curst;
//    }, ui->cutsCount->value());
    ui->output->setText(QString::number(angle));

    load();
    drawCut();
}

void MainWindow::load()
{
    inputMap.load(inputImagePath);
    inputImage = inputMap.toImage();
    ui->image->setPixmap(inputMap);
}

void MainWindow::drawCut()
{
    int width(inputImage.width());
    int height(inputImage.height());
    int cuts = ui->cutsCount->value();
    //We're checking a new angle. Our total hits so far is 0;
    int hitsThisAngle = 0;
    int checkingAngle = angle;
    int maxNumberOfAngles = 360 / (cuts * 2);

    //We do all cuts in order, starting with primary
    for (int currentCutIndex = 0; currentCutIndex < cuts * 2; currentCutIndex++)
    {
        //Keep track of what the last pixel was. We're going to be getting a lot of repeats
        int     lastX(0), lastY(0);

        //What spot are we looking at
        int     currentX(0), currentY(0);

        for (int spotAlongCut = 0; spotAlongCut < cutLength; spotAlongCut++)
        {
            if (spotAlongCut == 0)
            {
                currentX = centerX;
                currentY = centerY;
            }
            else
            {
                currentX = std::floor(centerX + std::cos((checkingAngle + maxNumberOfAngles * currentCutIndex) * M_PI / 180) * spotAlongCut);
                currentY = std::floor(centerY + std::sin((checkingAngle + maxNumberOfAngles * currentCutIndex) * M_PI / 180) * spotAlongCut);
            }

            //Due to rounding, we are on the same spot we checked last time. Continue loop
            if (currentX == lastX && currentY == lastY)
                continue;

            //It was a fresh pixel. Make sure we don't do it next time again.
            lastX = currentX;
            lastY = currentY;

            //We're out of the image. Don't try to check and bust us out of the cutting loop
            if (currentX > width || currentY > height || currentX < 0 || currentY < 0)
                break;

            inputImage.setPixel(currentX, currentY, black);
        }
    }
}
