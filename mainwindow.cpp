#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->butt_convert, SIGNAL(clicked()), SLOT(slot_convert()));
    connect(ui->action_about, SIGNAL(triggered()), SLOT(slot_about()));
}

int MainWindow::byte_to_int(QByteArray arr)
{
    int n = arr.size();
    if(n % 2){
        n--;
    }
    for(int i = 0; i < n / 2; i++){
        qSwap(arr[i], arr[arr.size() - 1 - i]);
    }
    return QByteArray::fromRawData(arr, arr.size()).toHex().toInt(nullptr, 16);
}

int MainWindow::cul_delt(int col1, int col2)
{
    return qPow((col1 >> 16) - (col2 >> 16), 2) +
            qPow(((col1 >> 8) & 0xff) - ((col2 >> 8) & 0xff), 2) +
            qPow((col1 & 0xff) - (col2 & 0xff), 2);
}

bool compareByLength(const Color &a, const Color &b)
{
    return a.num > b.num;
}

void MainWindow::add_data(QByteArray &arr, int data, int lenght)
{
    for(int i = 0, shift = 0; i < lenght; i++, shift+=8){
        arr.push_back((char)((data >> shift) & 0xff));
    }
}

void MainWindow::edit_data(QByteArray &arr, int data, int begin, int length)
{
    for(int i = 0, shift = 0; i < length; i++, shift += 8){
        arr[begin + i] = (char)((data >> shift) & 0xff);
    }
}


