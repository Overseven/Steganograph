#include "steganograph.h"
#include "ui_steganograph.h"
#include <QFileDialog>
#include <QMessageBox>
#include <stdio.h>
#include <iostream>
#include <QTextStream>
#include <QtEndian>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <QtMath>

QTextStream cout(stdout);
QTextStream cin(stdin);
using namespace cv;
using namespace std;

Steganograph::Steganograph(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Steganograph)
{
    ui->setupUi(this);
    //задание фонового цвета
    QColor QColor1(246, 232, 230);
    QPalette Palette1 = ui->tabWidget->palette();
    Palette1.setColor(backgroundRole(), QColor1);
    Palette1.setColor(foregroundRole(), Qt::black);
    ui->tabWidget->setPalette(Palette1);
    //вывод ключа в строку при открытии программы
    ui->InputKeyArea->setText(StringKey);
    ui->CounterSymb->setText("Введено символов: 0");
    ui->tabWidget->setTabText(0, "Встраивание");
    ui->tabWidget->setTabText(1, "Изъятие");
    ui->tabWidget->setTabText(2, "Редактировать ключ");
    namedWindow(imageWindow);
    namedWindow(HiddenImage);
    ui->radioButtonText->setChecked(1);
    ui->ChooseStgImgButton->setDisabled(1);
    ui->radioButtonToImg->setChecked(1);
}

Steganograph::~Steganograph()
{
    delete ui;
}

double Steganograph::PSNRFullImage(Mat InputImg1, Mat InputImg2)
{
    double PSNR, MSE=0, trash;
    for (int i=0; i<InputImg1.rows; i++){
        for (int j=0; j<InputImg1.cols; j++){
            trash=InputImg1.at<Vec3b>(i, j)[0]-InputImg2.at<Vec3b>(i, j)[0];
            MSE+=trash*trash;
            trash=InputImg1.at<Vec3b>(i, j)[1]-InputImg2.at<Vec3b>(i, j)[1];
            MSE+=trash*trash;
            trash=InputImg1.at<Vec3b>(i, j)[2]-InputImg2.at<Vec3b>(i, j)[2];
            MSE+=trash*trash;
        }
    }
    MSE=MSE/((double)InputImg1.rows*(double)InputImg1.cols*3.0);
    if (MSE==0)
        PSNR = 100;
    else
        PSNR = 10.0*log10(255.0*255.0/MSE);
    return PSNR;
}

QBitArray Steganograph::toBitArray(const QByteArray &dataByte)
{
    //размер (в битах) переданного массива в функцию
     unsigned int sizeArr = dataByte.size()*8;
    unsigned int index=0;
    //создание нового битного массива
    QBitArray dataBit(sizeArr);
    //побитовое чтение байтового массива
    for (unsigned char Byte : dataByte){
        for (unsigned int Bit = 0; Bit < 8; Bit++){
            if (Byte & 0x01)
                dataBit.setBit(index, true);
            Byte = Byte >> 1;
            index++;
        }
    }
    //возвращение полученного массива бит
    return dataBit;
}

QByteArray Steganograph::toByteArray(const QBitArray &dataBit)
{
    unsigned int index = 0;
    //подсчет длины битовой последовательности
    unsigned int countBit = dataBit.count();
    //подсчет длины байтовой последовательности
    unsigned int countByte = (countBit + 7) / 8;
    //создание нового массива байт
    QByteArray byteArr(countByte, 0);
    //побитовый перевод в байты
    for (unsigned int n = 0; n < countBit;){
        unsigned char byte = 0;
        for (unsigned int bit = 0; bit < 8; bit++){
            byte = byte >> 1;
            if (n < countBit)
                if (dataBit[n++])
                    byte |= 0x80;
        }
    byteArr[index++] = byte;
    }
    //возвращение массива
    return byteArr;
}

QBitArray Steganograph::ConvertFrameToBitArray(Mat &Frame)
{
    QBitArray FrameBitArray(Frame.cols*Frame.rows*3*8);
    QByteArray FrameByteArray((char*)Frame.data, Frame.cols*Frame.rows*3);
    FrameBitArray = toBitArray(FrameByteArray);
    return FrameBitArray;
}

Mat Steganograph::BitPlane(Mat &Frame, int &CountBitPlane) // CountBitPlan: 0 - младший бит, ... 7 - старший бит
{
    int cols, rows, n;
    cols = Frame.cols;
    rows = Frame.rows;
    Mat ResultMat(rows, cols, CV_8UC1, Scalar(0));
    n = pow(2, CountBitPlane);
    for (int y = 0; y < rows; y++){
        for (int x = 0; x < cols; x++){
            ResultMat.at<uchar>(y, x) = (Frame.at<uchar>(y, x) & uchar(n)) ? uchar(255) : uchar(0); //Here's where I AND by either 1,2,4,8,16,32,64, or 128 to get the corresponding bit planes
        }
    }
    return ResultMat;
}

