#ifndef KAEVURIRIDA_H
#define KAEVURIRIDA_H

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTime>

class KaevuriRida : public QHBoxLayout
{
    Q_OBJECT
public:
    explicit KaevuriRida(QWidget *parent = nullptr);
    int restartNo;  //Näitab, mitu restarti on juba tehtud, nullitakse, kui tuleb alive signaal
    QLabel* nameLabel;
    QLabel* portLabel;
    QLabel* timeLabel;
    QLabel* statusLabel;
    QPushButton* restartButton;   //Saab käsitsi restardi teha

signals:
    void orderRestart(QString s);

public slots:
    void alive();    //"alive" signaali registreerimine
    void orderRestart();    //Saadab signaali, et teha käsitsi restart
};

#endif // KAEVURIRIDA_H
