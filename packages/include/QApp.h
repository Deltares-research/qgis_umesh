// RUN MLSupportCppHParser.exe
// corresponding multi language support files are generated

#ifndef _INC_QAPP
#define _INC_QAPP

#include "DllLinkage.h"
#include <QtWidgets/QApplication>


class QApp {

public:

    DLL_LINK static void init(int HWND);
    DLL_LINK static void close();
    DLL_LINK static int getHWND();

private:

};

#undef DLL_LINK

#endif  /* _INC_QAPP */