Mat Steganograph::PutDataInFrame(Mat &Frame, QBitArray &InputInfoBit, char &TypeOfHideInfo) //char: T, I, V
{
    cv::Mat FrameWithInfo = Frame;
    QByteArray TypeOfHideInfoByte;
    TypeOfHideInfoByte.append(TypeOfHideInfo);
    QBitArray TypeOfHideInfoBit = toBitArray(TypeOfHideInfoByte);
    int index = 0;
    int ChangedBit=0;
    //вектор, хранящий цветовые компоненты пикселей в байтах
    QVector<unsigned char> colors(Frame.cols*Frame.rows*3);
    for (int y = 0; y < Frame.rows; y++){
        for (int x = 0; x < Frame.cols; x++){
            //построчное считывание пикселей
            colors[index++] = Frame.at<Vec3b>(y,x)[2]; //красный
            colors[index++] = Frame.at<Vec3b>(y,x)[1]; //зелённый
            colors[index++] = Frame.at<Vec3b>(y,x)[0]; //синий

        }
    }
    index = 0;
    //встраивание в младшие биты массива color одного байта с информацией о типе встраиваемых данных
    for (int i=0; i<8; i++){
        if (!(TypeOfHideInfoBit[i])){
            //если изменяем младший бит, то увеличиваем счётчик на 1
            if (colors[index] & 0x0001)
                ChangedBit++;
            colors[index++] &= 0x00FE;
        }else{
            if (!(colors[index] & 0x0001))
                ChangedBit++;
            colors[index++] |= 0x0001;
        }
    }
    if (TypeOfHideInfo=='T'){          // ///////////////////////Если встраиваем текст, то встраиваем информацию о длине сообщения
        QByteArray countSymbByte;
        for(int i = 0; i < 16; i++)
        {
            //перенос значения из переменной textSize
            countSymbByte.append((char)((textSize & (0xFF << (i*8))) >> (i*8)));
        }
        //QMessageBox::information(this, tr("Message!"), "text size: " + QString::number(textSize));

        QBitArray countSymbBit = toBitArray(countSymbByte);
        //встраивание двух байтов с информацией о длине сообщения в младшие биты массива color
        index = 8;
        for (int i=0; i<16; i++){
            if (!(countSymbBit[i])){
                //если изменяем младший бит, то увеличиваем счётчик на 1
                if (colors[index] & 0x0001)
                    ChangedBit++;
                colors[index++] &= 0x00FE;
            }else{
                if (!(colors[index] & 0x0001))
                    ChangedBit++;
                colors[index++] |= 0x0001;
            }
        }


    }else if (TypeOfHideInfo=='I' || TypeOfHideInfo=='V'){    // //////Если встраиваем видео или картинку/////////////////////////////////
        if (TypeOfHideInfo=='V'){                         // ///////Если встраиваем видео, то встраиваем число кадров/////////////////
            QByteArray CountFrameByte;
            for(int i = 0; i < 16; i++)
            {
                //перенос значения из переменной CountFrame
                CountFrameByte.append((char)((CountFrameVH & (0xFF << (i*8))) >> (i*8)));
            }
            QBitArray CountFrameBit = toBitArray(CountFrameByte);
            //встраивание двух байтов с информацией о количестве фреймов в младшие биты массива color
            index=8;
            for (int i=0; i<16; i++){
                if (!(CountFrameBit[i])){
                    //если изменяем младший бит, то увеличиваем счётчик на 1
                    if (colors[index] & 0x0001)
                        ChangedBit++;
                    colors[index++] &= 0x00FE;
                }else{
                    if (!(colors[index] & 0x0001))
                        ChangedBit++;
                    colors[index++] |= 0x0001;
                }
            }
        } // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //переменная, хранящая ширину встраиваемого фрейма в байтах
        QByteArray HiddenMatWidthByte;
        for(int i = 0; i < 16; i++)
        {
            //перенос значения из переменной HiddenMatWidth
            HiddenMatWidthByte.append((char)((HiddenMatWidth & (0xFF << (i*8))) >> (i*8)));
        }
        //переменная, хранящая число символов в сообщении в байтах
        //QMessageBox::information(this, tr("Message!"), "Ширина");

        QByteArray HiddenMatHeightByte;
        for(int i = 0; i < 16; i++)
        {
            //перенос значения из переменной HiddenMatHeight
            HiddenMatHeightByte.append((char)((HiddenMatHeight & (0xFF << (i*8))) >> (i*8)));
        }
        QBitArray HiddenMatWidthBit = toBitArray(HiddenMatWidthByte);
        QBitArray HiddenMatHeightbBit = toBitArray(HiddenMatHeightByte);

        index = 24;
        //встраивание двух байтов с информацией о ширине фрейма в младшие биты массива color
        for (int i=0; i<16; i++){
            if (!(HiddenMatWidthBit[i])){
                //если изменяем младший бит, то увеличиваем счётчик на 1
                if (colors[index] & 0x0001)
                    ChangedBit++;
                colors[index++] &= 0x00FE;
            }else{
                if (!(colors[index] & 0x0001))
                    ChangedBit++;
                colors[index++] |= 0x0001;
            }
        }
        //встраивание двух байтов с информацией о высоте фрейма в младшие биты массива color
        for (int i=0; i<16; i++){
            if (!(HiddenMatHeightbBit[i])){
                //если изменяем младший бит, то увеличиваем счётчик на 1
                if (colors[index] & 0x0001)
                    ChangedBit++;
                colors[index++] &= 0x00FE;
            }else{
                if (!(colors[index] & 0x0001))
                    ChangedBit++;
                colors[index++] |= 0x0001;
            }
        }

    }
    // ///////////////////////////////////////////////////// ВСТРАИВАНИЕ ДАННЫХ///////////////////////////////////////////////////
    //встраивание данных в массив color
    index = 56;
    for (unsigned i = 0; i<(unsigned)InputInfoBit.count(); i++){
        if (!(InputInfoBit[i])){
            if (colors[index] & 0x0001)
                ChangedBit++;
            colors[index++] &= 0x00FE;
        }else{
            if (!(colors[index] & 0x0001))
                ChangedBit++;
            colors[index++] |= 0x0001;
        }
    }
    index = 0; //index=0
    //перенос данных из массива color в цвета нового изображения
    for ( int y = 0 ; y < Frame.rows; y++)
        for ( int x = 0 ; x < Frame.cols; x++){
            FrameWithInfo.at<Vec3b>(y,x)[2] = colors[index++];
            FrameWithInfo.at<Vec3b>(y,x)[1] = colors[index++];
            FrameWithInfo.at<Vec3b>(y,x)[0] = colors[index++];
     }
    return FrameWithInfo;
}

