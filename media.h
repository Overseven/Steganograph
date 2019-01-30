#ifndef MEDIA_H
#define MEDIA_H


#include <QWidget>
#include <cstring>
#include <QByteArray>
#include <QBitArray>
#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.h>
#include <opencv2/opencv.hpp>

class Media {
public:
    Media (const Media &other); //запрет копирования
    Media &operator= (const Media &other); //запрет присваивания



protected:
    Media (){} //конструктор по умолчанию
    QString filename;
    cv::Mat frame;
    size_t size;
    unsigned height;
    unsigned width;

};

class Video : Media {
public:
    Video () : filename('\n'), frame(0), size(0), height(0), width(0), stream_(0), countFrame_(0){}

    Video (const QString &path) : filename(path){
        stream_.open(filename.toUtf8().constData());
        stream_.set(CV_CAP_PROP_FORMAT, CV_8UC3);
        width = stream_.get(CV_CAP_PROP_FRAME_WIDTH)/4;
        height = stream_.get(CV_CAP_PROP_FRAME_HEIGHT)/4;
        stream_ >> frame;
        countFrame_ = stream_.get(CV_CAP_PROP_FRAME_COUNT);
        size=width*height*8*3;
    }
    cv::Mat &getFrame(){

        return frame;
    }
    ~Video(){
        stream_.~VideoCapture();
    }
private:
    cv::VideoCapture stream_;
    size_t numOfFrame_;
    size_t countFrame_;
};




#endif // MEDIA_H
