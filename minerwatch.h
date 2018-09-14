#ifndef MINERWATCH_H
#define MINERWATCH_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QTextStream>
#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QUdpSocket>
#include <QLabel>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <stdio.h>

//#include <QDebug>

#include "kaevuririda.h"

namespace Ui {
class MinerWatch;
}

class MinerWatch : public QMainWindow
{
    Q_OBJECT

public:
    explicit MinerWatch(QWidget *parent = 0);
    ~MinerWatch();
    struct Settings {
        QString serialName;
        qint32 baudRate;
        QSerialPort::DataBits dataBits;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBits;
        QSerialPort::FlowControl flowControl;
    };

private:
    bool server;
    int doesNotReplyTime; //Aeg sekundites, mis hetkest näidatakse staatuseks "ei vasta"
    int heartBeatTime;  //Kui kaua Arduino ootab viimasest signaalist alates enne, kui juhtkaevurile restart tehakse, sekundites
    int checkInterval;     //Kui tihti kontrollitakse teiste kaevurite staatust ja enda elus olemise signaali Arduinole saadetakse, sekundites
    int maxRestartNo;   //Mitu restardi katset maksimaalselt tehakse
    int resetPressTime; //Kui kaua restarti tehes reset nuppu all hoitakse, millisekundites
    int restartTime1;    //Aeg, sekundites, kui kaua oodatakse teiste kaevurite elus olemise signaali enne, kui restart neile tehakse
    int restartTime2;    //Aeg, sekundites, kui kaua oodatakse teiste kaevurite elus olemise signaali, peale esimest restarti, enne, kui uus restart tehakse
    int restartTime3;    //Aeg, sekundites, kui kaua oodatakse teiste kaevurite elus olemise signaali, peale esimest restarti, enne, kui uus restart tehakse
    Ui::MinerWatch *ui;
    QFile *logi = nullptr; //Logifail
    QList<KaevuriRida*> kaevuriRead;    //Rida programmi aknas iga kaevuri jaoks
    QList<QSerialPortInfo> ports;
//    QMap<QString, int> kaevurid;
    QString oma_nimi;   //Selle masina nimi
    QTimer kontroll;    //Timer staatuste kontrolli jooksutamiseks
    QUdpSocket *udpSocket = nullptr;
    QSerialPort *serial = nullptr;
    Settings settings;


private slots:
    void avaSerial();   //Arduinoga ühenduse loomine
    void kirjutaLogisse(QString s);
    void loeConfFail();
    void loeSerialist();    //Arduinoga suhtlus
    void loeTeateid();  //Teistest arvutitest tulnud broadcast'ide lugemine
    void otsiSerialiNimi();
    void staatuseKontroll();
    void teeRestart(QString s);  //Käsitsi restardi tegemine

};

#endif // MINERWATCH_H