QBitArray Steganograph::ExtractFromFrame(Mat &Frame)
{
    unsigned int LimitOfIndex=0;
    //вектор, хранящий цветовые компоненты пикселей в байтах
    QVector<unsigned char> colors(Frame.cols*Frame.rows*3);
    unsigned int index = 0;
    //считывание цветовых компонент пикселей
    //QMessageBox::warning(0,"Warning", "colors count: " + QString::number(colors.count()));
    for (int y = 0; y < Frame.rows; y++){
        for (int x = 0; x < Frame.cols; x++){
            //построчное считывание пикселей
            colors[index++] = Frame.at<Vec3b>(y,x)[2]; //красный
            colors[index++] = Frame.at<Vec3b>(y,x)[1]; //зелённый
            colors[index++] = Frame.at<Vec3b>(y,x)[0]; //синий
        }
    }

    // //////////////////////////////////////////////////////////////////////ОПРЕДЕЛЕНИЕ ТИПА ВСТРОЕННЫХ ДАННЫХ///////////////////
    //массив бит, хранящий тип встроенной информации
    QBitArray TypeOfHideInfoBit(8);
    //Изъятие типа встроенной информации
    for (int n = 0; n < 8; n++){
        if (colors[n] & 0x0001)
            TypeOfHideInfoBit[n]=true;
        else
            TypeOfHideInfoBit[n]=false;
    }
    //перевод из битовой в байтовую форму
    QByteArray TypeOfHideInfoByte = toByteArray(TypeOfHideInfoBit);
    //тип встроенной информации
    TypeOfHideInfo  = TypeOfHideInfoByte.at(0);
    //QMessageBox::warning(0,"Warning", "Type Hidden Info: " + QString::QString(TypeOfHideInfo));

    if (TypeOfHideInfo=='T'){  // ///////////////////////////////////////////Если встроен текст, то изымаем число символов в сообщении
        //массив бит, хранящий число символов сокрытого собщения
        QBitArray sizeTextBit(16);
        //Изъятие числа спрятанных символов
        for (int n = 8, i=0; n < 24; n++, i++){
            if (colors[n] & 0x0001)
                sizeTextBit[i]=true;
            else
                sizeTextBit[i]=false;
        }
        //перевод из битного в байтовую форму
        QByteArray countText = toByteArray(sizeTextBit);
        //число символов
        textSize  = qFromLittleEndian<quint16>((uchar*)countText.data());
        LimitOfIndex = textSize*8+56;
        //QMessageBox::warning(0,"Warning", QString::number(textSize));
    }else if(TypeOfHideInfo=='I' || TypeOfHideInfo=='V'){ // ////////////////////Если встроено видео или изображение//////////////////////
        if (TypeOfHideInfo=='V' && FirstExtract){  // ///////////////Если встроено видео, то при первом обращении изымаем число кадров
            //массив бит, хранящий число кадров в видео
            QBitArray CountFrameBit(16);
            for (int n = 8, i = 0; n < 24; n++, i++){
                if (colors[n] & 0x0001)
                    CountFrameBit[n]=true;
                else
                    CountFrameBit[n]=false;
            }
            //перевод из битного в байтовую форму
            QByteArray CountFrameByte = toByteArray(CountFrameBit);
            //число кадров
            CountFrameVH  = qFromLittleEndian<quint16>((uchar*)CountFrameByte.data());
            //QMessageBox::warning(0,"Warning", QString::number(CountFrameVH));
            FirstExtract = false;
        }  // ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //массивы бит, хранящие ширину и высоту встроенного фрейма
        QBitArray HiddenMatWidthBit(16);
        QBitArray HiddenMatHeightbBit(16);
        index = 24;
        for (int n = 0; n < 16; n++, index++){
            if (colors[index] & 0x0001)
                HiddenMatWidthBit[n]=true;
            else
                HiddenMatWidthBit[n]=false;
        }
        //перевод из битовой в байтовую форму
        QByteArray HiddenMatWidthByte = toByteArray(HiddenMatWidthBit);
        //тип встроенной информации
        HiddenMatWidth = qFromLittleEndian<quint16>((uchar*)HiddenMatWidthByte.data());

        for (int n = 0; n < 16; n++, index++){
            if (colors[index] & 0x0001)
                HiddenMatHeightbBit[n]=true;
            else
                HiddenMatHeightbBit[n]=false;
        }
        //перевод из битовой в байтовую форму
        QByteArray HiddenMatHeightbByte = toByteArray(HiddenMatHeightbBit);
        //тип встроенной информации
        HiddenMatHeight = qFromLittleEndian<quint16>((uchar*)HiddenMatHeightbByte.data());
        //QMessageBox::warning(0,"Warning", "HiddenMatWidth: " + QString::number(HiddenMatWidth) + "  HiddenMatHeight: " + QString::number(HiddenMatHeight));
        LimitOfIndex = HiddenMatHeight*HiddenMatWidth*8*3;

    }
    // //////////////////////////////////////////////////////////////////////////////////ИЗЪЯТИЕ САМИХ ДАННЫХ/////////////////////
    //изъятие блока  встроенных данных
    QBitArray Data(LimitOfIndex-56);
    index = 56;
    for (int i = 0; index < LimitOfIndex; i++){
        if ((colors[index++] & 0x0001) == 0x0001){
            Data[i] = true;
        }
        else{
            Data[i] = false;
        }
    }
    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    return Data;
}