void MainWindow::slot_convert()
{
    QByteArray arr;
    QFile file("C:\\Qt\\Project\\RGR_lob\\PCX.pcx");
    // Считываем файл в массив байт
    file.open(QIODevice::ReadOnly);
    arr = file.readAll();
    file.close();
    QHash <int, int> pixels;
    QHash <int, int> colors;
    QVector <Color> vec_palette;
    QElapsedTimer timer;
    // Получаем нужные данные
    int pcx_width = byte_to_int(QByteArray::fromRawData(&arr[8], 2)) + 1;
    int pcx_height = byte_to_int(QByteArray::fromRawData(&arr[10], 2)) + 1;
    int pcx_dpi = byte_to_int(QByteArray::fromRawData(&arr[12], 2));
    int pcx_begin = 128; // Байт начала массива пикселей
    int bmp_palette_size = 256;
    QVector <int> vec_planes;
    timer.start();
    for(int i = pcx_begin, x = 0, y = 0, g = 0; i < arr.size();){
        g = QByteArray::fromRawData(&arr[i], 1).toHex().toInt(nullptr, 16);
        if(g >= 192){
            i++;
            for(int j = 0; j < (g & 0x3f); j++){
                vec_planes.push_back(QByteArray::fromRawData(&arr[i], 1).toHex().toInt(nullptr, 16));
                x++;
                if(x == pcx_width){
                    x = 0;
                    y++;
                }
            }
        }else{
            vec_planes.push_back(QByteArray::fromRawData(&arr[i], 1).toHex().toInt(nullptr, 16));
            x++;
            if(x == pcx_width){
                x = 0;
                y++;
            }
        }
        i++;
    }
    qDebug() << "Получаем массив пикселей из PCX:\t" << timer.elapsed();
    timer.start();
    for(int i = 0, val, index = 0, index_pix = 0; i < pcx_height; ++i){
        for(int j = 0; j < pcx_height; j++){
            val = (vec_planes[index] << 16) + (vec_planes[index + pcx_width] << 8) + vec_planes[i + pcx_width * 2];
            pixels.insert(index_pix, val);
            ++index;
            ++index_pix;
            if(colors.contains(val)){
                colors.insert(val, colors.value(val) + 1);
            }else{
                colors.insert(val, 1);
            }
        }
        index += pcx_width * 2;
    }
    for (auto [key, val] : colors.asKeyValueRange()){
        vec_palette.push_back(Color(key, val));
    }
    std::sort(vec_palette.begin(), vec_palette.end(), compareByLength);
    qDebug() << "Формируем полную палитру цветов:\t" << timer.elapsed();
    timer.start();
    int radius_start = 100, radius_step = 100;
    for(int delt, radius = radius_start; vec_palette.size() > bmp_palette_size; radius+=radius_step){
        for(int i = 0; i < vec_palette.size() - 1; i++){
            for(int j = i + 1; j < vec_palette.size() && (vec_palette.size() > bmp_palette_size); j++){
                delt = cul_delt(vec_palette[i].col, vec_palette[j].col);
                if(delt <= radius){
                    vec_palette.remove(j);
                }
            }
        }
    }
    qDebug() << "Урезаем палитру до 256 цветов:\t" << timer.elapsed();
    timer.start();
    QHash <int, int>::iterator it = colors.begin();
    for(int index = 0; it != colors.end();) {
        for(int i = 0, delt, delt_min = 0; i < vec_palette.size(); i++){
            delt = cul_delt(it.key(), vec_palette[i].col);
            if(delt_min == 0 || delt < delt_min){
                index = i;
                delt_min = delt;
            }
        }
        colors.insert(it.key(), index);
        ++it;
    }
    qDebug() << "Находим похожиецвета из новой палитры:\t" << timer.elapsed();
    timer.start();
    for(int i = 0; i < pixels.size(); i++){
        pixels.insert(i, colors.value(pixels.value(i)));
    }
    qDebug() << "Меняем цвета пикселей на цвета из палитры:\t" << timer.elapsed();
    timer.start();
    arr.clear();
    add_data(arr, 0x4d42, 2);       // Индетификатор разработчика
    add_data(arr, 0, 4);            // Размер файла
    add_data(arr, 0, 4);            // Зарезервированые байты
    add_data(arr, 1078, 4);           // Размер заголовка файла
    add_data(arr, 40, 4);           // Размер DIB заголовка
    add_data(arr, pcx_width, 4);    // Ширина изображения
    add_data(arr, pcx_height, 4);   // Высота изображения
    add_data(arr, 1, 2);            // Количество цветовых плоскостей
    add_data(arr, 8, 2);            // Количество бит на пиксель
    add_data(arr, 0, 4);            // Тип сжатия данных (без сжатия)
    add_data(arr, 16, 4);           // Размер строки bitmap
    add_data(arr, pcx_dpi, 4);      // DPI по горизонтали
    add_data(arr, pcx_dpi, 4);      // DPI по вертикали
    add_data(arr, 256, 4);          // Количество цветов в палитре
    add_data(arr, 0, 4);            // Главный цвет
    for(int i = 0; i < vec_palette.size(); i++){
        add_data(arr, vec_palette[i].col, 4);
    }
    for(int i = pcx_height - 1; i >= 0; i--){
        for(int j = 0; j < pcx_width; j++){
            add_data(arr, pixels.value(i * pcx_width + j), 1);
        }
    }
    qDebug() << "Формируем BMP файл:\t" << timer.elapsed();
    edit_data(arr, arr.size(), 2, 4);
    timer.start();
    file.setFileName("C:\\Qt\\Project\\RGR_lob\\BMP.bmp");
    file.open(QIODevice::WriteOnly);
    for (int i = 0; i < arr.size(); ++i) {
        file.putChar(arr[i]);
    }
    file.close();
    qDebug() << "Сохранили BMP файл:\t" << timer.elapsed();
    ui->label->setPixmap(QPixmap("C:\\Qt\\Project\\RGR_lob\\BMP.bmp").scaled(400,
                                                                             400,
                                                                             Qt::KeepAspectRatio,
                                                                             Qt::SmoothTransformation));
}

void MainWindow::slot_about()
{
    QMessageBox::about(this, "Справка", "Работу выполнил: Лобакин Н.Д."
                                        "\nГруппа: ИП-917"
                                        "\nВариант: 9"
                                        "\nЗадание: Преобразовать True Color PCX"
                                        "\nфайл в 256-цветный BMP файл.");
}

MainWindow::~MainWindow()
{
    delete ui;
}
