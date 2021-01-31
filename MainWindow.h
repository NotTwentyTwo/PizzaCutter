#pragma once

#include <QMainWindow>
#include <QDir>
#include <QString>
#include <QImage>
#include <functional>
#include <QPixmap>

enum class PizzaPart
{
    notPizza,
    curst,
    cheese,
    peperoni,
    other
};


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_openButton_clicked();

    void on_processButton_clicked();

private:

    void process();

    void load();

    void drawCut();

    double calcCutAngle(const int width, const int height, std::function<PizzaPart(int, int)> getComponent, const int cuts);

    int centerX;
    int centerY;
    int cutLength = 0;
    int angle = 0;

    Ui::MainWindow *ui;

    QString inputImagePath = QString("C://Users//Michael Wiegand//Downloads//PizzaInput//five.jpg");
    QPixmap inputMap;
    QImage inputImage;
    std::vector<QImage> componetImages;
};