void Steganograph::PutDataInVideo(VideoCapture &Video, QBitArray &Data, char &TypeOfHideInfo)
{
    //тест на одном кадре
    CvSize size(Video.get(CAP_PROP_FRAME_WIDTH), Video.get(CAP_PROP_FRAME_HEIGHT));
    VideoWriter OutputVideoFile ("/Users/igor/Desktop/Steganography/Images/hah.mp4", CV_FOURCC('H','2','6','4'), 20, size, true); // CV_FOURCC_PROMPT
    OutputVideoFile.set(VIDEOWRITER_PROP_QUALITY, 100);                             //CV_FOURCC('m','j','p','g')
    Mat Frame, HFrame;
    QBitArray databit;
    QBitArray ImageInFrameBit;
    QByteArray ExtractDataByte;
    //Mat matrix (OutputVideoFile.);
    //QMessageBox::warning(0,"Warning", "Type Of Hide Info: " + QString::QString(TypeOfHideInfo));
    //QMessageBox::warning(0,"Warning", "Число элементов в Data: " + QString::number(Data.count()));
//
    if (TypeOfHideInfo == 'T' || TypeOfHideInfo == 'I'){

        for (unsigned int i = 1; i< 100; i++){
            Video >> Frame;
            Frame = PutDataInFrame(Frame, Data, TypeOfHideInfo);
            imshow(imageWindow, Frame);
            ImageInFrameBit = ExtractFromFrame(Frame);
            ExtractDataByte = toByteArray(ImageInFrameBit);
            cv::Mat HiddenImageMat = Mat(HiddenMatHeight, HiddenMatWidth, CV_8UC3, ExtractDataByte.data());
            imshow(HiddenImage, HiddenImageMat);

            OutputVideoFile << Frame;
        }
    }else if (TypeOfHideInfo == 'V'){
        for (unsigned int i = 1; i< 100; i++){
            Video >> Frame;
            VideoHidden >> HFrame;
            //QMessageBox::information(this, tr("Message!"), "Jiza ");

            cv::resize(HFrame, HFrame, cv::Size(), 0.25, 0.25);
            databit = ConvertFrameToBitArray(HFrame);
            Frame = PutDataInFrame(Frame, databit, TypeOfHideInfo);
            imshow(imageWindow, Frame);
            //QMessageBox::information(this, tr("Message!"), "Jiza 1");

            ImageInFrameBit = ExtractFromFrame(Frame);
            //QMessageBox::information(this, tr("Message!"), "Jiza 2");

            ExtractDataByte = toByteArray(ImageInFrameBit);
            cv::Mat HiddenImageMat = Mat(HiddenMatHeight, HiddenMatWidth, CV_8UC3, ExtractDataByte.data());
            imshow(HiddenImage, HiddenImageMat);
            //QMessageBox::information(this, tr("Message!"), "Jiza 3");
            OutputVideoFile << Frame;
            waitKey(30);
        }
    }
    //
    OutputVideoFile.release();
}

