// PocoWebsite.cpp: 定义应用程序的入口点。
//

#include "main.h"

#ifdef DEBUG
int main(int argc, char* argv[]) {
    std::cout << "\033[44m--* DEBUG START *--\033[0m\n";

    init();

    cv::Mat QRCode{ DrawTool::DrawQRcode("https://www.bilibili.com/", true) };
    cv::imwrite("QRCode.jpg", QRCode);

    TestProject::testModernSqlite();

    std::cout << "\033[44m--*  DEBUG END  *--\033[0m" << std::endl;
    return 0;
}
#endif // DEBUG


#ifndef DEBUG
int main(int argc, char* argv[])
{
    init();
    start();
    return 0;
}
#endif // !DEBUG

