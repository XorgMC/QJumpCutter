#include "jumpcututil.h"

#include <QDir>
#include <QFile>
#include <iostream>

#include <opencv2/videoio.hpp>
#include <sndfile.h>

#define		BUFFER_LEN	20

JumpCutUtil::JumpCutUtil()
{

}

//https://stackoverflow.com/a/440240
std::string JumpCutUtil::gen_random(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

std::string JumpCutUtil::exec(std::string command){ //stackoverflow!!!
    char buffer[256];
    std::string result = "";

    FILE * pipe = popen(command.c_str(), "r");

    if(!pipe){
        return "couldn't make pipe :(";
    }

    while(!feof(pipe)){
        if(fgets(buffer, 256, pipe) != NULL){
            result += buffer;
        }
    }

    pclose(pipe);
    return result;

}

void JumpCutUtil::dbg(std::string msg) {
    std::cout << msg << std::endl;
}

void JumpCutUtil::processVideo(QString inputFile) {
    dbg("Start Process");
    cv::VideoCapture capture(inputFile.toStdString());

    if(!capture.isOpened()) {
        std::cerr << "Error opening video stream" << std::endl;
        return;
    }

    const int videoLength(capture.get(cv::CAP_PROP_FRAME_COUNT));
    const double fps(capture.get(cv::CAP_PROP_FPS));
    const int videoWidth(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    const int videoHeight(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    const int silentThres(600);
    const int silentSpeed(10);

    QDir tempDir(QDir::temp());
    const std::string originalAudioFile(tempDir.absoluteFilePath( QString::fromStdString(gen_random(8) + ".wav") ).toStdString() );
    const std::string videoFile(tempDir.absoluteFilePath( QString::fromStdString(gen_random(8) + ".mp4") ).toStdString() );
    const std::string audioFile(tempDir.absoluteFilePath( QString::fromStdString(gen_random(8) + ".wav") ).toStdString() );

    dbg("Extracting audio to " + originalAudioFile);

    std::string out = exec("ffmpeg -y -hide_banner -loglevel error -i \"" + inputFile.toStdString() + "\" -ab 160k -ac 2 -ar 44100 -vn " + originalAudioFile);
    dbg("done extracting. ffmpeg output:\n" + out);

    dbg("opening file using libsndfile");
    SNDFILE *wavFile;
    SNDFILE *outpFile;
    SF_INFO sfinfo;
    wavFile = sf_open(originalAudioFile.c_str(), SFM_READ, &sfinfo);
    int sampleRate = sfinfo.samplerate;
    int audioChannels = sfinfo.channels;
    int audioFrames = sfinfo.frames;

    outpFile = sf_open(audioFile.c_str(), SFM_WRITE, &sfinfo);

    sf_count_t readcount;
    static short data[BUFFER_LEN];
    QList<QList<short>> audioData;
    QList<QList<short>> modifiedAudio;

    readcount = (int) sf_read_short (wavFile, data, BUFFER_LEN);
    int readoffset = 0;
    while (readcount > 0) {
        int k, chan, ic, nth;

        for (chan = 0 ; chan < audioChannels ; chan ++) {
            nth = readoffset;
            for (k = chan ; k < readcount ; k+= audioChannels) {
                while(audioData.length() <= nth) {
                    QList<short> sub1;
                    QList<short> sub2;
                    for(int ac = 0; ac < audioChannels; ac ++) { sub1.append(0); sub2.append(0); }
                    audioData.append(sub1);
                    modifiedAudio.append(sub2);
                }
                audioData[nth][chan] = data[k];
                modifiedAudio[nth][chan] = 0;
                nth++;
            }
        }

        readoffset += (int) readcount/audioChannels;
        readcount = (int) sf_read_short (wavFile, data, BUFFER_LEN);
    }



    /*QList<short> slicedModList;
    int outputCounter(0);
    for(int slCount = 0; slCount < audioData.length(); slCount++) {
        for(int chN = 0; chN < audioChannels; chN++) {
            //slicedModAudio[outputCounter] = modifiedAudio[slCount][chN];
            //outputCounter++;
            slicedModList.append(audioData[slCount][chN]);
        }
    }*/

    /*short segment[BUFFER_LEN];
        int offset = 0;
        for(int i = 0; i < slicedModList.length(); i += BUFFER_LEN) {
            int written(0);
            for(int j = 0; j < BUFFER_LEN; j ++) {
                if(i+j < slicedModList.length()) {
                    segment[j] = slicedModList[i+j];
                    written++;
                }
            }

            sf_write_short(outpFile, segment, written);
        }*/

    /*short slicedModAudio[audioData.length() * audioChannels];
    for(int slCount = 0; slCount < slicedModList.length(); slCount++) {
        for(int chN = 0; chN < audioChannels; chN++) {
            slicedModAudio[outputCounter] = audioData[slCount][chN];
            outputCounter++;
        }
    }*/

    /*dbg("Writing audio to " + audioFile);
    sf_write_short(outpFile, slicedModAudio, audioData.length() * audioChannels);*/
    //dbg("done!");

    cv::VideoWriter videoOutput(videoFile, cv::VideoWriter::fourcc('m','p','4','v'), fps, cv::Size(videoWidth, videoHeight));

    int audioPointer(0), audioPointerEnd(0), silentStart(0), silentEnd(-1);
    QList<cv::Mat> buffer;

    while(1) {
        cv::Mat frame;
        bool ret = capture.read(frame);
        if(!ret) break;

        int currentFrame = capture.get(cv::CAP_PROP_POS_FRAMES);
        int audioSampleStart = round(currentFrame / fps * sampleRate);
        int audioSampleCount = sampleRate / fps;
        int audioSampleEnd = audioSampleStart + audioSampleCount;

        if(audioSampleEnd > audioFrames) { dbg("audioSampleEnd too large!!"); audioSampleEnd = audioFrames; }

        dbg("doing frame " + std::to_string(currentFrame));


        //int audioSample[audioSampleCount][audioChannels];
        QList<QList<short>> audioSample;
        audioSample = audioData.mid(audioSampleStart, audioSampleCount);

        int maxLvl(0);

        for(int audFrm = 0; audFrm < audioSample.length(); audFrm ++) {
            for (int chN = 0; chN < audioChannels; chN ++) {
                int sVal = abs(audioSample[audFrm][chN]);
                if(sVal > maxLvl) maxLvl = sVal;
            }
        }

        if(maxLvl < silentThres) {
            buffer.append(frame);
            silentEnd = audioSampleEnd;
            continue;
        }

        if(silentEnd != -1) {
            QList<QList<short>> silentSample;
            //int silentSample[(silentEnd - silentStart][audioChannels];
            int ssa, ssc, scc;
            for(ssa = silentStart; ssa < silentEnd; ssa+=silentSpeed) {
                QList<short> sil;
                sil.append(audioData[ssa]);
                silentSample.append(sil);
            }

            audioPointerEnd = audioPointer + silentSample.size();
            int loopCount = 0;
            for(int apc = audioPointer; apc < audioPointerEnd; apc++) {
                QList<short> sil;
                sil.append(silentSample[loopCount]); //Fast audio
                //for (int chN = 0; chN < audioChannels; chN ++) sil.append(0); //Silence
                modifiedAudio[apc] = sil;
                loopCount++;
            }

            QList<cv::Mat> fastBuffer;
            for(int bufN = 0; bufN < buffer.length(); bufN += silentSpeed) fastBuffer.append(buffer[bufN]);

            audioPointer += round(fastBuffer.length() / fps * sampleRate);

            foreach(cv::Mat frm, fastBuffer) {
                videoOutput.write(frm);
            }

            buffer.clear();
        }

        videoOutput.write(frame);
        silentStart = audioSampleEnd + 1;
        silentEnd = -1;

        audioPointerEnd = audioPointer + audioSampleCount;
        int loopCount = 0;
        for(int apc = audioPointer; apc < audioPointerEnd; apc++) {
            for(int chN = 0; chN < audioChannels; chN++) {
                modifiedAudio[apc][chN] = audioSample[loopCount][chN];
            }
            loopCount++;
        }
        audioPointer = audioPointerEnd;
    }

    QList<short> slicedModList;
    int outputCounter(0);
    int endAudio = audioPointer <= modifiedAudio.length() ? audioPointer : modifiedAudio.length();
    for(int slCount = 0; slCount < endAudio; slCount++) {
        for(int chN = 0; chN < audioChannels; chN++) {
            slicedModList.append(modifiedAudio[slCount][chN]);
        }
    }
    short segment[BUFFER_LEN];
    int offset = 0;
    for(int i = 0; i < slicedModList.length(); i += BUFFER_LEN) {
        int written(0);
        for(int j = 0; j < BUFFER_LEN; j ++) {
            if(i+j < slicedModList.length()) {
                segment[j] = slicedModList[i+j];
                written++;
            }
        }

        sf_write_short(outpFile, segment, written);
    }
    dbg("Writing audio to " + audioFile);

    capture.release();
    videoOutput.release();

}
