#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QElapsedTimer>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct Color
{
    Color(int c, int n)
    {
        col = c;
        num = n;
    }
    int col;
    int num;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void slot_convert();
    void slot_about();
private:
    int byte_to_int(QByteArray arr);
    int cul_delt(int col1, int col2);
    void add_data(QByteArray &arr, int data, int lenght);
    void edit_data(QByteArray &arr, int data, int begin, int length);
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
