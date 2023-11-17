/*
 * RawLogReader.cpp
 *
 *  Created on: 19 Nov 2012
 *      Author: thomas
 */

#include "RawLogReader.h"

RawLogReader::RawLogReader(Bytef *& decompressionBuffer,
                           cv::Mat *& deCompImage,
                           std::string file,
                           bool flipColors)
 : decompressionBuffer(decompressionBuffer),
   deCompImage(deCompImage),
   file(file),
   flipColors(flipColors),
   isCompressed(false)
{
    assert(boost::filesystem::exists(file.c_str()));

    fp = fopen(file.c_str(), "rb");

    currentFrame = 0;

    assert(fread(&numFrames, sizeof(int32_t), 1, fp));

    depthReadBuffer = new unsigned char[Resolution::getInstance().numPixels() * 2];
    imageReadBuffer = new unsigned char[Resolution::getInstance().numPixels() * 3];
}

RawLogReader::~RawLogReader()
{
    delete [] depthReadBuffer;
    delete [] imageReadBuffer;

    fclose(fp);
}

void RawLogReader::getNext()
{
    assert(fread(&timestamp, sizeof(int64_t), 1, fp));

    assert(fread(&depthSize, sizeof(int32_t), 1, fp));
    assert(fread(&imageSize, sizeof(int32_t), 1, fp));

    assert(fread(depthReadBuffer, depthSize, 1, fp));

    if(imageSize > 0)
    {
        assert(fread(imageReadBuffer, imageSize, 1, fp));
    }

    if(deCompImage != 0)
    {
        delete deCompImage;
    }

    cv::Mat tempMat = cv::Mat(1, imageSize, CV_8UC1, (void *)imageReadBuffer);

    if(imageSize == Resolution::getInstance().numPixels() * 3)
    {
        isCompressed = false;

        deCompImage = new cv::Mat(Resolution::getInstance().height(), Resolution::getInstance().width(), CV_8UC3);

        memcpy(deCompImage->data, imageReadBuffer, Resolution::getInstance().numPixels() * 3);
    }
    else if(imageSize > 0)
    {
        isCompressed = true;

        deCompImage = new cv::Mat(cv::imdecode(cv::Mat(tempMat), cv::IMREAD_COLOR));
    }
    else
    {
        isCompressed = false;

        deCompImage = new cv::Mat(Resolution::getInstance().height(), Resolution::getInstance().width(), CV_8UC3);

        memset(deCompImage->data, 0, Resolution::getInstance().numPixels() * 3);
    }

    if(depthSize == Resolution::getInstance().numPixels() * 2)
    {
        //RGB should not be compressed in this case
        assert(!isCompressed);

        memcpy(&decompressionBuffer[0], depthReadBuffer, Resolution::getInstance().numPixels() * 2);
    }
    else if(depthSize > 0)
    {
        //RGB should also be compressed
        assert(isCompressed);

        unsigned long decompLength = Resolution::getInstance().numPixels() * 2;

        uncompress(&decompressionBuffer[0], (unsigned long *)&decompLength, (const Bytef *)depthReadBuffer, depthSize);
    }
    else
    {
        isCompressed = false;

        memset(&decompressionBuffer[0], 0, Resolution::getInstance().numPixels() * 2);
    }

    if(flipColors)
    {
        cv::Mat3b rgb(Resolution::getInstance().rows(), Resolution::getInstance().cols(), (cv::Vec<unsigned char, 3> *)deCompImage->data, Resolution::getInstance().width() * 3);
        cv::cvtColor(rgb, rgb, cv::COLOR_RGB2BGR);
    }

    currentFrame++;
}

int RawLogReader::getNumFrames()
{
    return numFrames;
}

bool RawLogReader::hasMore()
{
    return currentFrame + 1 < numFrames;
}