QBitArray Steganograph::ExtractFromVideo()
{
    Mat Frame; //(Video.get(CAP_PROP_FRAME_HEIGHT), Video.get(CAP_PROP_FRAME_WIDTH)); //CV_8UC3

    Video.read(Frame);
    imshow(HiddenImage, Frame);
    QBitArray Data = ExtractFromFrame(Frame);
    return Data;
}

void Steganograph::CryptMessage()
{
    QByteArray ByteKey; //байтовое представление ключа
    QBitArray BitKey; //битовое представление ключа
    int NumOfBlocks=textSize/8; //хранит число секций текста по 64 бита
    ByteKey = StringKey.toLatin1(); //байтовое представление ключа  toUtf8()
    BitKey=toBitArray(ByteKey); //битовое представление ключа
    BlockA.resize(32);
    BlockB.resize(32);
    ExchangeBlock.resize(32);
    for (int NumBlocks=0; NumBlocks<NumOfBlocks; NumBlocks++){
        //формирование блоков А и Б
        for (int i=0; i<32; i++){
            BlockA[i]=MessageBit[NumBlocks*64+i];
            BlockB[i]=MessageBit[NumBlocks*64+32+i];
        }
        int ItterKey=0;
        //переменная ItterOfCrypt отвечает за число циклов шифрования
        for (int ItterOfCrypt=0; ItterOfCrypt<8; ItterOfCrypt++){
            //Копируем блок А во временную переменную
            for (int i=0; i<32; i++)
                ExchangeBlock[i]=BlockA[i];
            for (int a=0; a<8; a++){ //каждый проход цикла работает с одним блоком по 4 бита
                //Сложение блока А с ключом
                for(int b=0; b<4; b++)
                    BlockA[a*4+b]=BlockA[a*4+b] ^ BitKey[ItterKey*4+b];
                ItterKey++; if (ItterKey==64) ItterKey=0;
                //Сложение полученного блока А с блоком Б, и копирование значения из начального блока А в блок Б
                for(int b=0; b<4; b++){
                    BlockA[a*4+b]=BlockA[a*4+b] ^ BlockB[a*4+b];
                    BlockB[a*4+b]=ExchangeBlock[a*4+b];
                }
            }
        }
        //Возвращение полученной 64-битной последовательности
        for (int i=0; i<32; i++){
            MessageBit[NumBlocks*64+i]=BlockA[i];
            MessageBit[NumBlocks*64+32+i]=BlockB[i];
        }
    }
}

void Steganograph::DecryptMessage()
{
    QByteArray ByteKey; //байтовое представление ключа
    QBitArray BitKey; //битовое представление ключа
    int NumOfBlocks=textSize/8; //хранит число секций текста по 64 бита
    BlockA.resize(32);
    BlockB.resize(32);
    ExchangeBlock.resize(32);
    ByteKey = StringKey.toLatin1(); //перевод ключа в байтовое представление
    BitKey=toBitArray(ByteKey); //перевод ключа в битовое представление
    int ItterKey; //хранит актуальный номер блока ключа из 4-х бит
    for (int NumBlocks=0; NumBlocks<NumOfBlocks; NumBlocks++){
        //формирование блоков А и Б
        for (int i=0; i<32; i++){
            BlockA[i]=MessageBit[NumBlocks*64+i];
            BlockB[i]=MessageBit[NumBlocks*64+32+i];
        }
        ItterKey=1;
        for (int ItterOfCrypt=0; ItterOfCrypt<8; ItterOfCrypt++){
            //формирования блока замены
            for (int i=0; i<32; i++)
                ExchangeBlock[i]=BlockB[i];
            //каждый проход цикла работает с одним блоком по 4 бита
            for (int a=7; a>=0; a--){
                //Сложение блока Б с ключом
                for(int b=0; b<4; b++){
                    BlockB[a*4+b]=BlockB[a*4+b] ^ BitKey[256-ItterKey*4+b];
                }
                //инкрементирование счётчика блоков ключа
                ItterKey++; if (ItterKey==65) ItterKey=1;
                //Сложение полученного блока А с блоком Б, и копирование значения из начального блока А в блок Б
                for(int b=0; b<4; b++){
                    BlockB[a*4+b]=BlockB[a*4+b] ^ BlockA[a*4+b];
                    BlockA[a*4+b]=ExchangeBlock[a*4+b];
                }
            }
        }
        //Возвращение полученной 64-битной последовательности
        for (int i=0; i<32; i++){
            MessageBit[NumBlocks*64+i]=BlockA[i];
            MessageBit[NumBlocks*64+32+i]=BlockB[i];
        }
    }
}

