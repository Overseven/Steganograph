#ifndef STEGANOGRAPH_H
#define STEGANOGRAPH_H

//#include <media.h>
#include <QWidget>
#include <QByteArray>
#include <QBitArray>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv/cv.h>
#include <opencv2/opencv.hpp>

namespace Ui {
class Steganograph;
}

class Steganograph : public QWidget
{
    Q_OBJECT

public:
    explicit Steganograph(QWidget *parent = 0);
    ~Steganograph();

private slots:
    void on_InputStegButton_clicked();
    void on_ChangeKey_clicked();
    void on_InputKeyArea_textChanged();
    void on_InputTextArea_textChanged();
    void on_OpenFileTab2_clicked();
    void on_OpenFileTab1_clicked();
    void on_radioButtonText_clicked();
    void on_radioButtonImg_clicked();
    void on_ChooseStgImgButton_clicked();
    void on_radioButtonToImg_clicked();
    void on_radioButtonToVideo_clicked();
    void on_ExtractDataButton_clicked();

    void on_radioButtonVideo_clicked();

private:
    cv::Mat ImgMat; //стего-контейнер
    cv::Mat HideImgMat; //фрейм встраиваемой информации
    cv::VideoCapture Video; //видео-поток (стего-контейнер)
    cv::VideoCapture VideoHidden;
    unsigned int CountFrame;
    unsigned int CountFrameVH;
    QBitArray BitArray;
    bool TypeOfStegoCont = false; //описывает формат стего-контейнера, FALSE - изображение, TRUE - видео
    bool FirstExtract = true;
    char TypeOfHideInfo = 'T'; //описывает тип информации, которая будет встроена, I - Image, V - Video, T - Text
    QString filename;
    QString HiddenFileName;
    int width, HiddenMatWidth; //width - описывает ширину фрейма картинки или видео, куда встраиваем, HiddenMatWidth - ширина встраиваемого видео или картинки
    int height, HiddenMatHeight;
    unsigned int HiddenDataSizeBit, FrameSizeBit;
    //unsigned int PSNR;
    bool BoolLoadFile = false;
    bool BoolLoadVideo = false;
    bool BoolInputText = false;
    bool BoolLoadImageToHide = false;
    QString message;
    quint16 textSize = 0;
    QBitArray MessageBit;
    QBitArray ExtractDataBit;

    //Media *stegoContainer;
    //Media *secretMediaFile;

    QBitArray toBitArray(const QByteArray &);
    QByteArray toByteArray(const QBitArray &);
    cv::Mat PutDataInFrame(cv::Mat &Frame, QBitArray &InputInfoBit, char &TypeOfHideInfo);
    QBitArray ExtractFromFrame(cv::Mat &Frame);
    QBitArray ExtractFromVideo();
    QBitArray ConvertFrameToBitArray(cv::Mat &Frame);
    cv::Mat BitPlane(cv::Mat &Frame, int &CountBitPlane);
    double PSNRFullImage(cv::Mat InputImg1, cv::Mat InputImg2);
    void PutDataInVideo(cv::VideoCapture &Video, QBitArray &Data, char &TypeOfHideInfo);
    void extractVideo();
    void CryptMessage();
    void DecryptMessage();
    void on_label_change_name(const int &num);


    QString StringKey = "Sk36236kjhHjhkehGKhg48oKH4gjkjb3";
    const std::string imageWindow = "Original Image";
    const std::string HiddenImage = "StegoImg";
    QBitArray BlockA, BlockB, ExchangeBlock;
    Ui::Steganograph *ui;
};


#endif // STEGANOGRAPH_H
