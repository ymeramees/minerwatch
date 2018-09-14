#include "minerwatch.h"
#include <QApplication>

bool verbose = false;

int main(int argc, char *argv[])
{
    if(argc > 1)
        for(int i = 0; i < argc; i++)
            if(QString("%1").arg(argv[i]) == "-v"){
                QTextStream(stdout) << "-v => debug mode" << endl;
                verbose = true;
            }
    QApplication a(argc, argv);
    MinerWatch w;
    w.show();

    return a.exec();
}