// //////////////////////////////////////////////////////////КНОПКИ УПРАВЛЕНИЯ///////////////////////////////////////
// //////////////////////////////////////////////////////////ПЕРВАЯ ВКЛАДКА//////////////////////////////////////////

void Steganograph::on_ChooseStgImgButton_clicked()
{
    HiddenFileName = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "/Users/igor/Desktop/Steganography/Images", //потом - оставить только кавычки, всё работает и без пути
                "All Files (*.*)");
    if (HiddenFileName.isEmpty()){
        return;
    }
    //переменная, содержащая факт нажатия кнопки
    BoolLoadImageToHide=true;
    //загрузка изображения в переменную ImgMat
    if (TypeOfHideInfo == 'I'){
        HideImgMat=imread(HiddenFileName.toUtf8().constData(), CV_LOAD_IMAGE_COLOR); //CV_LOAD_IMAGE_COLOR - цветное, CV_LOAD_IMAGE_GRAYSCALE - оттенки серого
        cv::resize(HideImgMat, HideImgMat, cv::Size(), 0.5, 0.5); //в дальнейшем можно будет выбирать

    /*
    namedWindow("0");
    imshow("0", BitPlane(HideImgMat, 0));
    namedWindow("1");
    imshow("1", BitPlane(HideImgMat, 1));
    namedWindow("2");
    imshow("2", BitPlane(HideImgMat, 2));
    namedWindow("3");
    imshow("3", BitPlane(HideImgMat, 3));
    namedWindow("4");
    imshow("4", BitPlane(HideImgMat, 4));
    namedWindow("5");
    imshow("5", BitPlane(HideImgMat, 5));
    namedWindow("6");
    imshow("6", BitPlane(HideImgMat, 6));
    namedWindow("7");
    imshow("7", BitPlane(HideImgMat, 7));
*/
        imshow(HiddenImage, HideImgMat);
        HiddenMatWidth=HideImgMat.cols;
        HiddenMatHeight=HideImgMat.rows;
        HiddenDataSizeBit=HiddenMatWidth*HiddenMatHeight*8*3;
    }
    else if (TypeOfHideInfo == 'V'){
        VideoHidden.open(HiddenFileName.toUtf8().constData());
        VideoHidden.set(CV_CAP_PROP_FORMAT, CV_8UC3);
        HiddenMatWidth = VideoHidden.get(CAP_PROP_FRAME_WIDTH)/4;
        HiddenMatHeight = VideoHidden.get(CAP_PROP_FRAME_HEIGHT)/4;
        cv::Mat fr;
        VideoHidden >> fr;
        imshow(HiddenImage, fr);
        CountFrameVH = VideoHidden.get(CV_CAP_PROP_FRAME_COUNT);
        QMessageBox::information(this, tr("Message!"), "Nums of Frame: " + QString::number(CountFrameVH));
        HiddenDataSizeBit=width*height*8*3;
    }
}

void Steganograph::on_OpenFileTab1_clicked()
{
    if (ui->radioButtonToImg->isChecked()){      // ///////////////////////////////////ЕСЛИ ВСТРАИВАЕМ В ИЗОБРАЖЕНИЕ/////////
        //переменная, хранящая полное имя выбранного файла
        filename = QFileDialog::getOpenFileName(
                    this,
                    tr("Open File"),
                    "/Users/igor/Desktop/Steganography/Images",
                    "All Files (*.*)");
        if (filename.isEmpty()){
            return;
        }
        //переменная, содержащая факт нажатия кнопки
        BoolLoadFile=true;
        //загрузка изображения в переменную ImgMat
        ImgMat=imread(filename.toUtf8().constData(), CV_LOAD_IMAGE_COLOR); //CV_LOAD_IMAGE_COLOR - цветное,
                                                                           //CV_LOAD_IMAGE_GRAYSCALE - оттенки серого
        width=ImgMat.cols;
        height=ImgMat.rows;
        FrameSizeBit=width*height*8*3;
        cv::Mat matrix;
        cv::resize(ImgMat, matrix, cv::Size(), 0.5, 0.5);
        imshow(imageWindow, matrix);
    }else if (ui->radioButtonToVideo->isChecked()){ // ///////////////////////////////////ЕСЛИ ВСТРАИВАЕМ В ВИДЕО////////////
        filename = QFileDialog::getOpenFileName(
                    this,
                    tr("Open File"),
                    "/Users/igor/Desktop/Steganography/Images", //потом - оставить только кавычки, всё работает и без пути
                    "All Files (*.*)");
        Video.open(filename.toUtf8().constData());
        Video.set(CV_CAP_PROP_FORMAT, CV_8UC3);
        width = Video.get(CAP_PROP_FRAME_WIDTH);
        height = Video.get(CAP_PROP_FRAME_HEIGHT);
        CountFrame = Video.get(CV_CAP_PROP_FRAME_COUNT);
        QMessageBox::information(this, tr("Message!"), "Nums of Frame: " + QString::number(CountFrame));
        FrameSizeBit=width*height*8*3;
        BoolLoadFile = true;
        //imshow(imageWindow, ImgMat);
    }
}

