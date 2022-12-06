#ifndef JUMPCUTUTIL_H
#define JUMPCUTUTIL_H

#include <QString>

class JumpCutUtil
{
public:
    JumpCutUtil();
    void processVideo(QString inputFile);
private:
    std::string gen_random(const int len);
    std::string exec(std::string command);
    void dbg(std::string msg);
};

#endif // JUMPCUTUTIL_H