void Steganograph::on_InputStegButton_clicked()
{
    if (BoolLoadFile==false){
        QMessageBox::information(this, tr("Message!"), "Сначала загрузите изображение");
    }else{

        if (ui->radioButtonText->isChecked()){         // /////////////////////////////////////////// Если встраиваем текст
            if (ui->InputTextArea->toPlainText() <= 0){
                QMessageBox::information(this, tr("Message!"), "Введите сообщение!");
                return;
            }
            //считывание введённого текста из окна
            message=ui->InputTextArea->toPlainText();
            //переменная, хранящая размер сообщения (в байтах)
            textSize = message.size();
            //проверка длины текста на кратность 8
            int sizesize=8-textSize%8;
            if (textSize%8!=0){
                for (int i=0; i<sizesize; i++){
                    message+='!';
                    textSize++;
                }
            }
            //проверка на превышение доступного объёма для встраивания сообщения
            if ((textSize*8) > (FrameSizeBit-16))
                QMessageBox::information(this, tr("Message!"), "Слишком длинное сообщение");
                return;
            //байтовое представление сообщения
            QByteArray dataByte = message.toLatin1();
            //перевод из массива байт в массив бит
            MessageBit = toBitArray(dataByte);
            //вызов функции, шифрующей текст
            CryptMessage();

        }else if (ui->radioButtonImg->isChecked()){    // /////////////////////////////////////////// Если встраиваем картинку
            if (BoolLoadFile == false){
                QMessageBox::information(this, tr("Message!"), "Сначала загрузите встраиваемый файл");
                return;
            }
            MessageBit = ConvertFrameToBitArray(HideImgMat);
            //QMessageBox::warning(0,"Warning", "Широта: " + QString::number(HiddenMatWidth));
            //QMessageBox::warning(0,"Warning", "Высота: " + QString::number(HiddenMatHeight));

        }else if (ui->radioButtonVideo->isChecked()){    // /////////////////////////////////////////// Если встраиваем видео
            if (BoolLoadFile == false){
                QMessageBox::information(this, tr("Message!"), "Сначала загрузите встраиваемый файл");
                return;
            }

            /*if (HiddenDataSizeBit*8 >= FrameSizeBit){
                QMessageBox::information(this, tr("Message!"), "Слишком большой объём данных для выбранного стего-контейнера");
                return;
            }*/
            //QMessageBox::warning(0,"Warning", "Широта: " + QString::number(HiddenMatWidth));
            //QMessageBox::warning(0,"Warning", "Высота: " + QString::number(HiddenMatHeight));
    }
        BoolInputText=true;
        /*if (MessageBit.count()*8 >= FrameSizeBit){
            QMessageBox::information(this, tr("Message!"), "Слишком большой объём данных для выбранного стего-контейнера");
            return;
        }*/
        if (!TypeOfStegoCont){                         // /////////////////////////////////////////// Если встраиваем в изображение
            imwrite("/Users/igor/Desktop/Steganography/Images/result.bmp", PutDataInFrame(ImgMat, MessageBit, TypeOfHideInfo));

        }else{                                         // /////////////////////////////////////////// Если встраиваем в видео
            PutDataInVideo(Video, MessageBit, TypeOfHideInfo);
        }
    }
}

// //////////////////////////////////////////////////////////ВТОРАЯ ВКЛАДКА//////////////////////////////////////////

void Steganograph::on_OpenFileTab2_clicked()
{
    //переменная, хранящая полное имя выбранного файла
    filename = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "/Users/igor/Desktop/Steganography/Images",
                "All Files (*.*)");
    int bmp=-1, jpg=-1, mp4=-1;
    if (!filename.isEmpty()){
        bmp = filename.indexOf(".bmp");
        jpg = filename.indexOf(".jpg");
        mp4 = filename.indexOf(".mp4");
    }
    if ((bmp<=0 && mp4<=0) || (bmp>0 && mp4>0)){
        QMessageBox::information(this, tr("Message!"), "Не выбран файл или формат выбранного файла не поддерживается");
        return;
    }else if (bmp > 0 || jpg > 0){                     // ЕСЛИ ЗАГРУЗИЛИ ИЗОБРАЖЕНИЕ

        TypeOfStegoCont = false;
        ImgMat=imread(filename.toUtf8().constData(), CV_LOAD_IMAGE_COLOR); //CV_LOAD_IMAGE_COLOR - цветное, CV_LOAD_IMAGE_GRAYSCALE - оттенки серого
        //вывод изображения в отдельное окно
        cv::Mat matrix;
        cv::resize(ImgMat, matrix, cv::Size(), 0.5, 0.5);
        imshow(HiddenImage, matrix);
        width=ImgMat.cols;
        height=ImgMat.rows;
        //вычисление объёма всего файла
        FrameSizeBit=width*height*8*3-56;
        //переменная, содержащая факт нажатия кнопки
        BoolLoadFile=true;


    }else if (mp4 > 0){                     // ЕСЛИ ЗАГРУЗИЛИ ВИДЕО-ФАЙЛ
        Video.open(filename.toUtf8().constData());
        if(!Video.isOpened())
            return;
        Video.set(CV_CAP_PROP_FORMAT, CV_8UC3);
        TypeOfStegoCont = true;
        BoolLoadFile=true;
        width = Video.get(CAP_PROP_FRAME_WIDTH);
        height = Video.get(CAP_PROP_FRAME_HEIGHT);
        CountFrame = Video.get(CV_CAP_PROP_FRAME_COUNT);
        //QMessageBox::information(this, tr("Message!"), "Nums of Frame: " + QString::number(CountFrame));
        FrameSizeBit=width*height*8*3;
        //imshow(HiddenImage, ImgMat);
    }

}

void Steganograph::on_ExtractDataButton_clicked()
{
    //проверка на загруженный файл
    if (BoolLoadFile==false){
        QMessageBox::information(this, tr("Message!"), "Сначала откройте изображение");
        return;
    }
        if (!TypeOfStegoCont)                                   //Если  открыли картинку
            ExtractDataBit = ExtractFromFrame(ImgMat);
        else                                                    //Если открыли видео
            ExtractDataBit = ExtractFromVideo();



        if (TypeOfHideInfo == 'T'){                             //Если встроено сообщение

            MessageBit=ExtractDataBit;
                    //вызов функции, расшифровыващей сообщение
            DecryptMessage();
            //перевод из бит в байты
            message = toByteArray(MessageBit);
            //стирание лишних вспомогательных символов
            int sizeMessage=message.size()-1;
            while (message.endsWith('!')){
                message=message.left(sizeMessage);
                sizeMessage--;
            }
            ui->PrintTextArea->setText(message);

        }else if (TypeOfHideInfo == 'I'){                       //Если встроено изображение

            QByteArray ExtractDataByte = toByteArray(ExtractDataBit);
            cv::Mat HiddenImageMat = Mat(HiddenMatHeight, HiddenMatWidth, CV_8UC3, ExtractDataByte.data());
            imshow(HiddenImage, HiddenImageMat);

        }else if (TypeOfHideInfo == 'V'){                       //Если встроено видео

        }
        //обнуление переменных
        BitArray.fill(0);
        filename.fill(0);
        width=0;
        height=0;
        FrameSizeBit=0;
        BoolLoadFile=false;
        BoolInputText=false;
        message.fill(0);
        textSize=0;
        MessageBit.fill(0);

}

// //////////////////////////////////////////////////////////ТРЕТЬЯ ВКЛАДКА//////////////////////////////////////////

void Steganograph::on_ChangeKey_clicked()
{
    //Проверка на длину введённого ключа
    if (ui->InputKeyArea->text().size()!=32)
        QMessageBox::information(this, tr("Message!"), "Измените длину ключа.");
    else{
        StringKey=ui->InputKeyArea->text();
    }
}

// //////////////////////////////////////////////////////////ПОБОЧНЫЕ ФУНКЦИИ////////////////////////////////////////

void Steganograph::on_InputKeyArea_textChanged()
{
    QString EditKey=ui->InputKeyArea->text();
    int EditKeySize=EditKey.size();
    //передача числа введённых символов
    on_label_change_name(EditKeySize);
}

void Steganograph::on_label_change_name(const int &num)
{
    //динамическая проверка на длину ключа
    if (num<32)
        ui->Status->setText("Не хватает " + (QString::number(32-num)) + " символов");
    else if (num==32)
        ui->Status->setText("Длина ключа подходит.");
    else
        ui->Status->setText("Удалите " + (QString::number(num-32)) + " символов");
}

void Steganograph::on_InputTextArea_textChanged()
{
    //отображение числа введённых символов
    ui->CounterSymb->setText("Введено символов: " + QString::number(ui->InputTextArea->toPlainText().size()));
}

void Steganograph::on_radioButtonText_clicked()
{
    ui->ChooseStgImgButton->setDisabled(1);
    ui->InputTextArea->setDisabled(0);
    TypeOfHideInfo = 'T';
}

void Steganograph::on_radioButtonImg_clicked()
{
    ui->ChooseStgImgButton->setDisabled(0);
    ui->InputTextArea->setDisabled(1);
    TypeOfHideInfo = 'I';
}

void Steganograph::on_radioButtonToImg_clicked()
{
    TypeOfStegoCont = false;
}

void Steganograph::on_radioButtonToVideo_clicked()
{
    TypeOfStegoCont = true;
}

void Steganograph::on_radioButtonVideo_clicked()
{
    ui->ChooseStgImgButton->setDisabled(0);
    ui->InputTextArea->setDisabled(1);
    TypeOfHideInfo = 'V';
}
